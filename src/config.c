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
#include "action.h"

static gboolean _load(XSetKeys *xsk,
                      KeyCombinationArray *inputs,
                      KeyCodeArrayArray *outputs);
static KeyCombination _create_key_combination(XSetKeys *xsk,
                                              const gchar *string);
static KeyCodeArray *_create_key_code_array(XSetKeys *xsk, const gchar *string);

gboolean config_load(XSetKeys *xsk, gchar filepath[])
{
  KeyCombinationArray *inputs = key_combination_array_new(6);
  KeyCodeArrayArray *outputs = key_code_array_array_new(6);

  gboolean result = _load(xsk, inputs, outputs);

  key_combination_array_free(inputs);
  key_code_array_array_free(outputs);

  return result;

#if 0
  key_combination_array_add(inputs, kc);
  key_code_array_array_add(outputs, _create_key_code_array(xsk, "Tab"));
  result = action_add_key_action(xsk, inputs, outputs);
  g_warn_if_fail(result);

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
#endif
}

static gboolean _load(XSetKeys *xsk,
                      KeyCombinationArray *inputs,
                      KeyCodeArrayArray *outputs)
{
  struct {
    const gchar *inputs[4];
    const gchar *outputs[4];
  } commands[] = {
    {{ "C-i" }, { "Tab" }},
    {{ "A-i" }, { "S-Tab" }}
  };

  gint index;
  gboolean result;

  for (index = 0; index < array_num(commands); index++) {
    const gchar **pointer;

    key_combination_array_clear(inputs);
    key_code_array_array_clear(outputs);

    for (pointer = commands[index].inputs; *pointer; pointer++) {
      KeyCombination kc = _create_key_combination(xsk, *pointer);
      key_combination_array_add(inputs, kc);
    }

    for (pointer = commands[index].outputs; *pointer; pointer++) {
      key_code_array_array_add(outputs, _create_key_code_array(xsk, *pointer));
    }

    if (is_debug) {
      GString *string = g_string_sized_new(32);

      for (pointer = commands[index].inputs; *pointer; pointer++) {
        g_string_append_c(string, ' ');
        g_string_append(string, *pointer);
      }
      g_string_append(string, " ::");
      for (pointer = commands[index].outputs; *pointer; pointer++) {
        g_string_append_c(string, ' ');
        g_string_append(string, *pointer);
      }
      debug_print("Registering: [%s ]", string->str);
      g_string_free(string, TRUE);
    }

    result = action_list_add_key_action(xsk_get_root_actions(xsk),
                                        inputs,
                                        outputs);
    g_return_val_if_fail(result, FALSE);
  }

  return FALSE;
}

static KeyCombination _create_key_combination(XSetKeys *xsk,
                                              const gchar *string)
{
  KeyCombination kc = ki_string_to_key_combination(xsk_get_display(xsk),
                                                   xsk_get_key_information(xsk),
                                                   string);
#if 0
  debug_print("%s: key_code=%d modifiers=%x",
              string,
              kc.s.key_code,
              kc.s.modifiers);
#endif
  return kc;
}

static KeyCodeArray *_create_key_code_array(XSetKeys *xsk, const gchar *string)
{
  KeyCodeArray *array =
    ki_string_to_key_code_array(xsk_get_display(xsk),
                                xsk_get_key_information(xsk),
                                string);
#if 0
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
  } else {
    debug_print("%s: NULL", string);
  }
#endif
  return array;
}
