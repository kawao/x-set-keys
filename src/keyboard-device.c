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

#define _USEC_PER_SEC  1000000ul
#define _USEC_PER_MSEC    1000ul

#define _ELAPSED_USEC(t1, t2)                      \
  (((t2)->tv_sec - (t1)->tv_sec) * _USEC_PER_SEC + \
   (t2)->tv_usec - (t1)->tv_usec)

static gint _open_device_file(const gchar *device_filepath);
static gint _find_keyboard();
static gboolean _is_keyboard(gint fd);
static gboolean _initialize_keys(Device *device);
static gboolean _get_ev_bits(gint fd, guint8 ev_bits[]);
static gboolean _get_key_bits(gint fd, guint8 key_bits[]);
static gboolean _handle_input(gpointer user_data);
static gboolean _handle_event(XSetKeys *xsk, struct input_event *event);
static gboolean _is_after_repeat_delay(Display *display,
                                       XkbDescPtr xkb,
                                       struct timeval *t1,
                                       const struct timeval *t2);

KeyboardDevice *kd_initialize(XSetKeys *xsk, const gchar *device_filepath)
{
  gint fd;
  KeyboardDevice *device;

  fd = _open_device_file(device_filepath);
  if (fd < 0) {
    return NULL;
  }

  device = (KeyboardDevice *)device_initialize(fd,
                                               "keyboard device",
                                               sizeof (KeyboardDevice),
                                               _handle_input,
                                               xsk);
  if (!_initialize_keys(&device->device)) {
    device_finalize(&device->device);
    return NULL;
  }
  if (ioctl(fd, EVIOCGRAB, 1) < 0) {
    print_error("Failed to grab keyboard device");
    device_finalize(&device->device);
    return NULL;
  }
  device->xkb = XkbAllocKeyboard();
  if (!device->xkb) {
    g_critical("Failed to allocate keyboard description");
    device_finalize(&device->device);
    return NULL;
  }
  device->pressing_keys = key_code_array_new(6);
  return device;
}

void kd_finalize(XSetKeys *xsk)
{
  KeyboardDevice *device = xsk_get_keyboard_device(xsk);

  if (ioctl(device_get_fd(&device->device), EVIOCGRAB, 0) < 0) {
    print_error("Failed to ungrab keyboard device");
  }
  if (device->pressing_keys) {
    key_code_array_free(device->pressing_keys);
  }
  if (device->xkb) {
    XkbFreeKeyboard(device->xkb, 0, True);
  }
  device_close(&device->device);
  device_finalize(&device->device);
}

gboolean kd_get_ev_bits(XSetKeys *xsk, guint8 ev_bits[])
{
  KeyboardDevice *device = xsk_get_keyboard_device(xsk);

  return _get_ev_bits(device_get_fd(&device->device), ev_bits);
}

gboolean kd_get_key_bits(XSetKeys *xsk, guint8 key_bits[])
{
  KeyboardDevice *device = xsk_get_keyboard_device(xsk);

  return _get_key_bits(device_get_fd(&device->device), key_bits);
}

gboolean kd_get_led_bits(XSetKeys *xsk, guint8 led_bits[])
{
  KeyboardDevice *device = xsk_get_keyboard_device(xsk);

  return ioctl(device_get_fd(&device->device),
               EVIOCGBIT(EV_LED, KD_LED_BITS_LENGTH),
               led_bits) >= 0;
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
  KeyboardDevice *device;
  gssize length;
  struct input_event event;

  xsk = user_data;
  device = xsk_get_keyboard_device(xsk);

  length = device_read(&device->device, &event, sizeof (event));
  if (length < 0) {
    return FALSE;
  }
  if (length != sizeof (event)) {
    g_critical("Tow few read length from keyboard device : read=%zd < %zd",
               length,
               sizeof (event));
    return FALSE;
  }

#ifdef TRACE
  debug_print("Read from keyboard : type=%02x code=%d value=%d",
              event.type,
              event.code,
              event.value);
#endif
  return _handle_event(xsk, &event);
}

static gboolean _handle_event(XSetKeys *xsk, struct input_event *event)
{
  KeyboardDevice *device = xsk_get_keyboard_device(xsk);
  gboolean is_after_repeat_delay;

  switch (event->type) {
  case EV_MSC:
    return TRUE;
    break;
  case EV_KEY:
    if (!ki_is_valid_key_code(event->code)) {
      break;
    }
    switch (event->value) {
    case 0:
      key_code_array_remove(device->pressing_keys, event->code);
      break;
    case 1:
      key_code_array_add(device->pressing_keys, event->code);
      device->press_start_time = event->time;
      switch (xsk_handle_key_press(xsk, event->code)) {
      case XSK_CONSUMED:
        return TRUE;
      case XSK_FAILED:
        return FALSE;
      default:
        break;
      }
      break;
    default:
      is_after_repeat_delay = _is_after_repeat_delay(xsk_get_display(xsk),
                                                     device->xkb,
                                                     &device->press_start_time,
                                                     &event->time);
      switch (xsk_handle_key_repeat(xsk, event->code, is_after_repeat_delay)) {
      case XSK_CONSUMED:
        return TRUE;
      case XSK_FAILED:
        return FALSE;
      default:
        break;
      }
    }
    break;
  }
  return ud_send_event(xsk, event);
}

static gboolean _is_after_repeat_delay(Display *display,
                                       XkbDescPtr xkb,
                                       struct timeval *t1,
                                       const struct timeval *t2)
{
  if (XkbGetControls(display,
                     XkbRepeatKeysMask|XkbControlsEnabledMask,
                     xkb) != Success) {
    g_warning("XkbGetControls() failed");
    return FALSE;
  }

  if (!(xkb->ctrls->enabled_ctrls & XkbRepeatKeysMask)) {
    return FALSE;
  }

  if (t2->tv_sec < t1->tv_sec ||
      (t2->tv_sec == t1->tv_sec && t2->tv_usec < t1->tv_usec) ||
      _ELAPSED_USEC(t1, t2) < xkb->ctrls->repeat_delay * _USEC_PER_MSEC) {
    return FALSE;
  }
  t1->tv_usec += xkb->ctrls->repeat_interval * _USEC_PER_MSEC;
  while (t1->tv_usec >= _USEC_PER_SEC) {
    t1->tv_sec++;
    t1->tv_usec -= _USEC_PER_SEC;
  }
  return TRUE;
}
