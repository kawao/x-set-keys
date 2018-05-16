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

#ifndef _COMMON_H
#define _COMMON_H

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

#ifndef MAIN
extern const
#endif
gboolean is_debug;

#define debug_print(format, ...) if (is_debug)                          \
    (g_debug("%s:%d (%s) " format,                                      \
             __FILE__, __LINE__, G_STRFUNC, ##__VA_ARGS__), fflush(stdout))

#define print_error(format, ...)                                        \
  (errno ? g_critical(format " : %s", ##__VA_ARGS__, strerror(errno))   \
   : g_critical(format, ##__VA_ARGS__))

#define array_num(array) (sizeof(array)/sizeof(*array))

void notify_error();

#endif /* _COMMON_H */
