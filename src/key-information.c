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

#include "key-information.h"

void ki_initialize(Display *display, KeyInformation *key_info)
{
}

void ki_finalize(KeyInformation *key_info)
{
}

void ki_get_key_combination(const KeyInformation *key_info,
               KeyCode key_code,
               const guchar kaymap[],
               KeyCombination *result)
{
}

gboolean ki_string_to_key_combination(Display *display,
                                      const KeyInformation *key_info,
                                      const char *string,
                                      KeyCombination *result)
{
  return FALSE;
}

gint ki_compare_key_code(gconstpointer a, gconstpointer b, gpointer user_data)
{
  const KeyCode *kc1 = a;
  const KeyCode *kc2 = b;

  return *kc1 - *kc2;
}
