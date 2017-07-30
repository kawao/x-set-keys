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

#ifndef _X_SET_KEYS_H
#define _X_SET_KEYS_H

#include <X11/Xlib.h>
#include <glib.h>

#include "key-information.h"
#include "key-code-array.h"
#include "action.h"

typedef struct XSetKeys_ {
  Display *display;
  KeyInformation key_information;
  ActionList *root_actions;
  const ActionList *current_actions;
  struct KeyboardDevice_ *keyboard_device;
  struct UInputDevice_ *uinput_device;
  gboolean is_selection_mode;
} XSetKeys;

typedef enum XskResult_ {
  XSK_CONSUMED,
  XSK_UNCONSUMED,
  XSK_FAILED
} XskResult;

gboolean xsk_initialize(XSetKeys *xsk);
gboolean xsk_start(XSetKeys *xsk, const gchar *device_filepath);
void xsk_finalize(XSetKeys *xsk);

XskResult xsk_handle_key_press(XSetKeys *xsk, KeyCode key_code);
XskResult xsk_handle_key_repeat(XSetKeys *xsk,
                                KeyCode key_code,
                                gint seconds_since_pressed);
gboolean xsk_send_key_events(XSetKeys *xsk,
                             const KeyCodeArrayArray *key_arrays);
void xsk_toggle_selection_mode(XSetKeys *xsk);

#define xsk_get_display(xsk) ((xsk)->display)
#define xsk_get_key_information(xsk) (&(xsk)->key_information)
#define xsk_get_root_actions(xsk) ((xsk)->root_actions)
#define xsk_get_keyboard_device(xsk) ((xsk)->keyboard_device)
#define xsk_get_uinput_device(xsk) ((xsk)->uinput_device)

#define xsk_set_current_actions(xsk, actions)   \
  ((xsk)->current_actions = (actions))

#endif /* _X_SET_KEYS_H */
