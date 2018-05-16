/***************************************************************************
 *
 * Copyright (C) 2017 Tomoyuki KAWAO
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

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <X11/Xutil.h>

#include "common.h"
#include "window-system.h"
#include "uinput-device.h"
#include "keyboard-device.h"

#define _is_valid_window(window) ((window) != None && (window) != PointerRoot)
#define _is_exist_keyboard_data(ws) ((ws)->keysyms || (ws)->modmap || (ws)->xkb)

static gboolean _handle_event(gpointer user_data);
static gboolean _dispatch_event(XSetKeys *xsk,
                                gboolean *xkb_rule_changed,
                                gboolean *keymapping_changed,
                                gboolean *modifier_changed);
static Window _get_focus_window(Display *display);
static gboolean _get_is_excluded(Display *display,
                                 Window window,
                                 gchar **excluded_classes);
static void _get_keyboard_data(Display *display, WindowSystem *ws);
static void _set_keyboard_data(Display *display, WindowSystem *ws);
static void _free_keyboard_data(WindowSystem *ws);
static gboolean _poll_display(Display *display, gint timeout);

WindowSystem *window_system_initialize(XSetKeys *xsk,
                                       gchar *excluded_classes[])
{
  Display *display = xsk_get_display(xsk);
  WindowSystem *ws;
  gint screen;

  ws = (WindowSystem *)device_initialize(ConnectionNumber(display),
                                         "X Window System",
                                         sizeof (WindowSystem),
                                         _handle_event,
                                         xsk);
  ws->excluded_classes = excluded_classes;
  ws->active_window_atom = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
  ws->xkb_rules_atom = XInternAtom(display, "_XKB_RULES_NAMES", False);
  _get_keyboard_data(display, ws);

  ws->focus_window = _get_focus_window(display);
  if (!_is_valid_window(ws->focus_window)) {
    g_critical("XGetInputFocus returned special window");
    device_finalize(&ws->device);
    return NULL;
  }

  ws->is_excluded = _get_is_excluded(display,
                                     ws->focus_window,
                                     ws->excluded_classes);
  debug_print("Input focus window exclusion: %s",
              ws->is_excluded ? "true" : "false");

  for (screen = 0; screen < ScreenCount(display); screen++) {
    XSelectInput(display, RootWindow(display, screen), PropertyChangeMask);
  }
  return ws;
}

void window_system_pre_finalize(XSetKeys *xsk)
{
  WindowSystem *ws = xsk_get_window_system(xsk);

  _free_keyboard_data(ws);
  if (xsk->uinput_device) {
    _get_keyboard_data(xsk_get_display(xsk), ws);
  }
}

void window_system_finalize(XSetKeys *xsk)
{
  WindowSystem *ws = xsk_get_window_system(xsk);

  if (_is_exist_keyboard_data(ws)) {
    _set_keyboard_data(xsk_get_display(xsk), ws);
  }
  device_finalize(&ws->device);
}

static gboolean _handle_event(gpointer user_data)
{
  XSetKeys *xsk = user_data;
  WindowSystem *ws = xsk_get_window_system(xsk);
  gboolean xkb_rule_changed = FALSE;
  gboolean keymapping_changed = FALSE;
  gboolean modifier_changed = FALSE;

  debug_print("Handle X event");

  if (!_dispatch_event(xsk,
                       &xkb_rule_changed,
                       &keymapping_changed,
                       &modifier_changed)) {
    return FALSE;
  }

  if (xkb_rule_changed && _is_exist_keyboard_data(ws)) {
    Display *display = xsk_get_display(xsk);
    gint status = _poll_display(display, 200);
    if (status < 0) {
      return FALSE;
    }
    if (status == 0) {
      debug_print("Poll display Timeout!");
    }
    xkb_rule_changed = FALSE;
    keymapping_changed = FALSE;
    if (!_dispatch_event(xsk,
                         &xkb_rule_changed,
                         &keymapping_changed,
                         &modifier_changed)) {
      return FALSE;
    }
    if (keymapping_changed && !xkb_rule_changed) {
      gint status = _poll_display(display, 200);
      if (status < 0) {
        return FALSE;
      }
      if (status == 0) {
        debug_print("Poll display Timeout!");
      }
      if (!_dispatch_event(xsk,
                           &xkb_rule_changed,
                           &keymapping_changed,
                           &modifier_changed)) {
        return FALSE;
      }
    }
    ud_send_key_events(xsk, ud_get_pressing_keys(xsk), FALSE, TRUE);
    _set_keyboard_data(xsk_get_display(xsk), ws);
    ud_send_key_events(xsk, ud_get_pressing_keys(xsk), TRUE, TRUE);
    return _dispatch_event(xsk,
                           &xkb_rule_changed,
                           &keymapping_changed,
                           &modifier_changed);
  } else if (modifier_changed) {
    kill(getpid(), SIGUSR1);
  }

  return TRUE;
}

static gboolean _dispatch_event(XSetKeys *xsk,
                                gboolean *xkb_rule_changed,
                                gboolean *keymapping_changed,
                                gboolean *modifier_changed)
{
  Display *display = xsk_get_display(xsk);
  WindowSystem *ws = xsk_get_window_system(xsk);

  while (XPending(display)) {
    XEvent event;

    XNextEvent(display, &event);
    switch (event.type) {
    case PropertyNotify:
      debug_print("PropertyNotify: %s",
                  XGetAtomName(display, event.xproperty.atom));
      if (event.xproperty.atom == ws->active_window_atom) {
        Window focus_window;
        gboolean is_excluded;

        focus_window = _get_focus_window(display);
        if (focus_window != ws->focus_window) {
          if (!_is_valid_window(focus_window)) {
            g_critical("XGetInputFocus returned special window");
            return FALSE;
          }
          ws->focus_window = focus_window;
          is_excluded = _get_is_excluded(display,
                                         focus_window,
                                         ws->excluded_classes);
          if (is_excluded && !xsk_is_excluded(xsk)) {
            xsk_reset_state(xsk);
          }
          ws->is_excluded = is_excluded;
          debug_print("Input focus window exclusion: %s",
                      is_excluded ? "true" : "false");
        }
      } else if (event.xproperty.atom == ws->xkb_rules_atom) {
        *xkb_rule_changed = TRUE;
      }
      break;

    case MappingNotify:
      switch (event.xmapping.request) {
      case MappingKeyboard:
        debug_print("MappingKeyboard");
        XRefreshKeyboardMapping(&event.xmapping);
        *keymapping_changed = TRUE;
        break;
      case MappingModifier:
        debug_print("MappingModifier");
        XRefreshKeyboardMapping(&event.xmapping);
        *modifier_changed = TRUE;
        break;
      }
      break;
    }
  }

  return TRUE;
}

static Window _get_focus_window(Display *display)
{
  Window window;
  gint focus_state;

  XGetInputFocus(display, &window, &focus_state);
  return window;
}

static gboolean _get_is_excluded(Display *display,
                                 Window window,
                                 gchar **excluded_classes)
{
  gboolean result = FALSE;
  XClassHint class_hints;
  Window root;
  Window parent;
  Window *children;
  guint nchildren;

  if (!excluded_classes) {
    return FALSE;
  }

  for (;;) {
    if (XGetClassHint(display, window, &class_hints)) {
      break;
    }
    if (!XQueryTree(display, window, &root, &parent, &children, &nchildren)) {
      print_error("XQueryTree failed window=%lx", window);
      return FALSE;
    }
    XFree(children);
    if (window == root || parent == root) {
      g_critical("Can not get window class for input focus window");
      return FALSE;
    }
    window = parent;
  }

  for (; *excluded_classes; excluded_classes++) {
    if (!g_strcmp0(*excluded_classes, class_hints.res_name)) {
      result = TRUE;
      break;
    }
    if (!g_strcmp0(*excluded_classes, class_hints.res_class)) {
      result = TRUE;
      break;
    }
  }

  XFree(class_hints.res_name);
  XFree(class_hints.res_class);

  return result;
}

static void _get_keyboard_data(Display *display, WindowSystem *ws)
{
  gint max_keycodes;

  XDisplayKeycodes(display, &ws->min_keycodes, &max_keycodes);
  ws->num_keycodes = max_keycodes - ws->min_keycodes + 1;
  ws->keysyms = XGetKeyboardMapping(display,
                                    ws->min_keycodes,
                                    ws->num_keycodes,
                                    &ws->keysyms_per_keycode);
  if (!ws->keysyms) {
    print_error("XGetKeyboardMapping failed!");
  }

  ws->modmap = XGetModifierMapping(display);
  if (!ws->modmap) {
    print_error("XGetModifierMapping failed!");
  }

  ws->xkb = XkbAllocKeyboard();
  if (!ws->xkb) {
    print_error("XkbAllocKeyboard failed!");
  } else if (XkbGetControls(display, XkbAllControlsMask, ws->xkb) != Success) {
    print_error("XkbGetControls failed!");
    XkbFreeKeyboard(ws->xkb, 0, True);
    ws->xkb = NULL;
  }
}

static void _set_keyboard_data(Display *display, WindowSystem *ws)
{
  debug_print("Set keyboard data");

  if (ws->keysyms) {
    XChangeKeyboardMapping(display,
                           ws->min_keycodes,
                           ws->keysyms_per_keycode,
                           ws->keysyms,
                           ws->num_keycodes);
    XFree(ws->keysyms);
    ws->keysyms = NULL;
  }

  if (ws->modmap) {
    gint retries;

    for (retries = 20; retries > 0; retries--) {
      gint status = XSetModifierMapping(display, ws->modmap);
      if (status == MappingBusy) {
        g_usleep(100000);
        continue;
      }
      if (status != MappingSuccess) {
        print_error("XSetModifierMapping returns=%d", status);
      }
      break;
    }
    if (!retries) {
      g_warning("XSetModifierMapping failed by MappingBusy");
    }
    XFreeModifiermap(ws->modmap);
    ws->modmap = NULL;
  }

  if (ws->xkb) {
    if (!XkbSetControls(display, XkbAllControlsMask, ws->xkb)) {
      print_error("XkbSetControls failed!");
    }
    XkbFreeKeyboard(ws->xkb, 0, True);
    ws->xkb = NULL;
  }

  XSync(display, FALSE);
}

static void _free_keyboard_data(WindowSystem *ws)
{
  if (ws->keysyms) {
    XFree(ws->keysyms);
    ws->keysyms = NULL;
  }
  if (ws->modmap) {
    XFreeModifiermap(ws->modmap);
    ws->modmap = NULL;
  }
  if (ws->xkb) {
    XkbFreeKeyboard(ws->xkb, 0, True);
    ws->xkb = NULL;
  }
}

static gint _poll_display(Display *display, gint timeout)
{
  GPollFD fds;
  fds.fd = ConnectionNumber(display);
  fds.events = G_IO_IN | G_IO_HUP | G_IO_ERR;

  for (;;) {
    gint status = g_poll(&fds, 1, timeout);
    if (status > 0) {
      if (fds.revents & G_IO_HUP) {
        print_error("Hang up on display");
        return -1;
      }
      if (fds.revents & G_IO_ERR) {
        print_error("I/O Error on display");
        return -1;
      }
    } else if (status < 0) {
      if (errno == EINTR) {
        continue;
      }
      print_error("g_poll failed!");
    }
    return status;
  }
}
