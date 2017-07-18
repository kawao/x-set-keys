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

#include <X11/keysym.h>

#include "common.h"
#include "key-information.h"

#define _MODIFIER_PUNCTUATION '-'

static const char *_modifier_names[] = {
  "alt",
  "control",
  "hyper",
  "meta",
  "shift",
  "super",
};

static void _initialize_modifier_info(Display *display,
                                      KeyInformation *key_info);
static KIModifier _get_modifier_for_modmap_row(Display *display,
                                               KeyInformation *key_info,
                                               XModifierKeymap *modmap,
                                               gint row);
static KIModifier _get_modifier_for_key_code(Display *display,
                                             KeyCode key_code);
static KIModifier _get_modifier_for_key_sym(KeySym key_sym);
static KIModifier _get_modifier_for_char(char modifier_char);
static void _set_modifier_info(KeyInformation *key_info,
                               KIModifier modifier,
                               XModifierKeymap *modmap,
                               gint row);
static void _initialize_cursor_info(Display *display, KeyInformation *key_info);

void ki_initialize(Display *display, KeyInformation *key_info)
{
  _initialize_modifier_info(display, key_info);
  _initialize_cursor_info(display, key_info);
}

void ki_finalize(KeyInformation *key_info)
{
}

KeyCombination
ki_pressing_keys_to_key_combination(const KeyInformation *key_info,
                                    KeyCode key_code,
                                    const KeyCodeArray *pressing_keys)
{
  KeyCombination result;
  KeyCode *pointer;
  guchar modifiers = 0;
  guchar mask;

  for (pointer = &key_code_array_get_data(pressing_keys, 0);
       *pointer;
       pointer++) {
    if (*pointer == key_code) {
      continue;
    }
    mask = key_info->modifier_mask_or_key_kind[*pointer];
    if (!mask || mask >= KI_KIND_MODIFIER_OTHER) {
      continue;
    }
    modifiers |= mask;
  }

  key_combination_set_value(result, key_code, modifiers);
  return result;
}

KeyCombination ki_string_to_key_combination(Display *display,
                                            const KeyInformation *key_info,
                                            const char *string)
{
  KeyCombination result;
  KeyCode key_code;
  guchar masks = 0;
  const char *pointer = string;
  gint length = strlen(string);
  KeySym key_sym;

  while (length > 2 && pointer[1] == _MODIFIER_PUNCTUATION) {
    KIModifier modifier = _get_modifier_for_char(*pointer);
    if (modifier == KI_MODIFIER_OTHER) {
      g_critical("Illegal modifier character: %c", *pointer);
      goto ERROR;
    }
    if (!key_info->modifier_key_code[modifier]) {
      if (modifier == KI_MODIFIER_ALT &&
          key_info->modifier_key_code[KI_MODIFIER_META]) {
        modifier = KI_MODIFIER_META;
      } else if (modifier == KI_MODIFIER_META &&
                 key_info->modifier_key_code[KI_MODIFIER_ALT]) {
        modifier = KI_MODIFIER_ALT;
      } else {
        g_critical("Modifier '%s' is not defined on your system",
                   _modifier_names[modifier]);
        goto ERROR;
      }
    }
    masks |= (1 << modifier);
    pointer += 2;
    length -= 2;
  }

  key_sym = XStringToKeysym(pointer);
  if (key_sym == NoSymbol) {
    g_critical("Invalid key string: %s", pointer);
    goto ERROR;
  }
  key_code = XKeysymToKeycode(display, key_sym);
  if (!key_code) {
    g_critical("Key '%s' is not defined on your system", pointer);
    goto ERROR;
  }

  key_combination_set_value(result, key_code, masks);
  return result;

 ERROR:
  key_combination_set_value(result, 0, 0);
  return result;
}

KeyCodeArray *ki_string_to_key_code_array(Display *display,
                                          const KeyInformation *key_info,
                                          const char *string)
{
  KeyCodeArray *result = key_code_array_new(4);
  const char *pointer = string;
  gint length = strlen(string);
  KeyCode key_code;
  KeySym key_sym;

  while (length > 2 && pointer[1] == _MODIFIER_PUNCTUATION) {
    KIModifier modifier = _get_modifier_for_char(*pointer);
    if (modifier == KI_MODIFIER_OTHER) {
      g_critical("Illegal modifier character: %c", *pointer);
      goto ERROR;
    }
    if (!key_info->modifier_key_code[modifier]) {
      if (modifier == KI_MODIFIER_ALT &&
          key_info->modifier_key_code[KI_MODIFIER_META]) {
        modifier = KI_MODIFIER_META;
      } else if (modifier == KI_MODIFIER_META &&
                 key_info->modifier_key_code[KI_MODIFIER_ALT]) {
        modifier = KI_MODIFIER_ALT;
      } else {
        g_critical("Modifier '%s' is not defined on your system",
                   _modifier_names[modifier]);
        goto ERROR;
      }
    }
    key_code_array_append(result, key_info->modifier_key_code[modifier]);
    pointer += 2;
    length -= 2;
  }

  key_sym = XStringToKeysym(pointer);
  if (key_sym == NoSymbol) {
    g_critical("Invalid key string: %s", pointer);
    goto ERROR;
  }
  key_code = XKeysymToKeycode(display, key_sym);
  if (!key_code) {
    g_critical("Key '%s' is not defined on your system", pointer);
    goto ERROR;
  }

  key_code_array_append(result, key_code);
  return result;

 ERROR:
  key_code_array_free(result);
  return NULL;
}

