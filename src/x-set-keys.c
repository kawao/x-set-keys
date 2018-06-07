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

#include "common.h"
#include "x-set-keys.h"
#include "window-system.h"
#include "fcitx.h"
#include "action.h"
#include "keyboard-device.h"
#include "uinput-device.h"

#define _reset_current_actions(xsk)                 \
  ((xsk)->current_actions = (xsk)->root_actions)

static const Action *_lookup_action(XSetKeys *xsk, KeyCode key_code);
static XskResult _key_pressed_on_selection_mode(XSetKeys *xsk,
                                                KeyCode key_code);
static gboolean _send_regular_modifiers_event(XSetKeys *xsk,
                                              const KeyCodeArray *keys,
                                              gboolean is_press);
static gboolean _adds_shift_on_send_keys(XSetKeys *xsk,
                                         const KeyCodeArray *keys);
static gboolean
_adds_shift_on_selection_mode(XSetKeys *xsk,
                              KeyCode key_code,
                              const KeyCodeArray *pressing_keys);
static gboolean _send_key_events(XSetKeys *xsk, const KeyCode *keys);

gboolean xsk_initialize(XSetKeys *xsk, gchar *excluded_classes[])
{
  xsk->display = XOpenDisplay(NULL);
  if (!xsk->display) {
    g_critical("Could not create X11 display");
    return FALSE;
  }
  xsk->window_system = window_system_initialize(xsk, excluded_classes);
  if (!xsk->window_system) {
    return FALSE;
  }
  ki_initialize(xsk->display, &xsk->key_information);
  xsk->root_actions = action_list_new();
  return TRUE;
}

gboolean xsk_start(XSetKeys *xsk,
                   const gchar *device_filepath,
                   gchar *excluded_fcitx_input_methods[])
{
  if (excluded_fcitx_input_methods) {
    xsk->fcitx = fcitx_initialize(xsk, excluded_fcitx_input_methods);
    if (!xsk->fcitx) {
      return FALSE;
    }
  }
  xsk->keyboard_device = kd_initialize(xsk, device_filepath);
  if (!xsk->keyboard_device) {
    return FALSE;
  }
  xsk->uinput_device = ud_initialize(xsk);
  if (!xsk->uinput_device) {
    return FALSE;
  }
  xsk_reset_state(xsk);
  return TRUE;
}

void xsk_finalize(XSetKeys *xsk, gboolean is_restart)
{
  if (xsk->window_system) {
    window_system_pre_finalize(xsk);
  }
  if (xsk->uinput_device) {
    ud_finalize(xsk);
  }
  if (xsk->keyboard_device) {
    kd_finalize(xsk);
  }
  if (xsk->fcitx) {
    fcitx_finalize(xsk);
  }
  if (xsk->root_actions) {
    action_list_free(xsk->root_actions);
  }
  if (xsk->window_system) {
    window_system_finalize(xsk, is_restart);
  }
  if (xsk->display) {
    XCloseDisplay(xsk->display);
  }
}

XskResult xsk_handle_key_press(XSetKeys *xsk, KeyCode key_code)
{
  const Action *action;

  if (xsk_is_excluded(xsk)) {
    return XSK_UNCONSUMED;
  }
  action = _lookup_action(xsk, key_code);
  if (action) {
    _reset_current_actions(xsk);
    return action->run(xsk, action) ? XSK_CONSUMED : XSK_FAILED;
  }
  if (!ki_is_modifier(&xsk->key_information, key_code)) {
    if (xsk->current_actions != xsk->root_actions) {
      _reset_current_actions(xsk);
      g_warning("Key sequence canceled");
    }
    if (xsk->is_selection_mode) {
      return _key_pressed_on_selection_mode(xsk, key_code);
    }
  }
  return XSK_UNCONSUMED;
}

XskResult xsk_handle_key_repeat(XSetKeys *xsk,
                                KeyCode key_code,
                                gboolean is_after_key_repeat_delay)
{
  const Action *action;
  XskResult result;

  if (ud_is_key_pressed(xsk, key_code)) {
    if (xsk_is_excluded(xsk)) {
      return XSK_UNCONSUMED;
    }
    action = _lookup_action(xsk, key_code);
    if (!action) {
      if (xsk->is_selection_mode) {
        return _key_pressed_on_selection_mode(xsk, key_code);
      }
      return XSK_UNCONSUMED;
    }
    if (!is_after_key_repeat_delay) {
      return XSK_CONSUMED;
    }
    if (!ud_send_key_event(xsk, key_code, FALSE, FALSE)) {
      return XSK_FAILED;
    }
    _reset_current_actions(xsk);
    return action->run(xsk, action) ? XSK_CONSUMED : XSK_FAILED;
  }

  if (!is_after_key_repeat_delay) {
    return XSK_CONSUMED;
  }
  result = xsk_handle_key_press(xsk, key_code);
  if (result != XSK_UNCONSUMED) {
    return result;
  }
  return ud_send_key_event(xsk, key_code, TRUE, FALSE)
    ? XSK_CONSUMED : XSK_FAILED;
}

