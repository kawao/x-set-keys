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
#include "keyboard-device.h"
#include "uinput-device.h"

gboolean xsk_initialize(XSetKeys *xsk)
{
  return TRUE;
}

gboolean xsk_start(XSetKeys *xsk, const gchar *device_filepath)
{
  if (!kd_initialize(xsk, device_filepath)) {
    return FALSE;
  }
  if (!ud_initialize(xsk)) {
    return FALSE;
  }
  return TRUE;
}

void xsk_finalize(XSetKeys *xsk)
{
  if (xsk_get_uinput_device(xsk)) {
    ud_finalize(xsk);
  }
  if (xsk_get_keyboard_device(xsk)) {
    kd_finalize(xsk);
  }
}
