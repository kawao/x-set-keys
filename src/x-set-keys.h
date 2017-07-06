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

typedef struct _XSetKeys {
  Display *display;
  gpointer keyboard_device;
  gpointer uinput_device;
  guchar key_state[256];
} XSetKeys;

gboolean xsk_initialize(XSetKeys *xsk);
gboolean xsk_start(XSetKeys *xsk, const gchar *device_filepath);
void xsk_finalize(XSetKeys *xsk);

#define xsk_set_keyboard_device(xsk, kd) ((xsk)->keyboard_device = (kd))
#define xsk_get_keyboard_device(xsk) ((xsk)->keyboard_device)
#define xsk_set_uinput_device(xsk, ud) ((xsk)->uinput_device = (ud))
#define xsk_get_uinput_device(xsk) ((xsk)->uinput_device)

#endif /* _X_SET_KEYS_H */
