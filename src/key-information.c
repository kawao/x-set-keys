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
static KI_KeyCodeArray *_new_modifier_keys(const XModifierKeymap *modmap,
                                           gint row);
static KIModifier _get_modifier_for_key_code(Display *display,
                                             KeyCode key_code);
static KIModifier _get_modifier_for_key_sym(KeySym key_sym);

void ki_initialize(Display *display, KeyInformation *key_info)
{
  key_info->all_modifier_keys = ki_key_code_set_new();
  _initialize_modifier_info(display, key_info);

  key_info->cursor_keys = ki_key_code_set_new();
}

void ki_finalize(KeyInformation *key_info)
{
  gint index;

  for (index = 0; index < KI_NUM_MODIFIER; index++) {
    if (key_info->modifier_keys[index]) {
      ki_key_code_array_free(key_info->modifier_keys[index]);
    }
  }
  ki_key_code_set_free(key_info->all_modifier_keys);
  ki_key_code_set_free(key_info->cursor_keys);
}

KeyCombination ki_keymap_to_key_combination(const KeyInformation *key_info,
                                            KeyCode key_code,
                                            const guchar kaymap[])
{
  KeyCombination result = { 0 };
  return result;
}

KeyCombination ki_string_to_key_combination(Display *display,
                                            const KeyInformation *key_info,
                                            const char *string)
{
  KeyCombination result = { 0 };
  return result;
}

gint ki_compare_key_code(gconstpointer a, gconstpointer b, gpointer user_data)
{
  return *(const KeyCode *)a - *(const KeyCode *)b;
}

static void _initialize_modifier_info(Display *display,
                                      KeyInformation *key_info)
{
  gint row;
  gint col;
  XModifierKeymap *modmap = XGetModifierMapping(display);

  key_info->modifier_keys[KI_MODIFIER_SHIFT] = _new_modifier_keys(modmap, 0);
  if (!key_info->modifier_keys[KI_MODIFIER_SHIFT]) {
    g_critical("Key is not defined for modifier: %s",
               _modifier_names[KI_MODIFIER_SHIFT]);
  }
  key_info->modifier_keys[KI_MODIFIER_CONTROL] = _new_modifier_keys(modmap, 2);
  if (!key_info->modifier_keys[KI_MODIFIER_CONTROL]) {
    g_critical("Key is not defined for modifier: %s",
               _modifier_names[KI_MODIFIER_CONTROL]);
  }

  for (row = 0; row < 8; row++) {
    for (col = 0; col < modmap->max_keypermod; col++) {
      gint modifier_index;
      KeyCode key_code = modmap->modifiermap[row * modmap->max_keypermod + col];
      if (!key_code) {
        continue;
      }
      ki_key_code_set_insert(key_info->all_modifier_keys, key_code);
      if (row < 3) {
        continue;
      }

      modifier_index = _get_modifier_for_key_code(display, key_code);
      if (modifier_index == KI_MODIFIER_UNKNOWN) {
        continue;
      }
      if (key_info->modifier_keys[modifier_index]) {
        g_warning("%s corresponds to multi modifiers. Ignore row=%d",
                  _modifier_names[modifier_index],
                  row);
        continue;
      }
      key_info->modifier_keys[modifier_index] = _new_modifier_keys(modmap, row);
      if (!key_info->modifier_keys[modifier_index]) {
        g_critical("Key is not defined for modifier: %s",
                   _modifier_names[modifier_index]);
        continue;
      }
      debug_print("found modifier=%s row=%d",
                  _modifier_names[modifier_index],
                  row);
      break;
    }
  }

  XFreeModifiermap(modmap);
}

static KI_KeyCodeArray *_new_modifier_keys(const XModifierKeymap *modmap,
                                           gint row)
{
  KI_KeyCodeArray *result;
  gint col;

  result = ki_key_code_array_new(modmap->max_keypermod);

  for (col = 0; col < modmap->max_keypermod; col++) {
    KeyCode key_code = modmap->modifiermap[row * modmap->max_keypermod + col];
    if (key_code) {
      ki_key_code_array_append(result, key_code);
    }
  }

  if (!ki_key_code_array_length(result)) {
    ki_key_code_array_free(result);
    return NULL;
  }
  return result;
}

static KIModifier _get_modifier_for_key_code(Display *display, KeyCode key_code)
{
  gint modifier_index = KI_MODIFIER_UNKNOWN;
  KeySym *key_syms;
  gint num_key_sym;
  gint sym_index;

  key_syms = XGetKeyboardMapping(display, key_code, 1, &num_key_sym);
  if (!key_syms) {
    return modifier_index;
  }
  for (sym_index = 0; sym_index < num_key_sym; sym_index++) {
    if (key_syms[sym_index] != NoSymbol) {
      modifier_index = _get_modifier_for_key_sym(key_syms[sym_index]);
      if (modifier_index != KI_MODIFIER_UNKNOWN) {
        break;
      }
    }
  }
  XFree(key_syms);
  return modifier_index;
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
  return KI_MODIFIER_UNKNOWN;
}
