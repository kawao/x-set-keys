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
#include "action.h"
#include "key-combination.h"

static gboolean _add_action(ActionList *action_list,
                            const KeyCombination input_keys[],
                            guint num_input_keys,
                            Action *action);
static gboolean _send_key_events(XSetKeys *xsk, gconstpointer data);
static void _free_key_code_array_array(gpointer data);

gboolean action_add_key_action(XSetKeys *xsk,
                               const KeyCombinationArray *input_keys,
                               KeyCodeArrayArray *output_keys)
{
  Action *action;

  action = g_new(Action, 1);
  action->type = ACTION_TYPE_KEY_EVENTS;
  action->run = _send_key_events;
  action->free_data = _free_key_code_array_array;
  action->data = key_code_array_array_deprive(output_keys);
  if (!_add_action(xsk_get_root_actions(xsk),
                   &key_combination_array_get_at(input_keys, 0),
                   key_combination_array_get_length(input_keys),
                   action)) {
    action_free(action);
    return FALSE;
  }
  return TRUE;
}

void action_free(gpointer action_)
{
  Action *action = action_;

  action->free_data(action->data);
  g_free(action);
}

gint action_compare_key_combination(gconstpointer a,
                                    gconstpointer b,
                                    gpointer user_data)
{
  return key_combination_compare(*(const KeyCombination *)a,
                                 *(const KeyCombination *)b);
}

static gboolean _add_action(ActionList *action_list,
                            const KeyCombination input_keys[],
                            guint num_input_keys,
                            Action *action)
{
  if (num_input_keys == 1) {
    if (action_list_lookup(action_list, input_keys)) {
      g_critical("Duplicate input");
      return FALSE;
    }
    action_list_insert(action_list, input_keys, action);
  } else {
    g_warn_if_reached();
    return TRUE;
    /*
    nested_action = actionlist_lookup(action_list, input_keys);
    if (!nested_action) {
      nested_action = create_nested_action();
      actionlist_insert(action_list, input_keys, nested_action);
    } else if (!nested_action.type != NESTED) {
      g_critical("duplicate input");
      return FALSE;
    }
    if (!add_action(nested_action->action_list,
                    &input_keys[1],
                    num_input - 1,
                    action) {
       return FALSE;
    }
     */
  }
  return TRUE;
}

static gboolean _send_key_events(XSetKeys *xsk, gconstpointer data)
{
  const KeyCodeArrayArray *key_arrays = data;

  if (!key_code_array_array_get_length(key_arrays)) {
    debug_print("Empty key action");
    return TRUE;
  }
  debug_print("Executing key action, number of key combinations=%d",
              key_code_array_array_get_length(key_arrays));
  return xsk_send_key_events(xsk, key_arrays);
}

static void _free_key_code_array_array(gpointer data)
{
  key_code_array_array_free(data);
}
