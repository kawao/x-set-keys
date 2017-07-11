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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "common.h"
#include "keyboard-device.h"
#include "uinput-device.h"

static gint _open_device_file(const gchar *device_filepath);
static gint _find_keyboard();
static gboolean _is_keyboard(gint fd);
static gboolean _get_ev_bits(gint fd, uint8_t ev_bits[]);
static gboolean _get_key_bits(gint fd, uint8_t key_bits[]);
static gboolean _handle_event(gpointer user_data);

Device *kd_initialize(XSetKeys *xsk, const gchar *device_filepath)
{
  gint fd = _open_device_file(device_filepath);
  if (fd < 0) {
    return NULL;
  }
  return device_initialize(fd, "keyboard device", _handle_event, xsk);
}

void kd_finalize(XSetKeys *xsk)
{
  Device *device = xsk_get_keyboard_device(xsk);

  if (ioctl(device_get_fd(device), EVIOCGRAB, 0) < 0) {
    print_error("Failed to ungrab keyboard device");
  }
  device_finalize(device);
}

gboolean kd_get_ev_bits(XSetKeys *xsk, uint8_t ev_bits[])
{
  Device *device = xsk_get_keyboard_device(xsk);

  return _get_ev_bits(device_get_fd(device), ev_bits);
}

gboolean kd_get_key_bits(XSetKeys *xsk, uint8_t key_bits[])
{
  Device *device = xsk_get_keyboard_device(xsk);

  return _get_key_bits(device_get_fd(device), key_bits);
}

static gint _open_device_file(const gchar *device_filepath)
{
  gint fd;

  if (device_filepath) {
    fd = open(device_filepath, O_RDWR);
    if (fd < 0) {
      print_error("Failed to open %s", device_filepath);
      return -1;
    }
  } else {
    fd = _find_keyboard();
    if (fd < 0) {
      g_critical("Can not find keyboard device."
                 " Maybe you need root privilege to run %s.",
                 g_get_prgname());
      return -1;
    }
  }
  if (ioctl(fd, EVIOCGRAB, 1) < 0) {
    print_error("Failed to grab keyboard device");
    close(fd);
    return -1;
  }
  return fd;
}

static gint _find_keyboard()
{
  gint index;
  gchar filepath[32];
  gint fd;

  for (index = 0; index < 32; index++) {
    snprintf(filepath, sizeof (filepath), "/dev/input/event%d", index);
    fd = open(filepath, O_RDWR);
    if (fd < 0) {
      continue;
    }
    if (_is_keyboard(fd)) {
      g_message("Found keyboard device : %s", filepath);
      return fd;
    }
    close(fd);
  }
  return -1;
}

static gboolean _is_keyboard(gint fd)
{
  gint index;
  uint8_t ev_bits[KD_EV_BITS_LENGTH] = { 0 };
  uint8_t key_bits[KD_KEY_BITS_LENGTH] = { 0 };

  if (!_get_ev_bits(fd, ev_bits)) {
    return FALSE;
  }
  if (!kd_test_bit(ev_bits, EV_KEY)) {
    return FALSE;
  }
  if (kd_test_bit(ev_bits, EV_REL)) {
    return FALSE;
  }
  if (kd_test_bit(ev_bits, EV_ABS)) {
    return FALSE;
  }

  if (!_get_key_bits(fd, key_bits)) {
    return FALSE;
  }
  for (index = KEY_Q; index <= KEY_P; index++) {
    if (!kd_test_bit(key_bits, index)) {
      return FALSE;
    }
  }

  return TRUE;
}

static gboolean _get_ev_bits(gint fd, uint8_t ev_bits[])
{
  return ioctl(fd, EVIOCGBIT(0, KD_EV_BITS_LENGTH), ev_bits) >= 0;
}

static gboolean _get_key_bits(gint fd, uint8_t key_bits[])
{
  return ioctl(fd, EVIOCGBIT(EV_KEY, KD_KEY_BITS_LENGTH), key_bits) >= 0;
}

static gboolean _handle_event(gpointer user_data)
{
  XSetKeys *xsk;
  Device *device;
  gssize length;
  struct input_event event;

  xsk = user_data;
  device = xsk_get_keyboard_device(xsk);

  length = device_read(device, &event, sizeof (event));
  if (length < 0) {
    return FALSE;
  }
  if (length != sizeof (event)) {
    g_critical("Tow few read length from keyboard device : read=%ld < %ld",
               length,
               sizeof (event));
    return FALSE;
  }
  debug_print("Read from keyboard : type=%02x code=%d value=%d",
              event.type,
              event.code,
              event.value);
  return ud_send_event(xsk, &event);
}
