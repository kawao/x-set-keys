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

typedef struct _KeyCombination {
  KeyCode key_code;
  guint modifiers;
} KeyCombination;

#define kc_new(key_code, modifiers) { (key_code), (modifiers) }
#define kc_assign(kc, code, mods)                   \
  (kc)->key_code = (code), (kc)->modifiers = (mods)

#define kc_serialize(kc) ((kc)->key_code | ((kc)->modifiers << 16))

#endif /* _KEY_COMBINATION_H */
