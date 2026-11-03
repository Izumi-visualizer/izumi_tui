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

#include <izumi/parser.h>

#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <limits.h>
#include <libgen.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "files.h"


FileData check_file(const char *path) {
    FileData file_data = { .exists = false };
    
    if (path == NULL) return file_data;

    struct stat s;
    if (stat(path, &s) == 0) {
        file_data.exists = true;
        file_data.is_file = S_ISREG(s.st_mode);
    }

    return file_data;
}

char *file_basename(const char *path) {
    // basename() may modify the input string, so we need to make a copy
    char *scratchpad_path = strdup(path);
    char *filename_basename = basename(scratchpad_path);
    char *filename = strdup(filename_basename);
    free(scratchpad_path);
    return filename;
}

char *read_file(const char * path, InstructionTableArray *tables_array) {
    FileData file_data = check_file(path);

    if (file_data.exists && file_data.is_file) {
        free_InstructionTableArray(tables_array);

        *tables_array = parse_file(path);

        return file_basename(path);
    }

    return NULL;
}

static bool instruction_exists(InstructionTableArray *array, uint64_t id) {
    return array->tables != NULL && id/256 < array->qtty_tables &&
           array->tables[id/256] != NULL &&
           array->tables[id/256]->content[id%256].valid;
}

static void append_stage(Instruction *instruction, const Stage *stage,
                         uint32_t source) {
    if (instruction->qtty_stages % 10 == 0)
        instruction->stages = realloc(instruction->stages,
            (instruction->qtty_stages + 10) * sizeof(Stage));

    Stage *copy = &instruction->stages[instruction->qtty_stages++];
    *copy = *stage;
    copy->source = source;
    copy->name = stage->name ? strdup(stage->name) : NULL;
}

void attach_trace(const char *path, InstructionTableArray *tables_array,
                  uint32_t source) {
    InstructionTableArray attached = parse_file(path);

    for (uint64_t t = 0; t < attached.qtty_tables; ++t) {
        if (attached.tables[t] == NULL) continue;
        for (uint64_t j = 0; j < 256; ++j) {
            Instruction *from = &attached.tables[t]->content[j];
            if (!from->valid) continue;

            uint64_t id = t*256 + j;
            bool existed = instruction_exists(tables_array, id);
            Instruction *to = get_instruction(tables_array, id);

            if (!existed) {
                to->mem_addr = from->mem_addr ? strdup(from->mem_addr) : NULL;
                to->instruction = from->instruction ? strdup(from->instruction) : NULL;
                to->finished = from->finished;
                to->flushed = from->flushed;
            }

            for (uint64_t k = 0; k < from->qtty_stages; ++k)
                append_stage(to, &from->stages[k], source);
        }
    }

    free_InstructionTableArray(&attached);
}
