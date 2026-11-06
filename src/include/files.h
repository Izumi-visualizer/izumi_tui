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

#ifndef FILES_H
#define FILES_H

#include <sys/types.h>
#include <stdbool.h>

#include <izumi/data_structs.h>

struct FileData_s {
    bool exists;
    bool is_file;
};

typedef struct FileData_s FileData;

/*
 * This function checks if a file exists and if it is a file.
 *
 * path: The path of the file.
 *
 */
FileData check_file(const char *path);

/* Expand ~ and ~username using HOME and the user database. The caller owns
 * the returned string. Unknown users, missing HOME and allocation errors
 * return NULL; ordinary paths are copied unchanged. */
char *expand_path(const char *path);

/*
 * This function reads a file and stores the data in the tables array.
 *
 * path: The path of the file.
 * tables_array: The array of tables.
 */
char *read_file(const char *path, InstructionTableArray *tables_array);

/*
 * This function returns a newly allocated copy of the file's basename.
 */
char *file_basename(const char *path);

/*
 * Parse a trace and merge its stages into an existing tables array. Stages are
 * tagged with the given source index so they remain distinguishable from the
 * primary trace even when lane IDs collide. Instructions missing from the
 * primary trace are created with the attached trace's labels.
 *
 * path: The path of the attached trace.
 * tables_array: The array of tables of the open trace.
 * source: Source index (1 for the first attached file, 2 for the next, ...).
 */
void attach_trace(const char *path, InstructionTableArray *tables_array,
                  uint32_t source);

#endif
