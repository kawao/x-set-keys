/***************************************************************************
 *
 * Copyright (C) 2017-2018 Tomoyuki KAWAO
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

#include "common.h"
#include "config.h"
#include "action.h"

static gboolean _parse_line(XSetKeys *xsk,
                            KeyCombinationArray *inputs,
                            KeyCodeArrayArray *outputs,
                            gchar *line);
static gchar *_get_next_word(gchar **line_pointer);

gboolean config_load(XSetKeys *xsk, const gchar filepath[])
{
  gboolean result = TRUE;
  GError *error = NULL;
  GIOChannel *channel;
  gint line_number;
  KeyCombinationArray *inputs;
  KeyCodeArrayArray *outputs;
  // ------- open file
  channel = g_io_channel_new_file(filepath, "r", &error);
  if (!channel) {
    g_critical("Failed to open configuration file(%s): %s",
               filepath,
               error->message);
    g_error_free(error);
    return FALSE;
  }
  // -------- parse lines
  inputs = key_combination_array_new(6);
  outputs = key_code_array_array_new(6);

  for (line_number = 1; result; line_number++) {
    gchar *line;
    GIOStatus status = g_io_channel_read_line(channel,
                                              &line,
                                              NULL,
                                              NULL,
                                              &error);
    if (status == G_IO_STATUS_ERROR) {
      g_critical("Failed to read configuration file(%s) at line %d: %s",
                 filepath,
                 line_number,
                 error->message);
      g_error_free(error);
      result = FALSE;
      break;
    }
    if (status == G_IO_STATUS_EOF) {
      break;
    }
    // parse line!
    if (!_parse_line(xsk, inputs, outputs, line)) {
      g_critical("Configuration file(%s) error at line %d",
                 filepath,
                 line_number);
      result = FALSE;
    }
    g_free(line);
  }

  key_combination_array_free(inputs);
  key_code_array_array_free(outputs);
  // -------- close file
  if (g_io_channel_shutdown(channel, FALSE, &error) == G_IO_STATUS_ERROR) {
    g_critical("Failed to close configuration file(%s): %s",
               filepath,
               error->message);
    g_error_free(error);
  }

  if (result && !action_list_get_length(xsk_get_root_actions(xsk))) {
    g_critical("No data in configuration file: %s", filepath);
    result = FALSE;
  }

  return result;
}

/**
 * @inputs: empty array, left side of config line
 * @outputs: empty array, right side of config line
 */
static gboolean _parse_line(XSetKeys *xsk,
                            KeyCombinationArray *inputs,
                            KeyCodeArrayArray *outputs,
                            gchar *line)
{
  gchar *word;
  // -- comment or not
  word = _get_next_word(&line);
  if (!word || *word == '#') {
    return TRUE;
  }

  debug_print("Parsing: %s %s", word, line);

  // -- fill inputs, outputs with zeroes
  key_combination_array_clear(inputs);
  key_code_array_array_clear(outputs);

  // -- first parte before "::"
  do {
    KeyCombination kc;

    kc = ki_string_to_key_combination(xsk_get_display(xsk),
                                      xsk_get_key_information(xsk),
                                      word);
    if (key_combination_is_null(kc)) {
      return FALSE;
    }
    // common preparation
    key_combination_array_add(inputs, kc);

    word = _get_next_word(&line);
    if (!word) {
      return FALSE;
    }
  } while (strcmp(word, "::"));

  if (!key_combination_array_get_length(inputs)) {
    return FALSE;
  }
  // -- second part after "::"
  while ((word = _get_next_word(&line))) {
    KeyCodeArray *key_array;
    if (!strcmp(word, "$select")) {
      return action_list_add_select_action(xsk_get_root_actions(xsk), inputs);
    }
    gboolean is_start = !strcmp(word, "$start");
    if (is_start) {
      return action_list_add_startstop_action(xsk_get_root_actions(xsk), inputs, is_start);
    }
    if (!strcmp(word, "$stop")) {
      return action_list_add_startstop_action(xsk_get_root_actions(xsk), inputs, is_start);
    }
    key_array = ki_string_to_key_code_array(xsk_get_display(xsk),
                                            xsk_get_key_information(xsk),
                                            word);
    if (!key_array) {
      return FALSE;
    }
    // common preparation
    key_code_array_array_add(outputs, key_array);
  }

  if (!key_code_array_array_get_length(outputs)) {
    return FALSE;
  }
  return action_list_add_key_action(xsk_get_root_actions(xsk), inputs, outputs);
}

static gchar *_get_next_word(gchar **line_pointer)
{
  gchar *result;
  gchar *pointer = *line_pointer;

  for ( ; *pointer; pointer++) {
    if (!g_ascii_isspace(*pointer)) {
      break;
    }
  }
  if (!*pointer) {
    return NULL;
  }

  result = pointer;
  for ( ; *pointer; pointer++) {
    if (g_ascii_isspace(*pointer)) {
      break;
    }
  }
  if (*pointer) {
    *pointer++ = '\0';
  }
  *line_pointer = pointer;
  return result;
}
