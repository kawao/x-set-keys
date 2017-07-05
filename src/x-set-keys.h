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
  gpointer kd_source;
  gpointer ui_source;
  guchar key_state[256];
} XSetKeys;

gboolean xsk_initialize(XSetKeys *xsk);
gboolean xsk_start(XSetKeys *xsk, const gchar *device_filepath);
void xsk_finalize(XSetKeys *xsk);

#define xsk_set_kd_source(xsk, source) ((xsk)->kd_source = (source))
#define xsk_get_kd_source(xsk) ((xsk)->kd_source)
#define xsk_set_ui_source(xsk, source) ((xsk)->ui_source = (source))
#define xsk_get_ui_source(xsk) ((xsk)->ui_source)

#endif /* _X_SET_KEYS_H */
