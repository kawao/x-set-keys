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

#ifndef _ACTION_H
#define _ACTION_H

#include "x-set-keys.h"

typedef struct _Action {
  gboolean (*run)(XSetKeys *xsk, gpointer data);
  void (*free_data)(gpointer data);
  gpointer data;
} Action;

void action_free(gpointer action);
gint action_compare_kc(gconstpointer a, gconstpointer b, gpointer user_data);

#define action_list_new()                                       \
  g_tree_new_full(action_compare_kc, NULL, g_free, action_free)
#define action_list_free(actions) g_tree_destroy(actions)
#define action_list_lookup(actions, kc) g_tree_lookup((actions), (kc))

#endif /* _ACTION_H */
