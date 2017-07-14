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

#include "action.h"
#include "key-combination.h"

const Action *action_lookup(XSetKeys *xsk, KeyCode key_code)
{
  KeyCombination kc = ki_new_key_combination(xsk_get_key_information(xsk),
                                             key_code,
                                             xsk_get_keyboard_keymap(xsk));
  return action_list_lookup(xsk_get_current_actions(xsk), &kc);
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
  return kc_compare((const KeyCombination *)a, (const KeyCombination *)b);
}
