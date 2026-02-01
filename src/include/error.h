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

enum ErrorKind_e {
    ERORR_WRONG_COMMAND = 0,
    ERROR_EMPTY_COMMAND,
    ERROR_ON_EXE,

    ERRORS_AMOUNT,
    NO_ERROR
};

typedef enum ErrorKind_e ErrorKind;

static char *error_msg[ERRORS_AMOUNT] = {
    "Wrong command",
    "Empty command",
    "Error on execution of command",
};

#endif
