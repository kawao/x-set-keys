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

#ifndef _DEVICE_H
#define _DEVICE_H

#include "glib.h"

typedef struct _Device {
  GSource source;
  GPollFD poll_fd;
} Device;

Device *device_initialize(gint fd,
                          const char *name,
                          GSourceFunc callback,
                          gpointer user_data);
void device_finalize(Device *device);

#define device_get_fd(device) ((device)->poll_fd.fd)

gssize device_read(Device *device, gpointer buffer, gsize length);
gboolean device_write(Device *device, gconstpointer buffer, gsize length);

#endif  /* _DEVICE_H */
