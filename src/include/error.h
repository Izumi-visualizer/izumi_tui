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

#ifndef ERROR_H
#define ERROR_H

#include "i18n.h"

enum ErrorKind_e {
    ERORR_WRONG_COMMAND = 0,
    ERROR_EMPTY_COMMAND,
    ERROR_WRONG_AMOUNT_ARGS,
    ERROR_WRONG_ARGS,
    ERROR_SUBCOMMAND_EMPTY,
    ERROR_ON_EXE,
    ERROR_FILE_NOT_EXISTS,
    ERROR_NOT_A_FILE,
    ERROR_NO_WINDOW,
    ERROR_NO_TRACE_OPEN,
    ERROR_NO_RESULT,
    ERROR_IDX_BIGGER,

    ERRORS_AMOUNT,
    NO_ERROR
};

typedef enum ErrorKind_e ErrorKind;

/* Translated with _() where displayed. */
static char *error_msg[ERRORS_AMOUNT] = {
    N_("Wrong command"),
    N_("Empty command"),
    N_("Wrong amount of arguments on the command"),
    N_("Wrong arguments on the command"),
    N_("Command with subcommands empty"),
    N_("Error on execution of command"),
    N_("File does not exist"),
    N_("File is not a file"),
    N_("No window created"),
    N_("No trace open on the focused panel"),
    N_("No result on the search"),
    N_("Index is bigger than array"),
};

#endif
