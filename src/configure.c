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

#include "configure.h"
#include "command_tree.h"
#include "window.h"

#include <linux/limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

char *get_config_path() {
    const char *home = getenv("HOME");
    const char suffix[] = "/.config/izumi/config";
    if (home == NULL || strlen(home) > PATH_MAX - sizeof(suffix)) return NULL;
    char *path = malloc(strlen(home) + sizeof(suffix));
    if (path == NULL) return NULL;
    strcpy(path, home);
    strcat(path, suffix);
    return path;
}

char **read_config_file(char *path) {
    if (path == NULL) return NULL;
    FILE *file = fopen(path, "r");
    free(path);
    if (file == NULL) return NULL;

    size_t capacity = 10, count = 0;
    char **commands = malloc(capacity * sizeof(*commands));
    char *line = NULL;
    size_t len = 0;
    if (commands == NULL) { fclose(file); return NULL; }
    while (getline(&line, &len, file) != -1) {
        size_t length = strlen(line);
        while (length > 0 && (line[length-1] == '\r' || line[length-1] == '\n'))
            line[--length] = '\0';
        char *start = line;
        while (isspace((unsigned char)*start)) ++start;
        if (*start == '\0' || *start == '#') continue;
        if (count + 1 == capacity) {
            if (capacity > SIZE_MAX / sizeof(*commands) / 2) goto failure;
            char **grown = realloc(commands, capacity * 2 * sizeof(*commands));
            if (grown == NULL) goto failure;
            commands = grown;
            capacity *= 2;
        }
        commands[count] = strdup(start);
        if (commands[count] == NULL) goto failure;
        ++count;
    }
    if (ferror(file)) goto failure;
    free(line);
    fclose(file);
    commands[count] = NULL;
    return commands;

failure:
    for (size_t i = 0; i < count; ++i) free(commands[i]);
    free(commands);
    free(line);
    fclose(file);
    return NULL;
}

bool execute_config_commands(ApplicationData *app_data, char **commands) {
    if (commands == NULL) return false;
    bool success = true;

    for (int i = 0; commands[i] != NULL; i++) {
        char *command = commands[i];

        if (app_data->command != NULL) {
            free(app_data->command);
        }

        app_data->command = malloc(strlen(command) + 1);
        if (app_data->command == NULL) {
            for (int j = 0; commands[j] != NULL; j++) {
                free(commands[j]);
            }
            free(commands);
            return false; // Memory allocation failed
        }
        strcpy(app_data->command, command);

        // Execute the command
        ErrorKind error = run_command(app_data);
        if (error != NO_ERROR) {
            app_data->error = error;
            success = false;
        }
    }

    for (int j = 0; commands[j] != NULL; j++) {
        free(commands[j]);
    }
    free(commands);

    return success;
}
