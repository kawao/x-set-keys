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

#ifndef _ACTION_H
#define _ACTION_H

#include "key-combination.h"
#include "key-code-array.h"

struct XSetKeys_;

typedef GTree ActionList;

typedef enum ActionType_ {
  ACTION_TYPE_KEY_EVENTS,
  ACTION_TYPE_MULTI_STROKE,
  ACTION_TYPE_SELECTION,
  ACTION_TYPE_START,
  ACTION_TYPE_STOP
} ActionType;

/**
 * run - callback
 * free_data - callback that free action->data.key_arrays for normal keys.
 *   Used only in action_list_add_key_action and action_list_add_select_action.
 * data
 *   key_arrays - output_keys for final or single key action
 *   action_list - chain of Actions objects with callback
 */
typedef struct Action_ {
  ActionType type;
  gboolean (*run)(struct XSetKeys_ *xsk, const struct Action_ *action);
  void (*free_data)(struct Action_ *action);
  union ActionData_ {
    KeyCodeArrayArray *key_arrays;
    ActionList *action_list;
  } data;
} Action;

ActionList *action_list_new();
void action_list_free(ActionList *action_list);
gboolean action_list_add_key_action(ActionList *actions_list,
                                    const KeyCombinationArray *input_keys,
                                    KeyCodeArrayArray *output_keys);
gboolean action_list_add_select_action(ActionList *actions_list,
                                       const KeyCombinationArray *input_keys);
gboolean action_list_add_startstop_action(ActionList *actions_list,
                                          const KeyCombinationArray *input_keys,
                                          gboolean is_start);
gint action_list_get_length(const ActionList *action_list);
const Action *action_list_lookup(const ActionList *action_list,
                                 KeyCombination key_combination);

#endif /* _ACTION_H */
