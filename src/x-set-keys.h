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

#ifndef _X_SET_KEYS_H
#define _X_SET_KEYS_H

#include <X11/Xlib.h>
#include <glib.h>

#include "key-information.h"
#include "key-code-array.h"
#include "action.h"
/*
 * display - dafault display (XOpenDisplay(NULL))
 * key_information - ModifierMapping and "cursor" something??? with KEY_HOME, KEY_UP, KEY_PAGEUP, KEY_LEFT, KEY_RIGHT, KEY_END
 * root_actions - right side of config line
 * current_actions - ?
 * keyboard_device -
 * uinput_device -
 * is_selection_mode - text selection mode
 */
typedef struct XSetKeys_ {
  Display *display;
  KeyInformation key_information;
  struct WindowSystem_ *window_system;
  struct Fcitx_ *fcitx;
  ActionList *root_actions;
  const ActionList *current_actions;
  struct KeyboardDevice_ *keyboard_device;
  struct UInputDevice_ *uinput_device;
  gboolean is_selection_mode;
  gboolean is_stopped_mode;
} XSetKeys;

typedef enum XskResult_ {
  XSK_CONSUMED,
  XSK_UNCONSUMED,
  XSK_FAILED
} XskResult;

gboolean xsk_initialize(XSetKeys *xsk, gchar *excluded_classes[]);
gboolean xsk_start(XSetKeys *xsk,
                   const gchar *device_filepath,
                   gchar *excluded_fcitx_input_methods[]);
void xsk_finalize(XSetKeys *xsk, gboolean is_restart);

XskResult xsk_handle_key_press(XSetKeys *xsk, KeyCode key_code);
XskResult xsk_handle_key_repeat(XSetKeys *xsk,
                                KeyCode key_code,
                                gboolean is_after_key_repeat_delay);
gboolean xsk_send_key_events(XSetKeys *xsk,
                             const KeyCodeArrayArray *key_arrays);
void xsk_toggle_selection_mode(XSetKeys *xsk);
void xsk_toggle_stopped_mode(XSetKeys *xsk, gboolean is_start);
gboolean xsk_is_excluded(XSetKeys *xsk);
void xsk_reset_state(XSetKeys *xsk);
void xsk_mapping_changed(XSetKeys *xsk);

#define xsk_get_display(xsk) ((xsk)->display)
#define xsk_get_key_information(xsk) (&(xsk)->key_information)
#define xsk_get_window_system(xsk) ((xsk)->window_system)
#define xsk_get_root_actions(xsk) ((xsk)->root_actions)
#define xsk_get_keyboard_device(xsk) ((xsk)->keyboard_device)
#define xsk_get_uinput_device(xsk) ((xsk)->uinput_device)
#define xsk_get_fcitx(xsk) ((xsk)->fcitx)

#define xsk_set_current_actions(xsk, actions)   \
  ((xsk)->current_actions = (actions))

#endif /* _X_SET_KEYS_H */
