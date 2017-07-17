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

Device *ud_initialize(XSetKeys *xsk);
void ud_finalize(XSetKeys *xsk);

gboolean ud_send_key_event(XSetKeys *xsk, KeyCode key_cord, gboolean is_press);
gboolean ud_send_event(XSetKeys *xsk, struct input_event *event);

#define ud_is_key_pressed(xsk, key_code)                                \
  key_code_array_contains(xsk_get_uinput_pressing_keys(xsk), (key_code))

#endif  /* _UINPUT_DEVICE_H */
