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
static gboolean _initialize_keys(Device *device);
static gboolean _get_ev_bits(gint fd, guint8 ev_bits[]);
static gboolean _get_key_bits(gint fd, guint8 key_bits[]);
static gboolean _handle_input(gpointer user_data);

Device *kd_initialize(XSetKeys *xsk, const gchar *device_filepath)
{
  gint fd;
  Device *device;

  fd = _open_device_file(device_filepath);
  if (fd < 0) {
    return NULL;
  }

  device = device_initialize(fd, "keyboard device", _handle_input, xsk);
  if (!_initialize_keys(device)) {
    device_finalize(device);
    return NULL;
  }
  if (ioctl(fd, EVIOCGRAB, 1) < 0) {
    print_error("Failed to grab keyboard device");
    device_finalize(device);
    return NULL;
  }
  return device;
}

void kd_finalize(XSetKeys *xsk)
{
  Device *device = xsk_get_keyboard_device(xsk);

  if (ioctl(device_get_fd(device), EVIOCGRAB, 0) < 0) {
    print_error("Failed to ungrab keyboard device");
  }
  device_finalize(device);
}

gboolean kd_get_ev_bits(XSetKeys *xsk, guint8 ev_bits[])
{
  Device *device = xsk_get_keyboard_device(xsk);

  return _get_ev_bits(device_get_fd(device), ev_bits);
}

gboolean kd_get_key_bits(XSetKeys *xsk, guint8 key_bits[])
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
  guint8 ev_bits[KD_EV_BITS_LENGTH] = { 0 };
  guint8 key_bits[KD_KEY_BITS_LENGTH] = { 0 };
  gint index;

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

static gboolean _initialize_keys(Device *device)
{
  guint8 key_bits[KD_KEY_BITS_LENGTH] = { 0 };
  struct input_event event = { { 0 } };
  gint index;

  gettimeofday(&event.time, NULL);
  event.type = EV_KEY;
  event.value = 0;

  if (!_get_key_bits(device_get_fd(device), key_bits)) {
    print_error("Failed to get key bits from keyboard device");
    return FALSE;
  }
  for (index = 0; index < KEY_CNT; index++) {
    if (kd_test_bit(key_bits, index)) {
      event.code = index;
      if (!device_write(device, &event, sizeof (event))) {
        return FALSE;
      }
    }
  }

  event.type = EV_SYN;
  event.code = SYN_REPORT;
  event.value = 0;
  return device_write(device, &event, sizeof (event));
}

static gboolean _get_ev_bits(gint fd, guint8 ev_bits[])
{
  return ioctl(fd, EVIOCGBIT(0, KD_EV_BITS_LENGTH), ev_bits) >= 0;
}

static gboolean _get_key_bits(gint fd, guint8 key_bits[])
{
  return ioctl(fd, EVIOCGBIT(EV_KEY, KD_KEY_BITS_LENGTH), key_bits) >= 0;
}

static gboolean _handle_input(gpointer user_data)
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
  switch (event.type) {
  case EV_MSC:
    if (event.code == MSC_SCAN) {
      return TRUE;
    }
    break;
  case EV_KEY:
    if (xsk_is_valid_key(event.code)) {
      switch (event.value) {
      case 0:
        kd_is_key_pressed(xsk, event.code) = FALSE;
        if (!ud_is_key_pressed(xsk, event.code)) {
          return TRUE;
        }
        break;
      case 1:
        kd_is_key_pressed(xsk, event.code) = TRUE;
        xsk_set_key_press_start_time(xsk, event.time);
        break;
      default:
        break;
      }
    }
    break;
  }

  return ud_send_event(xsk, &event);
}
