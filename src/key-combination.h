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

typedef enum _KCModifier {
#define KC_MODIFIER_UNKNOWN -1
  KC_MODIFIER_ALT = 0,
  KC_MODIFIER_CONTROL = 1,
  KC_MODIFIER_HYPER = 2,
  KC_MODIFIER_META = 3,
  KC_MODIFIER_SHIFT = 4,
  KC_MODIFIER_SUPER = 5,
#define KC_NUM_MODIFIER (KC_MODIFIER_SUPER+1)
} KCModifier;

#define kc_compare(kc1, kc2) ((kc1)->i - (kc2)->i)

#endif /* _KEY_COMBINATION_H */
