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

#include "x-set-keys.h"
#include "action.h"
#include "keyboard-device.h"
#include "uinput-device.h"

#define _reset_current_actions(xsk)                 \
  ((xsk)->current_actions = (xsk)->root_actions)

const Action *_lookup_action(XSetKeys *xsk, KeyCode key_code);

gboolean xsk_initialize(XSetKeys *xsk)
{
  xsk->display = XOpenDisplay(NULL);
  if (!xsk->display) {
    g_critical("Could not create X11 display");
    return FALSE;
  }
  ki_initialize(xsk->display, &xsk->key_information);
  xsk->root_actions = action_list_new();
  xsk->keyboard_pressing_keys = key_code_array_new(6);
  xsk->uinput_pressing_keys = key_code_array_new(6);
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
  if (xsk->keyboard_pressing_keys) {
    key_code_array_free(xsk->keyboard_pressing_keys);
  }
  if (xsk->uinput_pressing_keys) {
    key_code_array_free(xsk->uinput_pressing_keys);
  }
}

gboolean xsk_is_disabled(XSetKeys *xsk)
{
  return FALSE;
}

XskResult xsk_handle_key_press(XSetKeys *xsk, KeyCode key_code)
{
  const Action *action;

  if (xsk_is_disabled(xsk)) {
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
    if (xsk_is_disabled(xsk)) {
      return XSK_PASSING_BY;
    }
    action = _lookup_action(xsk, key_code);
    if (!action) {
      return XSK_PASSING_BY;
    }
    if (!seconds_since_pressed) {
      return XSK_INTERCEPTED;
    }
    if (!ud_send_key_event(xsk, key_code, FALSE)) {
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
  return ud_send_key_event(xsk, key_code, TRUE) ? XSK_INTERCEPTED : XSK_ERROR;
}

const Action *_lookup_action(XSetKeys *xsk, KeyCode key_code)
{
  KeyCombination kc;

  kc = ki_pressing_keys_to_key_combination(xsk_get_key_information(xsk),
                                           key_code,
                                           xsk_get_keyboard_pressing_keys(xsk));
  return action_list_lookup(xsk_get_current_actions(xsk), &kc);
}
