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
#include "x-set-keys.h"

#define _list_new()                                                     \
  g_tree_new_full(_compare_key_combination, NULL, g_free, _free_action)
#define _list_free(list) g_tree_destroy(list)
#define _list_insert(list, key_combination, action)                     \
  g_tree_insert((list),                                                 \
                g_memdup(&(key_combination), sizeof (KeyCombination)),   \
                (action))
#define _list_lookup(list, key_combination) \
  g_tree_lookup((list), &(key_combination))

static void _free_action(gpointer action);
static gint _compare_key_combination(gconstpointer a,
                                     gconstpointer b,
                                     gpointer user_data);
static gboolean _add_action(ActionList *action_list,
                            const KeyCombination input_keys[],
                            guint num_input_keys,
                            Action *action);
static gboolean _send_key_events(XSetKeys *xsk, const Action *action);
static void _free_key_arrays(Action *action);
static gboolean _set_current_actions(XSetKeys *xsk, const Action *action);
static void _free_action_list(Action *action);

ActionList *action_list_new()
{
  return _list_new();
}

void action_list_free(ActionList *action_list)
{
  return _list_free(action_list);
}

gboolean action_list_add_key_action(ActionList *actions_list,
                                    const KeyCombinationArray *input_keys,
                                    KeyCodeArrayArray *output_keys)
{
  Action *action;

  action = g_new(Action, 1);
  action->type = ACTION_TYPE_KEY_EVENTS;
  action->run = _send_key_events;
  action->free_data = _free_key_arrays;
  action->data.key_arrays = key_code_array_array_deprive(output_keys);
  if (!_add_action(actions_list,
                   &key_combination_array_get_at(input_keys, 0),
                   key_combination_array_get_length(input_keys),
                   action)) {
    _free_action(action);
    return FALSE;
  }
  return TRUE;
}

const Action *action_list_lookup(const ActionList *action_list,
                                 KeyCombination key_combination)
{
  return _list_lookup((ActionList *)action_list, key_combination);
}

static void _free_action(gpointer action_)
{
  Action *action = action_;

  action->free_data(action);
  g_free(action);
}

static gint _compare_key_combination(gconstpointer a,
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
    if (_list_lookup(action_list, input_keys)) {
      g_critical("Duplicate input");
      return FALSE;
    }
    _list_insert(action_list, *input_keys, action);
  } else {
    Action *parent_action = _list_lookup(action_list, *input_keys);

    if (!parent_action) {
      parent_action = g_new(Action, 1);
      parent_action->type = ACTION_TYPE_MULTI_STROKE;
      parent_action->run = _set_current_actions;
      parent_action->free_data = _free_action_list;
      parent_action->data.action_list = _list_new();
      _list_insert(action_list, *input_keys, parent_action);
    } else if (parent_action->type != ACTION_TYPE_MULTI_STROKE) {
      g_critical("Duplicate input");
      return FALSE;
    }
    if (!_add_action(parent_action->data.action_list,
                     input_keys + 1,
                     num_input_keys - 1,
                     action)) {
      return FALSE;
    }
  }
  return TRUE;
}

static gboolean _send_key_events(XSetKeys *xsk, const Action *action)
{
  const KeyCodeArrayArray *key_arrays = action->data.key_arrays;

  if (!key_code_array_array_get_length(key_arrays)) {
    debug_print("Empty key action");
    return TRUE;
  }
  debug_print("Executing key action, number of key combinations=%d",
              key_code_array_array_get_length(key_arrays));
  return xsk_send_key_events(xsk, key_arrays);
}

static void _free_key_arrays(Action *action)
{
  key_code_array_array_free(action->data.key_arrays);
}

static gboolean _set_current_actions(XSetKeys *xsk, const Action *action)
{
  debug_print("Multi stroke action");
  xsk_set_current_actions(xsk, action->data.action_list);
  return TRUE;
}

static void _free_action_list(Action *action)
{
  _list_free(action->data.action_list);
}
