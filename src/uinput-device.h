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

#ifndef _UINPUT_DEVICE_H
#define _UINPUT_DEVICE_H

#include <linux/input.h>

#include "x-set-keys.h"
#include "device.h"

typedef struct UInputDevice_ {
  Device device;
  KeyCodeArray *pressing_keys;
  guint16 last_event_type;
} UInputDevice;

UInputDevice *ud_initialize(XSetKeys *xsk);
void ud_finalize(XSetKeys *xsk);

gboolean ud_send_key_event(XSetKeys *xsk,
                           KeyCode key_cord,
                           gboolean is_press,
                           gboolean is_temporary);
gboolean ud_send_event(XSetKeys *xsk, struct input_event *event);

#define ud_get_pressing_keys(xsk)               \
  (xsk_get_uinput_device(xsk)->pressing_keys)
#define ud_is_key_pressed(xsk, key_code)                            \
  key_code_array_contains(ud_get_pressing_keys(xsk), (key_code))

#endif  /* _UINPUT_DEVICE_H */
