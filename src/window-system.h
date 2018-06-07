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

#ifndef _WINDOW_SYSTEM_H
#define _WINDOW_SYSTEM_H

#include "x-set-keys.h"
#include "device.h"

typedef struct WindowSystem_ {
  Device device;
  gchar **excluded_classes;
  Atom active_window_atom;
  Atom xkb_rules_atom;
  Window focus_window;
  gboolean is_excluded;
} WindowSystem;

WindowSystem *window_system_initialize(XSetKeys *xsk,
                                       gchar *excluded_classes[]);
void window_system_pre_finalize(XSetKeys *xsk);
void window_system_finalize(XSetKeys *xsk, gboolean is_restart);

#define window_system_is_excluded(xsk) (xsk_get_window_system(xsk)->is_excluded)

#endif  /* _WINDOW_SYSTEM_H */
