/***************************************************************************
 *
 * Copyright (C) 2017-2018 Tomoyuki KAWAO
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ***************************************************************************/
/*
## Terms
- xks - main data structure. x-set-keys.h:XSetKeys_
- actions - key pressed or multiple keys or selection, defined in action.h

## How it works
It Uses Glib library, kernel input, Xlib
- glib.h
- glib-unix.h
- linux/input.h
- X11/Xlib.h

It uses keyboard device /dev/input/event and /dev/uinput (or /dev/input/uinput).
You require "uinput" kernel module.
	Device Drivers -> Input Device Support -> Miscellaneous drivers -> User level driver support

## execution path
- main.c
- x-set-keys.c:xsk_initialize - bind to windows.
  - key-information.c:ki_initialize - get system information about keys
- config.c:config_load -> _parse_line - to xsk object
- action.c:action_list_add_key/select_action -> _add_action
- x-set-keys.c:xsk_start
  - fcitx_initialize
  - keyboard-device.c:kd_initialize - set callback on kb device with xsk argument
  - uinput-device.c:ud_initialize - set callback on uidevice device - pass event ot kb device
  - xsk_reset_state
- main.c:g_main_context_iteration - activate blocking event loop of default GMainContext
- keyboard-device.c:_handle_event
- x-set-keys.c:xsk_handle_key_press(key_code) - check if config exist for keys pressed
  - _key_pressed_on_selection_mode or action->run(xsk, action)
- action.c: _send_key_events or _toggle_selection_mode

## error handling
- main.c: is_debug global variable is set if G_MESSAGES_DEBUG=all is defined in environment.
- common.h: #define debug_print()
- keyboard-device.c and uinput-device.c #ifdef TRACE output current keys pressed.

## source files
- action.c - action object is a key event or multi-stroke or selection.
- common.h - macros: debug_print, print_error, array_num(number of ellements in the array)
- config.c
- device.c - low level keyboard device handling for uinput and keyboard-device
- fcitx.c - watch for org.fcitx.Fcitx at DBus in X11, Fcitx is a Chinese/Japanese input program
- key-code-array.c
- key-information.c
- keyboard-device.c
- main.c - 1 parse_arguments 2 handle signals 3 xsk_initialize, config.config_load, xsk_start
- uinput-device.c - bind keyboard event handlers
- window-system.c
- x-set-keys.c - main file for handling keyboard events.

## selection mode
- Defined as a type in action.h
- "$select" word action in configuration file.
- x-set-keys.c:xsk_handle_key_press(key_code)
- action->run(xsk, action) -> _toggle_selection_mod or _key_pressed_on_selection_mode if xsk->is_selection_mode
- action.c:_toggle_selection_mode - toggle xsk->is_selection_mode
*/
// C Standard Library
#include <errno.h> // error message
#include <stdio.h> // IO
#include <string.h> // strings operations
#include <setjmp.h>

#include <glib-unix.h> // GLib include: g_main_context_iteration
#include <X11/Xlib.h>

#define MAIN
#include "common.h"
#include "x-set-keys.h"
#include "config.h"

// parsed command line arguments
typedef struct _Arguments_ {
  gchar *config_filepath;
  gchar *device_filepath;
  gchar **excluded_classes;
  gchar **excluded_fcitx_input_methods;
} _Arguments;

static volatile gboolean _caught_sigint = FALSE;
static volatile gboolean _caught_sigterm = FALSE;
static volatile gboolean _caught_sighup = FALSE;
static volatile gboolean _caught_sigusr1 = FALSE;
static volatile gboolean _error_occurred = FALSE;
static jmp_buf _xio_error_env;

static void _set_debug_flag();
static gboolean _parse_arguments(gint argc,
                                 gchar *argv[],
                                 _Arguments *arguments);
static void _free_arguments( _Arguments *arguments);
static gboolean _handle_signal(gpointer flag_pointer);
static gint _handle_x_error(Display *display, XErrorEvent *event);
static gint _handle_xio_error(Display *display);
static gboolean _run(const _Arguments *arguments);

gint main(gint argc, gchar *argv[])
{
  _Arguments arguments = { 0 };
  gint error_retry_count = 0;

  g_set_prgname(g_path_get_basename(argv[0]));
  _set_debug_flag();
  // Creates a new option context.
  if (!_parse_arguments(argc, argv, &arguments)) {
    return EXIT_FAILURE;
  }
  // handle signals SIGINT SIGTERM SIGHUP SIGUSR1
  // set TRUE for (gpointer)&_caught_*
  g_unix_signal_add(SIGINT, _handle_signal, (gpointer)&_caught_sigint);
  g_unix_signal_add(SIGTERM, _handle_signal, (gpointer)&_caught_sigterm);
  g_unix_signal_add(SIGHUP, _handle_signal, (gpointer)&_caught_sighup);
  g_unix_signal_add(SIGUSR1, _handle_signal, (gpointer)&_caught_sigusr1);
  // Xlib error handlers
  XSetErrorHandler(_handle_x_error);
  XSetIOErrorHandler(_handle_xio_error);

  // main subroutine goes furher to _run()
  while (_run(&arguments)) { // 10 times restart _run if x error, else just restart
    if (_error_occurred) {
      if (++error_retry_count > 10) {
        g_critical("Maximum error retry count exceeded");
        break;
      }
      g_usleep(G_USEC_PER_SEC);
      _error_occurred = FALSE;
    } else {
      error_retry_count = 0;
    }
    g_message("Restarting");
  }

  g_message("Exiting");
  _free_arguments(&arguments);
  return _error_occurred ? EXIT_FAILURE : EXIT_SUCCESS;
}

