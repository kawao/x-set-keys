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

#include <stdint.h>
#include <linux/input.h>

#include "x-set-keys.h"

gboolean kd_initialize(XSetKeys *xsk, const gchar *device_filepath);
void kd_finalize(XSetKeys *xsk);

#define KD_EV_BITS_LENGTH (EV_MAX/8 + 1)
gboolean kd_get_ev_bits(XSetKeys *xsk, uint8_t ev_bits[]);

#define KD_KEY_BITS_LENGTH (KEY_MAX/8 + 1)
gboolean kd_get_key_bits(XSetKeys *xsk, uint8_t key_bits[]);

#define kd_test_bit(array, bit) ((array)[(bit) / 8] & (1 << ((bit) % 8)))

gboolean kd_write(XSetKeys *xsk, gconstpointer buffer, gsize length);

#endif  /* _KEYBOARD_DEVICE_H */
