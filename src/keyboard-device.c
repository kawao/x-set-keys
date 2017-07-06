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
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "common.h"
#include "keyboard-device.h"

typedef struct __KeyboardDevice {
  GSource source;
  GPollFD poll_fd;
  XSetKeys *xsk;
} _KeyboardDevice;

static gint _open_device_file(const gchar *device_filepath);
static gint _find_keyboard();
static gboolean _is_keyboard(gint fd);
static gboolean _prepare(GSource *source, gint *timeout);
static gboolean _check(GSource *source);
static gboolean _dispatch(GSource *source,
                          GSourceFunc callback,
                          gpointer user_data);
static gboolean _get_ev_bits(gint fd, uint8_t ev_bits[]);
static gboolean _get_key_bits(gint fd, uint8_t key_bits[]);

gboolean kd_initialize(XSetKeys *xsk, const gchar *device_filepath)
{
  static GSourceFuncs event_funcs = {
    _prepare,
    _check,
    _dispatch,
    NULL
  };

  gint fd;
  GSource *source;
  _KeyboardDevice *kd;

  fd = _open_device_file(device_filepath);
  if (fd < 0) {
    return FALSE;
  }
  source = g_source_new(&event_funcs, sizeof(_KeyboardDevice));
  kd = (_KeyboardDevice *)source;

  kd->xsk = xsk;
  kd->poll_fd.fd = fd;
  kd->poll_fd.events = G_IO_IN | G_IO_HUP | G_IO_ERR;
  g_source_add_poll(source, &kd->poll_fd);
  g_source_attach(source, NULL);

  xsk_set_keyboard_device(xsk, kd);

  return TRUE;
}

void kd_finalize(XSetKeys *xsk)
{
  _KeyboardDevice *kd = xsk_get_keyboard_device(xsk);

  if (close(kd->poll_fd.fd) < 0) {
    g_critical("Failed to close keyboard device : %s", strerror(errno));
  }
  g_source_destroy((GSource *)kd);
  g_source_unref((GSource *)kd);
}

gboolean kd_get_ev_bits(XSetKeys *xsk, uint8_t ev_bits[])
{
  _KeyboardDevice *kd = xsk_get_keyboard_device(xsk);

  return _get_ev_bits(kd->poll_fd.fd, ev_bits);
}

gboolean kd_get_key_bits(XSetKeys *xsk, uint8_t key_bits[])
{
  _KeyboardDevice *kd = xsk_get_keyboard_device(xsk);

  return _get_key_bits(kd->poll_fd.fd, key_bits);
}

static gint _open_device_file(const gchar *device_filepath)
{
  gint fd;

  if (device_filepath) {
    fd = open(device_filepath, O_RDWR);
    if (fd < 0) {
      g_critical("Failed to open %s : %s", device_filepath, strerror(errno));
      return -1;
    }
  } else {
    fd = _find_keyboard();
    if (fd < 0) {
      g_critical("Can not find keyboard device."
                 "Maybe you need root privilege to run %s.",
                 g_get_prgname());
      return -1;
    }
  }
  if (ioctl(fd, EVIOCGRAB, 1) < 0) {
    g_critical("Failed to grab keyboard : %s", strerror(errno));
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
    snprintf(filepath, sizeof(filepath), "/dev/input/event%d", index);
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

static gboolean _prepare(GSource *source, gint *timeout)
{
  *timeout = -1;
  return FALSE;
}

static gboolean _check(GSource *source)
{
  _KeyboardDevice *kd = (_KeyboardDevice *)source;

  return (kd->poll_fd.revents & (G_IO_IN | G_IO_HUP | G_IO_ERR)) != 0;
}

static gboolean _dispatch(GSource *source,
                          GSourceFunc callback,
                          gpointer user_data)
{
  _KeyboardDevice *kd = (_KeyboardDevice *)source;

  if (kd->poll_fd.revents & G_IO_HUP) {
    handle_fatal_error("Hang up keyboard device");
  } else if (kd->poll_fd.revents & G_IO_ERR) {
    handle_fatal_error("Error on keyboard device");
  } else if (kd->poll_fd.revents & G_IO_IN) {

  }
  return G_SOURCE_CONTINUE;
}

static gboolean _get_ev_bits(gint fd, uint8_t ev_bits[])
{
  return ioctl(fd, EVIOCGBIT(0, KD_EV_BITS_LENGTH), ev_bits) >= 0;
}

static gboolean _get_key_bits(gint fd, uint8_t key_bits[])
{
  return ioctl(fd, EVIOCGBIT(EV_KEY, KD_KEY_BITS_LENGTH), key_bits) >= 0;
}
