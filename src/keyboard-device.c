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
#include <stdint.h>
#include <linux/input.h>

#include "common.h"
#include "keyboard-device.h"

typedef struct __KDSource {
  GSource source;
  GPollFD poll_fd;
  XSetKeys *xsk;
} _KDSource;

#define _test_bit(bit, array) ((array)[(bit) / 8] & (1 << ((bit) % 8)))

static gint _open_device_file(const gchar *device_filepath);
static gint _find_keyboard();
static gboolean _is_keyboard(gint fd);
static gboolean _prepare(GSource *source, gint *timeout);
static gboolean _check(GSource *source);
static gboolean _dispatch(GSource *source,
                          GSourceFunc callback,
                          gpointer user_data);

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
  _KDSource *kd_source;

  fd = _open_device_file(device_filepath);
  if (fd < 0) {
    return FALSE;
  }
  source = g_source_new(&event_funcs, sizeof(_KDSource));
  kd_source = (_KDSource *)source;

  kd_source->xsk = xsk;
  kd_source->poll_fd.fd = fd;
  kd_source->poll_fd.events = G_IO_IN | G_IO_HUP | G_IO_ERR;
  g_source_add_poll(source, &kd_source->poll_fd);
  g_source_attach(source, NULL);

  xsk_set_kd_source(xsk, kd_source);

  return TRUE;
}

void kd_finalize(XSetKeys *xsk)
{
  _KDSource *kd_source = xsk_get_kd_source(xsk);
  g_return_if_fail(kd_source);

  if (close(kd_source->poll_fd.fd) < 0) {
    g_critical("Failed to close keyboard device %s", strerror(errno));
  }
  g_source_destroy((GSource *)kd_source);
  g_source_unref((GSource *)kd_source);
}

static gint _open_device_file(const gchar *device_filepath)
{
  gint fd;

  if (device_filepath) {
    fd = open(device_filepath, O_RDWR);
    if (fd < 0) {
      g_critical("Failed to open %s:%s", device_filepath, strerror(errno));
      return -1;
    }
  } else {
    fd = _find_keyboard();
    if (fd < 0) {
      g_critical("Can not find keyboard device."
                 "Maybe you need root Privilege to run %s.",
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
  uint8_t ev_bits[EV_MAX/8 + 1] = { 0 };
  uint8_t key_bits[(KEY_MAX+7)/8] = { 0 };

  if (ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0) {
    return FALSE;
  }
  if (!_test_bit(EV_KEY, ev_bits)) {
    return FALSE;
  }
  if (_test_bit(EV_REL, ev_bits)) {
    return FALSE;
  }
  if (_test_bit(EV_ABS, ev_bits)) {
    return FALSE;
  }

  if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) < 0) {
    return FALSE;
  }
  for (index = KEY_Q; index <= KEY_P; index++) {
    if (!_test_bit(index, key_bits)) {
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
  _KDSource *kd_source = (_KDSource *)source;

  return (kd_source->poll_fd.revents & (G_IO_IN | G_IO_HUP | G_IO_ERR)) != 0;
}

static gboolean _dispatch(GSource *source,
                          GSourceFunc callback,
                          gpointer user_data)
{
  _KDSource *kd_source = (_KDSource *)source;

  if (kd_source->poll_fd.revents & G_IO_HUP) {
    handle_fatal_error("Hang up Keyboard device");
  } else if (kd_source->poll_fd.revents & G_IO_ERR) {
    handle_fatal_error("Error on Keyboard device");
  } else if (kd_source->poll_fd.revents & G_IO_IN) {

  }
  return G_SOURCE_CONTINUE;
}
