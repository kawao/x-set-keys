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

#include <X11/Xutil.h>

#include "common.h"
#include "window-system.h"

static gboolean _handle_event(gpointer user_data);

static Window _get_focus_window(Display *display);
static gboolean _get_is_excluded(Display *display,
                                 Window window,
                                 gchar **excluded_classes);

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
  ws->focus_window = _get_focus_window(display);
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

void window_system_finalize(XSetKeys *xsk)
{
  WindowSystem *ws = xsk_get_window_system(xsk);

  device_finalize(&ws->device);
}

static gboolean _handle_event(gpointer user_data)
{
  XSetKeys *xsk = user_data;
  Display *display = xsk_get_display(xsk);
  WindowSystem *ws = xsk_get_window_system(xsk);

  while (XPending(display)) {
    XEvent event;

    XNextEvent(display, &event);
    if (event.type == PropertyNotify) {
      if (event.xproperty.atom == ws->active_window_atom) {
        Window focus_window = _get_focus_window(display);

        if (focus_window != ws->focus_window) {
          gboolean is_excluded;

          ws->focus_window = focus_window;
          is_excluded = _get_is_excluded(display,
                                         focus_window,
                                         ws->excluded_classes);
          if (is_excluded && !ws->is_excluded) {
            xsk_reset_state(xsk);
          }
          ws->is_excluded = is_excluded;
          debug_print("Input focus window exclusion: %s",
                      is_excluded ? "true" : "false");
        }
      }
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

  if (excluded_classes == NULL) {
    return FALSE;
  }

  for (;;) {
    if (XGetClassHint(display, window, &class_hints)) {
      break;
    }
    if (!XQueryTree(display, window, &root, &parent, &children, &nchildren)) {
      g_critical("XQueryTree failed window=%lx", window);
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
