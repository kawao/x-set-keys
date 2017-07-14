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

#ifndef _KEY_INFORMATION_H
#define _KEY_INFORMATION_H

#include <X11/Xlib.h>
#include <glib.h>

#include "key-combination.h"

typedef GArray KI_KeyCodeArray;

typedef GTree KI_KeyCodeSet;

#define ki_key_code_set_new()                               \
  g_tree_new_full(ki_compare_key_code, NULL, g_free, NULL)
#define ki_key_code_set_free(set) g_tree_destroy(set)
#define ki_key_code_set_insert(set, key_code)                   \
  g_tree_insert((set), g_memdup(&(key_code), sizeof (KeyCode)), (gpointer)TRUE)
#define ki_key_code_set_contains(set, key_code) \
  g_tree_lookup((set), &(key_code))

typedef struct _KeyInformation {
  KI_KeyCodeArray *modifier_keys[KC_NUM_MODIFIER];
  KI_KeyCodeSet *all_modifier_keys;
  KI_KeyCodeSet *cursor_keys;
} KeyInformation;

void ki_initialize(Display *display, KeyInformation *key_info);
void ki_finalize(KeyInformation *key_info);

KeyCombination ki_new_key_combination(const KeyInformation *key_info,
                                      KeyCode key_code,
                                      const guchar kaymap[]);
KeyCombination ki_string_to_key_combination(Display *display,
                                            const KeyInformation *key_info,
                                            const char *string);

#define ki_is_modifier(key_info, key_code)                              \
  ki_key_code_set_contains((key_info)->all_modifier_keys, (key_code))

#define ki_is_corsor(key_info, key_code)                        \
  ki_key_code_set_contains((key_info)->cursor_keys, (key_code))

gint ki_compare_key_code(gconstpointer a, gconstpointer b, gpointer user_data);

#endif /* _KEY_INFORMATION_H */
