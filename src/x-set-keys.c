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
#include "action.h"
#include "keyboard-device.h"
#include "uinput-device.h"

#define _reset_current_actions(xsk)                 \
  ((xsk)->current_actions = (xsk)->root_actions)

static gboolean _is_disabled(XSetKeys *xsk);
static const Action *_lookup_action(XSetKeys *xsk, KeyCode key_code);
static gboolean _send_regular_modifiers_event(XSetKeys *xsk,
                                              const KeyCodeArray *keys,
                                              gboolean is_press);
static gboolean _send_key_events(XSetKeys *xsk, const KeyCode *keys);

gboolean xsk_initialize(XSetKeys *xsk)
{
  xsk->display = XOpenDisplay(NULL);
  if (!xsk->display) {
    g_critical("Could not create X11 display");
    return FALSE;
  }
  ki_initialize(xsk->display, &xsk->key_information);
  xsk->root_actions = action_list_new();
  return TRUE;
}

gboolean xsk_start(XSetKeys *xsk, const gchar *device_filepath)
{
  xsk->keyboard_device = kd_initialize(xsk, device_filepath);
  if (!xsk->keyboard_device) {
    return FALSE;
  }
  xsk->uinput_device = ud_initialize(xsk);
  if (!xsk->uinput_device) {
    return FALSE;
  }
  _reset_current_actions(xsk);
  return TRUE;
}

void xsk_finalize(XSetKeys *xsk)
{
  if (xsk->uinput_device) {
    ud_finalize(xsk);
  }
  if (xsk->keyboard_device) {
    kd_finalize(xsk);
  }
  if (xsk->display) {
    ki_finalize(&xsk->key_information);
    XCloseDisplay(xsk->display);
  }
  if (xsk->root_actions) {
    action_list_free(xsk->root_actions);
  }
}

XskResult xsk_handle_key_press(XSetKeys *xsk, KeyCode key_code)
{
  const Action *action;

  if (_is_disabled(xsk)) {
    return XSK_PASSING_BY;
  }
  action = _lookup_action(xsk, key_code);
  if (action) {
    _reset_current_actions(xsk);
    return action->run(xsk, action->data) ? XSK_INTERCEPTED : XSK_ERROR;
  }
  if (!ki_is_modifier(&xsk->key_information, key_code)) {
    if (xsk->current_actions != xsk->root_actions) {
      _reset_current_actions(xsk);
      g_warning("Key sequence canceled");
    }
  }
  return XSK_PASSING_BY;
}

XskResult xsk_handle_key_repeat(XSetKeys *xsk,
                                KeyCode key_code,
                                gint seconds_since_pressed)
{
  const Action *action;
  XskResult result;

  if (ud_is_key_pressed(xsk, key_code)) {
    if (_is_disabled(xsk)) {
      return XSK_PASSING_BY;
    }
    action = _lookup_action(xsk, key_code);
    if (!action) {
      return XSK_PASSING_BY;
    }
    if (!seconds_since_pressed) {
      return XSK_INTERCEPTED;
    }
    if (!ud_send_key_event(xsk, key_code, FALSE, FALSE)) {
      return XSK_ERROR;
    }
    _reset_current_actions(xsk);
    return action->run(xsk, action->data) ? XSK_INTERCEPTED : XSK_ERROR;
  }

  if (!seconds_since_pressed) {
    return XSK_INTERCEPTED;
  }
  result = xsk_handle_key_press(xsk, key_code);
  if (result != XSK_PASSING_BY) {
    return result;
  }
  return ud_send_key_event(xsk, key_code, TRUE, FALSE)
    ? XSK_INTERCEPTED : XSK_ERROR;
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
    KeyCodeArray *array = key_code_array_array_get_at(key_arrays, index);
    if (!_send_key_events(xsk, &key_code_array_get_at(array, 0))) {
      return FALSE;
    }
  }
  if (!_send_regular_modifiers_event(xsk, ud_get_pressing_keys(xsk), TRUE)) {
    return FALSE;
  }
  return TRUE;
}

gboolean _is_disabled(XSetKeys *xsk)
{
  return FALSE;
}

static const Action *_lookup_action(XSetKeys *xsk, KeyCode key_code)
{
  KeyCombination kc;

  kc = ki_pressing_keys_to_key_combination(&xsk->key_information,
                                           key_code,
                                           kd_get_pressing_keys(xsk));
  return action_list_lookup(xsk->current_actions, &kc);
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
