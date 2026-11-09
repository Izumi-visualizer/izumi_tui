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
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <sys/types.h>
#include <linux/limits.h>

#include "config.h"
#include "window.h"
#include "files.h"
#include "i18n.h"

static void print_help(const char *program) {
    printf(_("Usage: %s [OPTION]... [FILE]...\n"), program);
    fputs(_("Open one or more instruction traces in an interactive TUI.\n"), stdout);
    fputs("\n", stdout);
    fputs(_("  -h, --help     display this help and exit\n"
            "  -v, --version  display version information and exit\n"), stdout);
}

int main(int argc, char *argv[]) {
    ApplicationData app_data;

    setlocale(LC_ALL, "");
    /* Keep number parsing and formatting locale-independent. */
    setlocale(LC_NUMERIC, "C");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        }

        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("izumi %s\n", VERSION);
            return 0;
        }
    }

    init_application(&app_data);

    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            new_window(&app_data);

            char *path = realpath(argv[i], NULL);

            FileData file_data = check_file(path);

            if (file_data.exists && file_data.is_file) {
                WindowData *window = app_data.windows[app_data.window_focused];
                window->filename = read_file(argv[i], window->tables_array);
            }

            free(path);
            path = NULL;
        }
    }

    /*InstructionTableArray *tables_array = app_data.windows[0]->tables_array;

    for (uint64_t i = 0; i < tables_array->qtty_tables; ++i) {
        if (tables_array->tables[i] != NULL) {
            for (uint64_t j = 0; j < 256; ++j) {
                if (tables_array->tables[i]->content[j].valid) {
                    if (tables_array->tables[i]->content[j].instruction != NULL) {
                        fprintf(stderr, "%s\n", tables_array->tables[i]->content[j].instruction);
                    }
                }
            }
        }
        }*/

    main_loop(&app_data);

    close_application(&app_data);
}
