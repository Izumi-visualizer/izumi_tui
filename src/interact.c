/*
 * This file is part of Izumi.
 *
 * Izumi is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Izumi is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Izumi. If not, see <https://www.gnu.org/licenses/>.
 */

#include "system_curses.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interact.h"
#include "window.h"
#include "command_tree.h"
#include "error.h"
#include "autocomplete.h"

bool parse_input(ApplicationData *app_data, int ch) {
    // Terminal resized: nothing to do, the caller re-renders with the new size
    if (ch == KEY_RESIZE) return true;

    if (app_data->mode == NORMAL) {
        switch (ch) {
            case 'j':
            case KEY_DOWN:
            case 'k':
            case KEY_UP:
                for (uint64_t i = 0; i < app_data->windows_qtty; ++i) {
                    if (!app_data->windows_synced && i != app_data->window_focused) continue;
                    WindowData *win = app_data->windows[i];
                    bool forward = ch == 'j' || ch == KEY_DOWN;
                    uint64_t count = app_data->number == 0 ? 1 : app_data->number;
                    win->first_subrow = 0;
                    if (!app_data->config.show_empty_rows)
                        move_visible_instruction(win, forward, count);
                    else if (forward)
                        win->first_instruction += count > UINT64_MAX-win->first_instruction ?
                            UINT64_MAX-win->first_instruction : count;
                    else
                        win->first_instruction = count > win->first_instruction ?
                            0 : win->first_instruction-count;
                }
                break;
            case 'J':
            case 'K':
            case KEY_NPAGE:
            case KEY_PPAGE:
                for (uint64_t i = 0; i < app_data->windows_qtty; ++i) {
                    if (!app_data->windows_synced && i != app_data->window_focused) continue;
                    WindowData *win = app_data->windows[i];
                    uint64_t count = app_data->number == 0 ? 1 : app_data->number;
                    if (ch == KEY_NPAGE || ch == KEY_PPAGE)
                        count = win->height > 3 ? win->height-3 : 1;
                    scroll_instruction_rows(win, ch == 'J' || ch == KEY_NPAGE,
                        count, app_data->config.show_empty_rows);
                }
                break;
            case ':':
                app_data->mode = COMMAND;

                if (app_data->command != NULL) {
                    free(app_data->command);
                }

                app_data->command = malloc(1);
                app_data->command[0] = '\0';
                break;
            case 'n':
                if (app_data->command != NULL) {
                    free(app_data->command);
                }
                app_data->command = malloc(5*sizeof(char));
                strcpy(app_data->command, "next");
                app_data->error = run_command(app_data);
                break;
            case 'N':
                if (app_data->command != NULL) {
                    free(app_data->command);
                }
                app_data->command = malloc(5*sizeof(char));
                strcpy(app_data->command, "prev");
                app_data->error = run_command(app_data);
                break;

            default:
                break;
        }

        switch (ch) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                app_data->number *= 10;
                app_data->number += ch - '0';
                break;

            default:
                app_data->number = 0;
                break;
        }
    }
    else if (app_data->mode == COMMAND) {
        int len;
        switch (ch) {
            case 27: // Escape
                app_data->mode = NORMAL;
                break;
            case '\n': // Enter
                app_data->error = run_command(app_data);
                app_data->mode = NORMAL;
                break;
            case KEY_BACKSPACE: // Backspace
                if (strlen(app_data->command) > 0) {
                    int len = strlen(app_data->command);

                    app_data->command = realloc(app_data->command, sizeof(char) * len);
                    app_data->command[len - 1] = '\0';
                }
                break;
            case '\t': // Tab - accept autocomplete if available
                if (accept_autocomplete_suggestion(app_data)) {
                    /* suggestion accepted, do nothing else */
                }
                break;
            default:
                len = strlen(app_data->command);

                app_data->command = realloc(app_data->command, sizeof(char) * (len + 2));
                app_data->command[len + 1] = '\0';
                app_data->command[len] = ch;
                break;
        }
    }

    return true;
}
