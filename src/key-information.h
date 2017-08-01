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
#include "key-code-array.h"

typedef enum KIModifier_ {
  KI_MODIFIER_ALT = 0,
  KI_MODIFIER_CONTROL = 1,
  KI_MODIFIER_HYPER = 2,
  KI_MODIFIER_META = 3,
  KI_MODIFIER_SHIFT = 4,
  KI_MODIFIER_SUPER = 5,
#define KI_NUM_MODIFIER (KI_MODIFIER_SUPER+1)
  KI_MODIFIER_OTHER = 6
} KIModifier;

#define KI_KIND_MODIFIER_OTHER (1 << KI_MODIFIER_OTHER)
#define KI_KIND_CURSOR (KI_KIND_MODIFIER_OTHER + 1)

typedef struct KeyInformation_ {
  KeyCode modifier_key_code[KI_NUM_MODIFIER];
  guchar modifier_mask_or_key_kind[G_MAXUINT8];
} KeyInformation;

void ki_initialize(Display *display, KeyInformation *key_info);

KeyCombination
ki_pressing_keys_to_key_combination(const KeyInformation *key_info,
                                    KeyCode key_code,
                                    const KeyCodeArray *pressing_keys);

KeyCombination ki_string_to_key_combination(Display *display,
                                            const KeyInformation *key_info,
                                            const gchar *string);

KeyCodeArray *ki_string_to_key_code_array(Display *display,
                                          const KeyInformation *key_info,
                                          const gchar *string);

gboolean ki_contains_modifier(const KeyInformation *key_info,
                              const KeyCodeArray *keys,
                              KIModifier modifier);

#define ki_is_modifier(key_info, key_code)                              \
  ((key_info)->modifier_mask_or_key_kind[key_code] &&                   \
   (key_info)->modifier_mask_or_key_kind[key_code] <= KI_KIND_MODIFIER_OTHER)

#define ki_is_regular_modifier(key_info, key_code)                      \
  ((key_info)->modifier_mask_or_key_kind[key_code] &&                   \
   (key_info)->modifier_mask_or_key_kind[key_code] < KI_KIND_MODIFIER_OTHER)

#define ki_is_cursor(key_info, key_code)                                \
  ((key_info)->modifier_mask_or_key_kind[key_code] == KI_KIND_CURSOR)

#define ki_get_modifier_key_code(key_info, modifier)    \
  ((key_info)->modifier_key_code[modifier])

#define ki_is_valid_key_code(key_code)          \
  ((key_code) > 0 && (key_code) < G_MAXUINT8)

#endif /* _KEY_INFORMATION_H */
