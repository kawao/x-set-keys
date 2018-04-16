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

#ifndef _KEYBOARD_DEVICE_H
#define _KEYBOARD_DEVICE_H

#include <X11/XKBlib.h>
#include <linux/input.h>

#include "x-set-keys.h"
#include "device.h"

typedef struct KeyboardDevice_ {
  Device device;
  KeyCodeArray *pressing_keys;
  struct timeval press_start_time;
  XkbDescPtr xkb;
} KeyboardDevice;

KeyboardDevice *kd_initialize(XSetKeys *xsk, const gchar *device_filepath);
void kd_finalize(XSetKeys *xsk);

#define KD_EV_BITS_LENGTH (EV_MAX/8 + 1)
gboolean kd_get_ev_bits(XSetKeys *xsk, guint8 ev_bits[]);

#define KD_KEY_BITS_LENGTH (KEY_MAX/8 + 1)
gboolean kd_get_key_bits(XSetKeys *xsk, guint8 key_bits[]);

#define KD_LED_BITS_LENGTH (LED_MAX/8 + 1)
gboolean kd_get_led_bits(XSetKeys *xsk, guint8 led_bits[]);

#define kd_test_bit(array, bit) ((array)[(bit) / 8] & (1 << ((bit) % 8)))

#define kd_write(xsk, buffer, length)                                   \
  device_write(&xsk_get_keyboard_device(xsk)->device, (buffer), (length))

#define kd_get_pressing_keys(xsk)               \
  (xsk_get_keyboard_device(xsk)->pressing_keys)
#define kd_is_key_pressed(xsk, key_code)                            \
  key_code_array_contains(kd_get_pressing_keys(xsk), (key_code))

#endif  /* _KEYBOARD_DEVICE_H */
