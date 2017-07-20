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

#include "common.h"
#include "config.h"

static void _create_key_combination(XSetKeys *xsk, const gchar *string);
static void _create_key_code_array(XSetKeys *xsk, const gchar *string);

gboolean config_load(XSetKeys *xsk, gchar filepath[])
{
  _create_key_combination(xsk, "C-A-Delete");
  _create_key_combination(xsk, "C-B-Delete");
  _create_key_combination(xsk, "C-M-Delete");
  _create_key_combination(xsk, "C-H-Delete");
  _create_key_combination(xsk, "C-A-delete");
  _create_key_combination(xsk, "C-A-Caps_Lock");

  _create_key_code_array(xsk, "C-A-Delete");
  _create_key_code_array(xsk, "C-B-Delete");
  _create_key_code_array(xsk, "C-M-Delete");
  _create_key_code_array(xsk, "C-H-Delete");
  _create_key_code_array(xsk, "C-A-delete");
  _create_key_code_array(xsk, "C-A-Caps_Lock");

  return FALSE;
}

static void _create_key_combination(XSetKeys *xsk, const gchar *string)
{
  KeyCombination kc = ki_string_to_key_combination(xsk_get_display(xsk),
                                                   xsk_get_key_information(xsk),
                                                   string);
  debug_print("%s: key_code=%d modifiers=%x",
              string,
              kc.s.key_code,
              kc.s.modifiers);
}

static void _create_key_code_array(XSetKeys *xsk, const gchar *string)
{
  KeyCodeArray *array =
    ki_string_to_key_code_array(xsk_get_display(xsk),
                                xsk_get_key_information(xsk),
                                string);
  if (array) {
    GString *text = g_string_sized_new(32);
    KeyCode *pointer;

    for (pointer = &key_code_array_get_at(array, 0); *pointer; pointer++) {
      if (pointer != &key_code_array_get_at(array, 0)) {
        g_string_append(text, ", ");
      }
      g_string_append_printf(text, "%d", *pointer);
    }
    debug_print("%s: [%s]", string, text->str);

    g_string_free(text, TRUE);
    key_code_array_free(array);
  } else {
    debug_print("%s: NULL", string);
  }
}