gboolean ki_contains_modifier(const KeyInformation *key_info,
                              const KeyCodeArray *keys,
                              KIModifier modifier)
{
  KeyCode *pointer;

  for (pointer = &key_code_array_get_data(keys, 0); *pointer; pointer++) {
    if (key_info->modifier_mask_or_key_kind[*pointer] == (1 << modifier)) {
      return TRUE;
    }
  }
  return FALSE;
}

static void _initialize_modifier_info(Display *display,
                                      KeyInformation *key_info)
{
  gint row;
  XModifierKeymap *modmap = XGetModifierMapping(display);

  _set_modifier_info(key_info, KI_MODIFIER_SHIFT, modmap, 0);
  _set_modifier_info(key_info, KI_MODIFIER_CONTROL, modmap, 2);

  for (row = 3; row < 8; row++) {
    KIModifier modifier = _get_modifier_for_modmap_row(display,
                                                       key_info,
                                                       modmap,
                                                       row);
    _set_modifier_info(key_info, modifier, modmap, row);
  }

  XFreeModifiermap(modmap);
}

static KIModifier _get_modifier_for_modmap_row(Display *display,
                                               KeyInformation *key_info,
                                               XModifierKeymap *modmap,
                                               gint row)
{
  KIModifier modifier;
  gint col;

  for (col = 0; col < modmap->max_keypermod; col++) {
    KeyCode key_code = modmap->modifiermap[row * modmap->max_keypermod + col];
    if (!key_code) {
      continue;
    }
    modifier = _get_modifier_for_key_code(display, key_code);
    if (modifier == KI_MODIFIER_OTHER) {
      continue;
    }
    if (key_info->modifier_key_code[modifier]) {
      g_warning("'%s' corresponds to multi modifiers, row=%d",
                _modifier_names[modifier],
                row);
      continue;
    }
    return modifier;
  }
  return KI_MODIFIER_OTHER;
}

static KIModifier _get_modifier_for_key_code(Display *display, KeyCode key_code)
{
  KIModifier modifier = KI_MODIFIER_OTHER;
  KeySym *key_syms;
  gint num_key_sym;
  gint index;

  key_syms = XGetKeyboardMapping(display, key_code, 1, &num_key_sym);
  if (!key_syms) {
    return modifier;
  }
  for (index = 0; index < num_key_sym; index++) {
    if (key_syms[index] != NoSymbol) {
      modifier = _get_modifier_for_key_sym(key_syms[index]);
      if (modifier != KI_MODIFIER_OTHER) {
        break;
      }
    }
  }
  XFree(key_syms);
  return modifier;
}

static KIModifier _get_modifier_for_key_sym(KeySym key_sym)
{
  switch (key_sym) {
  case XK_Meta_L:
  case XK_Meta_R:
    return KI_MODIFIER_META;
  case XK_Alt_L:
  case XK_Alt_R:
    return KI_MODIFIER_ALT;
  case XK_Hyper_L:
  case XK_Hyper_R:
    return KI_MODIFIER_HYPER;
  case XK_Super_L:
  case XK_Super_R:
    return KI_MODIFIER_SUPER;
  }
  return KI_MODIFIER_OTHER;
}

static KIModifier _get_modifier_for_char(char modifier_char)
{
    switch (modifier_char) {
    case 'A':
    case 'a':
      return KI_MODIFIER_ALT;
    case 'C':
    case 'c':
      return KI_MODIFIER_CONTROL;
    case 'H':
    case 'h':
      return KI_MODIFIER_HYPER;
    case 'M':
    case 'm':
      return KI_MODIFIER_META;
    case 'S':
      return KI_MODIFIER_SHIFT;
    case 's':
      return KI_MODIFIER_SUPER;
    }
    return KI_MODIFIER_OTHER;
}

static void _set_modifier_info(KeyInformation *key_info,
                               KIModifier modifier,
                               XModifierKeymap *modmap,
                               gint row)
{
  gint col;

  for (col = 0; col < modmap->max_keypermod; col++) {
    KeyCode key_code = modmap->modifiermap[row * modmap->max_keypermod + col];
    if (!key_code) {
      continue;
    }
    key_info->modifier_mask_or_key_kind[key_code] = (1 << modifier);
    if (modifier != KI_MODIFIER_OTHER &&
        !key_info->modifier_key_code[modifier]) {
      key_info->modifier_key_code[modifier] = key_code;
    }
  }
  debug_print("Set modifier=%s modmap-row=%d", _modifier_names[modifier], row);
}

static void _initialize_cursor_info(Display *display, KeyInformation *key_info)
{
  KeySym key_syms[] = {
    XK_Home,
    XK_Left,
    XK_Up,
    XK_Right,
    XK_Down,
    XK_Prior,
    XK_Page_Up,
    XK_Next,
    XK_Page_Down,
    XK_End,
    XK_Begin,
    NoSymbol
  };
  KeySym *pointer;

  for (pointer = key_syms; *pointer != NoSymbol; pointer++) {
    KeyCode key_code = XKeysymToKeycode(display, *pointer);
    if (!key_code) {
      key_info->modifier_mask_or_key_kind[key_code] = KI_KIND_CURSOR;
    }
  }
}
