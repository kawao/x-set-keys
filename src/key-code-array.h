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

#ifndef _KEY_CODE_ARRAY_H
#define _KEY_CODE_ARRAY_H

#include <X11/Xlib.h>
#include <glib.h>

typedef GArray KeyCodeArray;

#define key_code_array_new(reserved_size)                               \
  g_array_sized_new(TRUE, FALSE, sizeof (KeyCode), (reserved_size))
#define key_code_array_add(array, key_code)             \
  (key_code_array_contains((array), (key_code))         \
   ? FALSE : g_array_append_val((array), (key_code)))
#define key_code_array_get_at(array, index)   \
  g_array_index((array), KeyCode, (index))
#define key_code_array_get_length(array) ((array)->len)

void key_code_array_free(gpointer array);
gboolean key_code_array_remove(KeyCodeArray *array, KeyCode key_code);
gboolean key_code_array_contains(const KeyCodeArray *array, KeyCode key_code);

typedef GPtrArray KeyCodeArrayArray;

#define key_code_array_array_new(reserved_size)                 \
  g_ptr_array_new_full((reserved_size), key_code_array_free)
#define key_code_array_array_free(array_array)  \
  g_ptr_array_free((array_array), TRUE)
#define key_code_array_array_add(array_array, array)    \
  g_ptr_array_add((array_array), (array))
#define key_code_array_array_clear(array_array) \
  g_ptr_array_set_size((array_array), 0);
#define key_code_array_array_get_length(array_array) ((array_array)->len)
#define key_code_array_array_get_at(array_array, index) \
  ((KeyCodeArray *)(g_ptr_array_index((array_array), (index))))
#define key_code_array_array_foreach(array_array, func, user_data)    \
  g_ptr_array_foreach((array_array), (func), (user_data))

KeyCodeArrayArray *key_code_array_array_deprive(KeyCodeArrayArray *from);

#endif /* _KEY_CODE_ARRAY_H */
