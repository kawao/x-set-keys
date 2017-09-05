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

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "fcitx.h"

#define _BUS_NAME "org.fcitx.Fcitx"
#define _OBJECT_PATH "/inputmethod"

static void _handle_name_appeared(GDBusConnection *connection,
                                  const gchar *name,
                                  const gchar *name_owner,
                                  gpointer user_data);
static void _handle_name_vanished(GDBusConnection *connection,
                                  const gchar *name,
                                  gpointer user_data);
static void _handle_signal(GDBusConnection *connection,
                           const gchar *sender_name,
                           const gchar *object_path,
                           const gchar *interface_name,
                           const gchar *signal_name,
                           GVariant *parameters,
                           gpointer user_data);
static void _update(GDBusConnection *connection, XSetKeys *xsk);
static gboolean _get_is_excluded(gchar **excluded_input_methods,
                                 const gchar *current_input_method);

Fcitx *fcitx_initialize(XSetKeys *xsk, gchar *excluded_input_methods[])
{
  Fcitx *fcitx;
  GError *error = NULL;
  const gchar *uid_string;
  uid_t original_euid;

  uid_string = g_getenv("SUDO_UID");
  if (!uid_string) {
    g_critical("You must run %s with sudo", g_get_prgname());
    return NULL;
  }

  original_euid = geteuid();
  if (seteuid(atoi(uid_string)) < 0) {
    print_error("seteuid(%s) failed", uid_string);
    return NULL;
  }

  fcitx = g_new0(Fcitx, 1);
  fcitx->excluded_input_methods = excluded_input_methods;
  fcitx->connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
  if (error) {
    g_critical("g_bus_get_sync failed: %s", error->message);
    g_critical("Maybe you need to take over the environment variable"
               " DBUS_SESSION_BUS_ADDRESS before sudo");
    g_error_free(error);
    g_free(fcitx);
    return NULL;
  }

  if (seteuid(original_euid) < 0) {
    print_error("seteuid(%d) failed", original_euid);
    g_free(fcitx);
    return NULL;
  }

  fcitx->watch_id =
    g_bus_watch_name_on_connection(fcitx->connection,
                                   _BUS_NAME,
                                   G_BUS_NAME_WATCHER_FLAGS_NONE,
                                   _handle_name_appeared,
                                   _handle_name_vanished,
                                   xsk,
                                   NULL);
  return fcitx;
}

void fcitx_finalize(XSetKeys *xsk)
{
  Fcitx *fcitx = xsk_get_fcitx(xsk);

  if (fcitx->subscription_id) {
    g_dbus_connection_signal_unsubscribe(fcitx->connection,
                                         fcitx->subscription_id);
  }
  g_bus_unwatch_name(fcitx->watch_id);
  g_object_unref(fcitx->connection);
  g_free(fcitx);
}

static void _handle_name_appeared(GDBusConnection *connection,
                                  const gchar *name,
                                  const gchar *name_owner,
                                  gpointer user_data)
{
  XSetKeys *xsk = user_data;
  Fcitx *fcitx = xsk_get_fcitx(xsk);

  g_message("Name appeared: name=%s, name_owner=%s", name, name_owner);

  fcitx->subscription_id =
    g_dbus_connection_signal_subscribe(connection,
                                       name_owner,
                                       "org.freedesktop.DBus.Properties",
                                       "PropertiesChanged",
                                       _OBJECT_PATH,
                                       NULL,
                                       G_DBUS_SIGNAL_FLAGS_NONE,
                                       _handle_signal,
                                       user_data,
                                       NULL);
  /* Somehow does not work well at automatic startup */
  /* _update(connection, xsk); */
}

static void _handle_name_vanished(GDBusConnection *connection,
                                  const gchar *name,
                                  gpointer user_data)
{
  XSetKeys *xsk = user_data;
  Fcitx *fcitx = xsk_get_fcitx(xsk);

  g_warning("Name vanished: name=%s", name);

  if (fcitx->subscription_id) {
    g_dbus_connection_signal_unsubscribe(connection, fcitx->subscription_id);
    fcitx->subscription_id = 0;
    fcitx->is_excluded = FALSE;
  }
}

static void _handle_signal(GDBusConnection *connection,
                           const gchar *sender_name,
                           const gchar *object_path,
                           const gchar *interface_name,
                           const gchar *signal_name,
                           GVariant *parameters,
                           gpointer user_data)
{
  _update(connection, user_data);
}

static void _update(GDBusConnection *connection, XSetKeys *xsk)
{
  Fcitx *fcitx = xsk_get_fcitx(xsk);
  GVariant *result;
  GError *error = NULL;
  gchar *current_input_method = NULL;

  result = g_dbus_connection_call_sync(connection,
                                       _BUS_NAME,
                                       _OBJECT_PATH,
                                       _BUS_NAME ".InputMethod",
                                       "GetCurrentIM",
                                       NULL,
                                       NULL,
                                       G_DBUS_CALL_FLAGS_NONE,
                                       -1,
                                       NULL,
                                       &error);
  if (error) {
    g_critical("Failed g_dbus_connection_call_sync(GetCurrentIM): %s",
               error->message);
    g_error_free(error);
    return;
  }

  g_variant_get(result, "(s)", &current_input_method);
  if (!current_input_method) {
    gchar *result_string = g_variant_print(result, TRUE);

    g_critical("Unexpected result: g_dbus_connection_call_sync(GetCurrentIM):"
               " %s",
               result_string);
    g_free(result_string);
  } else {
    gboolean is_excluded = _get_is_excluded(fcitx->excluded_input_methods,
                                            current_input_method);

    if (is_excluded && !xsk_is_excluded(xsk)) {
      xsk_reset_state(xsk);
    }
    fcitx->is_excluded = is_excluded;

    debug_print("Input method changed: %s, excluded: %s",
                current_input_method,
                is_excluded ? "true" : "false");
    g_free(current_input_method);
  }

  g_variant_unref(result);
}

static gboolean _get_is_excluded(gchar **excluded_input_methods,
                                 const gchar *current_input_method)
{
  for (; *excluded_input_methods; excluded_input_methods++) {
    if (!g_strcmp0(*excluded_input_methods, current_input_method)) {
      return TRUE;
    }
  }
  return FALSE;
}
