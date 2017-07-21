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

#ifndef _KEY_COMBINATION_H
#define _KEY_COMBINATION_H

#include <X11/Xlib.h>
#include <glib.h>

#include "key-information.h"

typedef union _KeyCombination {
  guint16 i;
  struct __KeyCombination {
    guint8 key_code;
    guint8 modifiers;
  } s;
} KeyCombination;

#define key_combination_set_value(kc, code, mods)       \
  ((kc).s.key_code = (code), (kc).s.modifiers = (mods))
#define key_combination_is_null(kc) (!(kc).i)
#define key_combination_compare(kc1, kc2) ((kc1).i - (kc2).i)

typedef GArray KeyCombinationArray;

#define key_combination_array_new(reserved_size)                        \
  g_array_sized_new(FALSE, FALSE, sizeof (KeyCombination), (reserved_size))
#define key_combination_array_free(array) g_array_free((array), TRUE)
#define key_combination_array_add(array, kc) g_array_append_val((array), (kc))
#define key_combination_array_clear(array) g_array_set_size((array), 0)
#define key_combination_array_get_length(array) (array)->len
#define key_combination_array_get_at(array, index)  \
  g_array_index((array), KeyCombination, (index))

#endif /* _KEY_COMBINATION_H */
