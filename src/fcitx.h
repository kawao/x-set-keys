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

#ifndef _FCITX_H
#define _FCITX_H

#include <gio/gio.h>

#include "x-set-keys.h"

typedef struct Fcitx_ {
  gchar **excluded_input_methods;
  GDBusConnection *connection;
  guint watch_id;
  guint subscription_id;
  gboolean is_excluded;
} Fcitx;

Fcitx *fcitx_initialize(XSetKeys *xsk, gchar *excluded_input_methods[]);
void fcitx_finalize(XSetKeys *xsk);

#define fcitx_is_excluded(xsk)                              \
  (xsk_get_fcitx(xsk) && xsk_get_fcitx(xsk)->is_excluded)

#endif /* _FCITX_H */
