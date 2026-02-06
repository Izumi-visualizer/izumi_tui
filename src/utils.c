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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#include "utils.h"

bool parse_u64(const char *s, uint64_t *out) {
    char *endptr;
    errno = 0;

    if (s == NULL || *s == '\0') {
        return false;
    }

    unsigned long long tmp = strtoull(s, &endptr, 10);

    /* No se convirtió nada */
    if (endptr == s) {
        return false;
    }

    /* Overflow / underflow */
    if (errno == ERANGE) {
        return false;
    }

    /* Caracteres sobrantes */
    if (*endptr != '\0') {
        return false;
    }

    *out = (uint64_t)tmp;
    return true;
}

