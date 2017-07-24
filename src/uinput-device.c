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

#include <unistd.h>
#include <linux/uinput.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common.h"
#include "uinput-device.h"
#include "keyboard-device.h"

static gint _open_uinput_device();
static gboolean _write_user_dev(Device *device);
static gboolean _set_evbits(Device *device, XSetKeys *xsk);
static gboolean _set_keybits(Device *device, XSetKeys *xsk);
static gboolean _handle_input(gpointer user_data);
static gboolean _send_event(XSetKeys *xsk,
                            struct input_event *event,
                            gboolean is_temporary);

UInputDevice *ud_initialize(XSetKeys *xsk)
{
  UInputDevice *device;
  gint fd = _open_uinput_device();

  if (fd < 0) {
    g_critical("Failed to open uinput device."
               " Maybe uinput module is not loaded");
    return NULL;
  }
  device = (UInputDevice *)device_initialize(fd,
                                             "uinput device",
                                             sizeof (UInputDevice),
                                             _handle_input,
                                             xsk);
  if (!_write_user_dev(&device->device)) {
    device_finalize(&device->device);
    return NULL;
  }
  if (!_set_evbits(&device->device, xsk)) {
    device_finalize(&device->device);
    return NULL;
  }
  if (!_set_keybits(&device->device, xsk)) {
    device_finalize(&device->device);
    return NULL;
  }
  if (ioctl(fd, UI_DEV_CREATE) < 0) {
    print_error("Failed to create uinput device");
    device_finalize(&device->device);
    return NULL;
  }
  device->pressing_keys = key_code_array_new(6);
  return device;
}

void ud_finalize(XSetKeys *xsk)
{
  UInputDevice *device = xsk_get_uinput_device(xsk);

  if (ioctl(device_get_fd(&device->device), UI_DEV_DESTROY) < 0) {
    print_error("Failed to destroy uinput device");
  }
  if (device->pressing_keys) {
    key_code_array_free(device->pressing_keys);
  }
  device_finalize(&device->device);
}

gboolean ud_send_key_event(XSetKeys *xsk,
                           KeyCode key_cord,
                           gboolean is_press,
                           gboolean is_temporary)
{
  struct input_event event;

  event.type = EV_KEY;
  event.code = key_cord;
  event.value = is_press ? 1 : 0;
  if (!_send_event(xsk, &event, is_temporary)) {
    return FALSE;
  }

  event.type = EV_SYN;
  event.code = SYN_REPORT;
  event.value = 0;
  if (!_send_event(xsk, &event, is_temporary)) {
    return FALSE;
  }

  return TRUE;
}

gboolean ud_send_event(XSetKeys *xsk, struct input_event *event)
{
  return _send_event(xsk, event, FALSE);
}

static gint _open_uinput_device()
{
  const gchar* filepath[] = { "/dev/input/uinput", "/dev/uinput" };
  gint fd;
  gint index;

  for (index = 0; index < array_num(filepath); index++) {
    fd = open(filepath[index], O_RDWR);
    if (fd >= 0) {
      break;
    }
  }
  return fd;
}

static gboolean _write_user_dev(Device *device)
{
  struct uinput_user_dev user_dev = { { 0 } };

  user_dev.id.bustype = BUS_VIRTUAL;
  strcpy(user_dev.name, "x-set-keys");
  user_dev.id.vendor = 1;
  user_dev.id.product = 1;
  user_dev.id.version = 1;
  if (!device_write(device, &user_dev, sizeof (user_dev))) {
    print_error("Failed to write uinput user device");
    return FALSE;
  }
  return TRUE;
}

static gboolean _set_evbits(Device *device, XSetKeys *xsk)
{
  guint8 ev_bits[KD_EV_BITS_LENGTH] = { 0 };
  gint index;

  if (!kd_get_ev_bits(xsk, ev_bits)) {
    print_error("Failed to get ev bits from keyboard device");
    return FALSE;
  }
  for (index = 0; index < EV_CNT; index++) {
    if (kd_test_bit(ev_bits, index)) {
      if (ioctl(device_get_fd(device), UI_SET_EVBIT, index) < 0) {
        print_error("Failed to set ev bit %02x to uinput device", index);
        return FALSE;
      }
      debug_print("UI_SET_EVBIT : %02x", index);
    }
  }
  return TRUE;
}

static gboolean _set_keybits(Device *device, XSetKeys *xsk)
{
  guint8 key_bits[KD_KEY_BITS_LENGTH] = { 0 };
  gint index;

  if (!kd_get_key_bits(xsk, key_bits)) {
    print_error("Failed to get key bits from keyboard device");
    return FALSE;
  }
  for (index = 0; index < KEY_CNT; index++) {
    if (kd_test_bit(key_bits, index)) {
      if (ioctl(device_get_fd(device), UI_SET_KEYBIT, index) < 0) {
        print_error("Failed to set key bit %d to uinput device", index);
        return FALSE;
      }
      debug_print("UI_SET_KEYBIT : %d", index);
    }
  }
  return TRUE;
}

static gboolean _handle_input(gpointer user_data)
{
  XSetKeys *xsk;
  UInputDevice *device;
  gssize length;
  struct input_event event;

  xsk = user_data;
  device = xsk_get_uinput_device(xsk);

  length = device_read(&device->device, &event, sizeof (event));
  if (length < 0) {
    return FALSE;
  }
  debug_print("Read from uinput : length=%ld", length);
  return kd_write(xsk, &event, length);
}

static gboolean _send_event(XSetKeys *xsk,
                            struct input_event *event,
                            gboolean is_temporary)
{
  UInputDevice *device = xsk_get_uinput_device(xsk);

  if (!is_temporary) {
    switch (event->type) {
    case EV_SYN:
      if (device->last_event_type == EV_SYN) {
        return TRUE;
      }
      break;
    case EV_KEY:
      if (!ki_is_valid_key_code(event->code)) {
        break;
      }
      switch (event->value) {
      case 0:
        if (!key_code_array_remove(device->pressing_keys, event->code)) {
          return TRUE;
        }
        break;
      case 1:
        key_code_array_add(device->pressing_keys, event->code);
        break;
      }
      break;
    }
  }
  device->last_event_type = event->type;
  debug_print("Write to uinput : type=%02x code=%d value=%d",
              event->type,
              event->code,
              event->value);
  gettimeofday(&event->time, NULL);
  return device_write(&device->device, event, sizeof (*event));
}