gboolean xsk_send_key_events(XSetKeys *xsk, const KeyCodeArrayArray *key_arrays)
{
  gint index;

  if (!_send_regular_modifiers_event(xsk, ud_get_pressing_keys(xsk), FALSE)) {
    return FALSE;
  }
  for (index = 0;
       index < key_code_array_array_get_length(key_arrays);
       index++) {
    const KeyCodeArray *array = key_code_array_array_get_at(key_arrays, index);
    gboolean adds_shift = _adds_shift_on_send_keys(xsk, array);
    KeyCode shift_code;

    if (adds_shift) {
      shift_code = ki_get_modifier_key_code(&xsk->key_information,
                                            KI_MODIFIER_SHIFT);
      if (!ud_send_key_event(xsk, shift_code, TRUE, TRUE)) {
        return FALSE;
      }
    }
    if (!_send_key_events(xsk, &key_code_array_get_at(array, 0))) {
      return FALSE;
    }
    if (adds_shift) {
      if (!ud_send_key_event(xsk, shift_code, FALSE, TRUE)) {
        return FALSE;
      }
    }
  }
  if (!_send_regular_modifiers_event(xsk, ud_get_pressing_keys(xsk), TRUE)) {
    return FALSE;
  }
  return TRUE;
}

void xsk_toggle_selection_mode(XSetKeys *xsk)
{
  debug_print("%s selection mode", xsk->is_selection_mode ? "Exit" : "Enter" );
  xsk->is_selection_mode = !xsk->is_selection_mode;
}

gboolean xsk_is_excluded(XSetKeys *xsk)
{
  return window_system_is_excluded(xsk) || fcitx_is_excluded(xsk);
}

void xsk_reset_state(XSetKeys *xsk)
{
  _reset_current_actions(xsk);
  xsk->is_selection_mode = FALSE;
}

void xsk_mapping_changed(XSetKeys *xsk)
{
  ki_initialize(xsk->display, &xsk->key_information);
  if (xsk->root_actions) {
    action_list_free(xsk->root_actions);
  }
  xsk->root_actions = action_list_new();
  xsk_reset_state(xsk);
}

static const Action *_lookup_action(XSetKeys *xsk, KeyCode key_code)
{
  KeyCombination kc;

  kc = ki_pressing_keys_to_key_combination(&xsk->key_information,
                                           key_code,
                                           kd_get_pressing_keys(xsk));
  return action_list_lookup(xsk->current_actions, kc);
}

static XskResult _key_pressed_on_selection_mode(XSetKeys *xsk,
                                                KeyCode key_code)
{
  KeyCode shift_code;

  if (!_adds_shift_on_selection_mode(xsk,
                                     key_code,
                                     ud_get_pressing_keys(xsk))) {
    return XSK_UNCONSUMED;
  }

  if (ud_is_key_pressed(xsk, key_code)) {
    if (!ud_send_key_event(xsk, key_code, FALSE, FALSE)) {
      return XSK_FAILED;
    }
  }

  shift_code = ki_get_modifier_key_code(&xsk->key_information,
                                        KI_MODIFIER_SHIFT);
  if (!ud_send_key_event(xsk, shift_code, TRUE, TRUE)) {
    return XSK_FAILED;
  }
  if (!ud_send_key_event(xsk, key_code, TRUE, TRUE)) {
    return XSK_FAILED;
  }
  if (!ud_send_key_event(xsk, key_code, FALSE, TRUE)) {
    return XSK_FAILED;
  }
  if (!ud_send_key_event(xsk, shift_code, FALSE, TRUE)) {
    return XSK_FAILED;
  }
  return XSK_CONSUMED;
}

static gboolean _send_regular_modifiers_event(XSetKeys *xsk,
                                              const KeyCodeArray *keys,
                                              gboolean is_press)
{
  const KeyCode *pointer;

  for (pointer = &key_code_array_get_at(keys, 0); *pointer; pointer++) {
    if (!ki_is_regular_modifier(&xsk->key_information, *pointer)) {
      continue;
    }
    if (!ud_send_key_event(xsk, *pointer, is_press, TRUE)) {
      return FALSE;
    }
  }
  return TRUE;
}

static gboolean _adds_shift_on_send_keys(XSetKeys *xsk,
                                         const KeyCodeArray *keys)
{
  KeyCode key_code;

  if (!xsk->is_selection_mode) {
    return FALSE;
  }
  key_code = key_code_array_get_at(keys, key_code_array_get_length(keys) - 1);
  return _adds_shift_on_selection_mode(xsk, key_code, keys);
}

static gboolean _adds_shift_on_selection_mode(XSetKeys *xsk,
                                              KeyCode key_code,
                                              const KeyCodeArray *pressing_keys)
{
  if (ki_is_cursor(&xsk->key_information, key_code)) {
    if (!ki_contains_modifier(&xsk->key_information,
                              pressing_keys,
                              KI_MODIFIER_SHIFT)) {
      return TRUE;
    }
  } else if (!ki_is_modifier(&xsk->key_information, key_code)) {
    g_warning("Selection mode canceled");
    xsk_toggle_selection_mode(xsk);
  }
  return FALSE;
}

static gboolean _send_key_events(XSetKeys *xsk, const KeyCode *keys)
{
  if (!*keys) {
    return TRUE;
  }
  if (!ud_send_key_event(xsk, *keys, TRUE, TRUE)) {
    return FALSE;
  }
  if (!_send_key_events(xsk, keys + 1)) {
    return FALSE;
  }
  if (!ud_send_key_event(xsk, *keys, FALSE, TRUE)) {
    return FALSE;
  }
  return TRUE;
}
