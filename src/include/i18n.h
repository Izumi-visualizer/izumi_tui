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

#ifndef I18N_H
#define I18N_H

#include <config.h>

#ifdef ENABLE_NLS
#  include <libintl.h>
#  define _(msgid) gettext(msgid)
#else
#  define _(msgid) ((char *) (msgid))
#  define textdomain(domain) ((void) (domain))
#  define bindtextdomain(domain, dir) ((void) (domain), (void) (dir))
#endif

/* Marks a string for extraction without translating it in place (for static
 * initializers); translate it with _() where it is used. */
#define N_(msgid) msgid

#endif
