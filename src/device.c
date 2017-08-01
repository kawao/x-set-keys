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
#include <string.h>
#include <errno.h>

#include "common.h"
#include "device.h"

static gboolean _prepare(GSource *source, gint *timeout);
static gboolean _check(GSource *source);
static gboolean _dispatch(GSource *source,
                          GSourceFunc callback,
                          gpointer user_data);

Device *device_initialize(gint fd,
                          const gchar *name,
                          guint struct_size,
                          GSourceFunc callback,
                          gpointer user_data)
{
  static GSourceFuncs event_funcs = {
    _prepare,
    _check,
    _dispatch,
    NULL
  };
  Device *device = (Device *)g_source_new(&event_funcs, struct_size);

  g_source_set_name(&device->source, name);
  device->poll_fd.fd = fd;
  device->poll_fd.events = G_IO_IN | G_IO_HUP | G_IO_ERR;
  g_source_add_poll(&device->source, &device->poll_fd);
  g_source_set_callback(&device->source, callback, user_data, NULL);
  g_source_attach(&device->source, NULL);

  return device;
}

void device_finalize(Device *device)
{
  g_source_destroy(&device->source);
  g_source_unref(&device->source);
}

void device_close(Device *device)
{
  if (close(device->poll_fd.fd) < 0) {
    print_error("Failed to close %s", g_source_get_name(&device->source));
  }
}

gssize device_read(Device *device, gpointer buffer, gsize count)
{
  gssize length;

  do {
    length = read(device->poll_fd.fd, buffer, count);
  } while (length < 0 && errno == EINTR);
  if (length < 0) {
    print_error("Failed to read %s", g_source_get_name(&device->source));
  }
  return length;
}

gboolean device_write(Device *device, gconstpointer buffer, gsize length)
{
  gssize rest = length;

  while (rest > 0) {
    gssize written = write(device->poll_fd.fd, buffer, rest);
    if (written < 0) {
      if (errno == EINTR) {
        continue;
      }
      print_error("Failed to write %s", g_source_get_name(&device->source));
      return FALSE;
    }
    rest -= written;
    buffer += written;
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
  Device *device = (Device *)source;

  return (device->poll_fd.revents & (G_IO_IN | G_IO_HUP | G_IO_ERR)) != 0;
}

static gboolean _dispatch(GSource *source,
                          GSourceFunc callback,
                          gpointer user_data)
{
  Device *device = (Device *)source;

  if (device->poll_fd.revents & G_IO_HUP) {
    print_error("Hang up %s", g_source_get_name(&device->source));
    notify_error();
  } else if (device->poll_fd.revents & G_IO_ERR) {
    print_error("I/O Error on %s", g_source_get_name(&device->source));
    notify_error();
  } else if (device->poll_fd.revents & G_IO_IN) {
    if (!callback(user_data)) {
      notify_error();
    }
  }
  return G_SOURCE_CONTINUE;
}