void notify_error()
{
  _error_occurred = TRUE;
}

static void _set_debug_flag()
{
  const gchar *domains = g_getenv("G_MESSAGES_DEBUG");

  is_debug = domains != NULL && strcmp(domains, "all") == 0;
}

static gboolean _parse_arguments(gint argc,
                                 gchar *argv[],
                                 _Arguments *arguments)
{
  gboolean result = TRUE;
  GOptionEntry entries[] = {
    {
      "device-file", 'd', 0, G_OPTION_ARG_FILENAME,
      &arguments->device_filepath,
      "Keyboard device file", "<devicefile>"
    }, {
      "exclude-focus-class", 'e', 0, G_OPTION_ARG_STRING_ARRAY,
      &arguments->excluded_classes,
      "Exclude class of input focus window (Can be specified multiple times)",
      "<classname>"
    }, {
      "exclude-fcitx-im", 'f', 0, G_OPTION_ARG_STRING_ARRAY,
      &arguments->excluded_fcitx_input_methods,
      "Exclude input method of fcitx (Can be specified multiple times)",
      "<inputmethod>"
    }, {
      NULL
    }
  };
  GError *error = NULL;
  GOptionContext *context = g_option_context_new("<configuration-file>");

  g_option_context_add_main_entries(context, entries, NULL);
  if (!g_option_context_parse(context, &argc, &argv, &error)) {
    gchar *help;

    g_critical("Option parsing failed: %s", error->message);
    g_error_free(error);

    help = g_option_context_get_help(context, TRUE, NULL);
    g_print("%s\n", help);
    g_free(help);
    result = FALSE;
  } else if (argc != 2) {
    gchar *help;

    if (argc < 2) {
      g_critical("Configuration file must be specified");
    } else {
      g_critical("Too many arguments");
    }

    help = g_option_context_get_help(context, TRUE, NULL);
    g_print("%s\n", help);
    g_free(help);
    result = FALSE;
  } else {
    arguments->config_filepath = argv[1];
  }
  g_option_context_free(context);
  return result;
}

static void _free_arguments( _Arguments *arguments)
{
  if (arguments->device_filepath) {
    g_free(arguments->device_filepath);
  }
  if (arguments->excluded_classes) {
    g_strfreev(arguments->excluded_classes);
  }
  if (arguments->excluded_fcitx_input_methods) {
    g_strfreev(arguments->excluded_fcitx_input_methods);
  }
}

static gboolean _handle_signal(gpointer flag_pointer)
{
  *((volatile gboolean *)flag_pointer) = TRUE;
  g_main_context_wakeup(NULL);
  return G_SOURCE_CONTINUE;
}

static gint _handle_x_error(Display *display, XErrorEvent *event)
{
  gchar message[256];

  XGetErrorText(display, event->error_code, message, sizeof (message));
  g_critical("X protocol error : %s on protocol request %d",
             message,
             event->request_code);
  _error_occurred = TRUE;
  return 0;
}

static gint _handle_xio_error(Display *display)
{
  g_critical("Connection lost to X server `%s'", DisplayString(display));
  longjmp(_xio_error_env, 1);
  return 0;
}

// second main function
static gboolean _run(const _Arguments *arguments)
{
  gboolean is_restart = FALSE;
  XSetKeys xsk = { 0 };

  if (setjmp(_xio_error_env)) {
    _error_occurred = TRUE;
    is_restart = FALSE;
  }
  // xsk_initialize: fill xsk - display, window_system, keys information
  if (!_error_occurred && !xsk_initialize(&xsk, arguments->excluded_classes)) {
    _error_occurred = TRUE;
    if (xsk_get_display(&xsk)) {
      is_restart = TRUE;
    }
  }
  // config_load - find keyboard device
  if (!_error_occurred && !config_load(&xsk, arguments->config_filepath)) {
    _error_occurred = TRUE;
  }
  // bind
  if (!_error_occurred && !xsk_start(&xsk,
                                     arguments->device_filepath,
                                     arguments->excluded_fcitx_input_methods)) {
    _error_occurred = TRUE;
  }

  if (!_error_occurred) {
    debug_print("Starting main loop");
    _caught_sighup = FALSE;
    while (!_caught_sigint &&
           !_caught_sigterm &&
           !_caught_sighup &&
           !_error_occurred) {
      g_main_context_iteration(NULL, TRUE); // glib/gmain.h
      if (_caught_sigusr1 && !_error_occurred) {
        g_message("Keyboard mapping changed");
        xsk_mapping_changed(&xsk);
        if (!_error_occurred &&
            !config_load(&xsk, arguments->config_filepath)) {
          _error_occurred = TRUE;
        }
        _caught_sigusr1 = FALSE;
      }
    }
    is_restart = TRUE;
    if (_caught_sigint) {
      is_restart = FALSE;
      g_message("Caught SIGINT");
    }
    if (_caught_sigterm) {
      is_restart = FALSE;
      g_message("Caught SIGTERM");
    }
    if (_caught_sighup) {
      g_message("Caught SIGHUP");
    }
  }

  g_message(is_restart ? "Initiating restart" : "Initiating shutdown");

  if (setjmp(_xio_error_env)) {
    return FALSE;
  }

  xsk_finalize(&xsk, is_restart);
  return is_restart;
}
