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
#include <linux/uinput.h>

#include "common.h"
#include "uinput-device.h"
#include "keyboard-device.h"

typedef struct __UInputDevice {
  GSource source;
  GPollFD poll_fd;
  XSetKeys *xsk;
} _UInputDevice;


static gint _open_uinput_device();
static gboolean _create_uinput_device(XSetKeys *xsk, gint fd);
static gboolean _prepare(GSource *source, gint *timeout);
static gboolean _check(GSource *source);
static gboolean _dispatch(GSource *source,
                          GSourceFunc callback,
                          gpointer user_data);
static gboolean _write(gint fd, gconstpointer buffer, gsize length);

gboolean ud_initialize(XSetKeys *xsk)
{
  static GSourceFuncs event_funcs = {
    _prepare,
    _check,
    _dispatch,
    NULL
  };
  gint fd;
  GSource *source;
  _UInputDevice *ud;

  fd = _open_uinput_device();
  if (fd < 0) {
    g_critical("Failed to open uinput device."
               " Maybe uinput module is not loaded");
    return FALSE;
  }
  if (!_create_uinput_device(xsk, fd)) {
    close(fd);
    return FALSE;
  }

  source = g_source_new(&event_funcs, sizeof(_UInputDevice));
  ud = (_UInputDevice *)source;

  ud->xsk = xsk;
  ud->poll_fd.fd = fd;
  ud->poll_fd.events = G_IO_IN | G_IO_HUP | G_IO_ERR;
  g_source_add_poll(source, &ud->poll_fd);
  g_source_attach(source, NULL);

  xsk_set_uinput_device(xsk, ud);

  return TRUE;
}

void ud_finalize(XSetKeys *xsk)
{
  _UInputDevice *ud = xsk_get_uinput_device(xsk);

  if (ioctl(ud->poll_fd.fd, UI_DEV_DESTROY) < 0) {
    g_critical("Failed to destroy uinput device : %s", strerror(errno));
  }
  if (close(ud->poll_fd.fd) < 0) {
    g_critical("Failed to close uinput device : %s", strerror(errno));
  }
  g_source_destroy((GSource *)ud);
  g_source_unref((GSource *)ud);
}

gboolean ud_write(XSetKeys *xsk, gconstpointer buffer, gsize length)
{
  _UInputDevice *ud = xsk_get_uinput_device(xsk);

  return _write(ud->poll_fd.fd, buffer, length);
}

static gint _open_uinput_device()
{
  const char* filepath[] = {"/dev/input/uinput", "/dev/uinput"};
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

static gboolean _create_uinput_device(XSetKeys *xsk, gint fd)
{
  struct uinput_user_dev device = { { 0 } };
  uint8_t ev_bits[KD_EV_BITS_LENGTH] = { 0 };
  uint8_t key_bits[KD_KEY_BITS_LENGTH] = { 0 };
  gint index = 0;

  device.id.bustype = BUS_VIRTUAL;
  strcpy(device.name, "x-set-keys");
  device.id.vendor = 1;
  device.id.product = 1;
  device.id.version = 1;
  if (!_write(fd, &device, sizeof(device))) {
    g_critical("Failed to write uinput user device : %s", strerror(errno));
    return FALSE;
  }

  if (!kd_get_ev_bits(xsk, ev_bits)) {
    g_critical("Failed to get ev bits from keyboard device : %s",
               strerror(errno));
    return FALSE;
  }
  for (index = 0; index < EV_CNT; index++) {
    if (kd_test_bit(ev_bits, index)) {
      if (ioctl(fd, UI_SET_EVBIT, index) < 0) {
        g_critical("Failed to set ev bit %02x to uinput device : %s",
                   index, strerror(errno));
        return FALSE;
      }
      debug_print("UI_SET_EVBIT : %02x", index);
    }
  }

  if (!kd_get_key_bits(xsk, key_bits)) {
    g_critical("Failed to get key bits from keyboard device : %s",
               strerror(errno));
    return FALSE;
  }
  for (index = 0; index < KEY_CNT; index++) {
    if (kd_test_bit(key_bits, index)) {
      if (ioctl(fd, UI_SET_KEYBIT, index) < 0) {
        g_critical("Failed to set key bit %d to uinput device : %s",
                   index, strerror(errno));
        return FALSE;
      }
      debug_print("UI_SET_KEYBIT : %d", index);
    }
  }

  if (ioctl(fd, UI_DEV_CREATE) < 0) {
    g_critical("Failed to create uinput device : %s", strerror(errno));
    return FALSE;
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
  _UInputDevice *ud = (_UInputDevice *)source;

  return (ud->poll_fd.revents & (G_IO_IN | G_IO_HUP | G_IO_ERR)) != 0;
}

static gboolean _dispatch(GSource *source,
                          GSourceFunc callback,
                          gpointer user_data)
{
  _UInputDevice *ud = (_UInputDevice *)source;

  if (ud->poll_fd.revents & G_IO_HUP) {
    handle_fatal_error("Hang up uinput device");
  } else if (ud->poll_fd.revents & G_IO_ERR) {
    handle_fatal_error("Error on uinput device");
  } else if (ud->poll_fd.revents & G_IO_IN) {
    gssize length;
    guchar buffer[256];

    do {
      length = read(ud->poll_fd.fd, buffer, sizeof(buffer));
    } while (length <= 0 && errno == EINTR);

    if (length <= 0) {
      handle_fatal_error("Failed to read uinput device");
    } else {
      debug_print("Read from uinput : length=%ld", length);
      if (!kd_write(ud->xsk, buffer, length)) {
        handle_fatal_error("Failed to write keyboard device");
      }
    }

  }
  return G_SOURCE_CONTINUE;
}

static gboolean _write(gint fd, gconstpointer buffer, gsize length)
{
  gssize rest = length;
  while (rest > 0) {
    gssize written = write(fd, buffer, rest);
    if (written < 0) {
      if (errno == EINTR) {
        continue;
      }
      return FALSE;
    }
    rest -= written;
    buffer += written;
  }
  return TRUE;
}
