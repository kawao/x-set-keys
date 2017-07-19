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
#include "device.h"

typedef struct _XSetKeys {
  Display *display;
  KeyInformation key_information;
  gpointer root_actions;
  gpointer current_actions;
  KeyCodeArray *keyboard_pressing_keys;
  KeyCodeArray *uinput_pressing_keys;
  Device *keyboard_device;
  Device *uinput_device;
  struct timeval key_press_start_time;
  guint16 uinput_last_event_type;
} XSetKeys;

typedef enum _XskResult {
  XSK_PASSING_BY,
  XSK_INTERCEPTED,
  XSK_ERROR
} XskResult;

gboolean xsk_initialize(XSetKeys *xsk);
gboolean xsk_start(XSetKeys *xsk, const gchar *device_filepath);
void xsk_finalize(XSetKeys *xsk);

gboolean xsk_is_disabled(XSetKeys *xsk);
XskResult xsk_handle_key_press(XSetKeys *xsk, KeyCode key_code);
XskResult xsk_handle_key_repeat(XSetKeys *xsk,
                                KeyCode key_code,
                                gint seconds_since_pressed);

#define xsk_get_display(xsk) ((xsk)->display)

#define xsk_get_key_information(xsk) (&(xsk)->key_information)
#define xsk_get_current_actions(xsk) ((xsk)->current_actions)

#define xsk_get_keyboard_pressing_keys(xsk) ((xsk)->keyboard_pressing_keys)
#define xsk_get_uinput_pressing_keys(xsk) ((xsk)->uinput_pressing_keys)

#define xsk_get_keyboard_device(xsk) ((xsk)->keyboard_device)
#define xsk_get_uinput_device(xsk) ((xsk)->uinput_device)

#define xsk_get_key_press_start_time(xsk) ((xsk)->key_press_start_time)
#define xsk_set_key_press_start_time(xsk, time) \
  ((xsk)->key_press_start_time = (time))

#define xsk_get_uinput_last_event_type(xsk) ((xsk)->uinput_last_event_type)
#define xsk_set_uinput_last_event_type(xsk, type) \
  ((xsk)->uinput_last_event_type = (type))

#define xsk_is_valid_key_code(key_code) \
  ((key_code) > 0 && (key_code) < G_MAXUINT8)

#endif /* _X_SET_KEYS_H */
