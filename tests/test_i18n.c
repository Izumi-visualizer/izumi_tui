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

/* Spanish and Catalan translations, using the catalogs compiled from po/ into
 * TEST_LOCALEDIR. Skipped (exit 77) when translations are disabled, msgfmt was
 * not available, or the es_ES.UTF-8 / ca_ES.UTF-8 locales are not installed. */

#include <locale.h>

#include "test.h"
#include "i18n.h"
#include "error.h"
#include "window.h"

#define SKIP 77

static FILE *input, *output;
static SCREEN *screen;

static void begin_screen(void) {
    input = tmpfile();
    output = tmpfile();
    CHECK(input != NULL && output != NULL);
    screen = newterm("xterm", output, input);
    CHECK(screen != NULL);
    set_term(screen);
    CHECK(resizeterm(30, 100) == OK);
    start_color();
}

static bool use_locale(const char *locale) {
    if (setlocale(LC_ALL, locale) == NULL) return false;
    /* Same as main(): numbers stay locale-independent. */
    setlocale(LC_NUMERIC, "C");
    return true;
}

static void status_line(char *text, int length) {
    CHECK(mvinnstr(getmaxy(stdscr)-1, 0, text, length) != ERR);
}

static void window_line(WindowData *win, int y, char *text, int length) {
    CHECK(mvwinnstr(win->win, y, 1, text, length) != ERR);
}

static void all_errors_translated(void) {
    for (unsigned i = 0; i < ERRORS_AMOUNT; ++i)
        CHECK(strcmp(_(error_msg[i]), error_msg[i]) != 0);
}

static void spanish(void) {
    CHECK(use_locale("es_ES.UTF-8"));
    all_errors_translated();
    CHECK(strcmp(_("Wrong command"), "Comando incorrecto") == 0);
    CHECK(strcmp(_("on"), "sí") == 0 && strcmp(_("off"), "no") == 0);

    ApplicationData app = {.config = {.bar_offset = 32, .stage_width = 3,
                                       .show_empty_rows = true}, .error = NO_ERROR};
    new_window(&app);
    WindowData *win = app.windows[0];
    get_instruction(win->tables_array, 0);
    line_of_data(win->tables_array, "L\t0\t0\t0x10: add a0");
    new_stage(win->tables_array, 0, "S\t0\t0\tF");
    new_stage(win->tables_array, 0, "S\t0\t1\tM");
    retire_instruction(win->tables_array, 2, "R\t0\t0\t0");
    CHECK(command(&app, "ctime 123456") == NO_ERROR);
    render(&app);

    char text[100];
    window_line(win, 2, text, 20);
    CHECK(strstr(text, "Carril 0") != NULL);
    window_line(win, win->height - 2, text, 60);
    CHECK(strstr(text, "temporal 0, ciclo 123456") != NULL);

    status_line(text, 40);
    CHECK(strstr(text, " NORMAL ") == text && strstr(text, "Filas vac") != NULL);

    /* Errors replace the "empty rows" indicator after the mode name. */
    app.error = ERORR_WRONG_COMMAND; /* sic, enum name in error.h */
    render_status_bar(&app);
    status_line(text, 40);
    CHECK(strncmp(text + 10, "Comando incorrecto", 18) == 0);

    /* The command line starts after the translated mode name. */
    app.mode = COMMAND;
    free(app.command);
    app.command = strdup("set");
    render_status_bar(&app);
    status_line(text, 40);
    CHECK(strncmp(text, " COMANDO ", 9) == 0 && strncmp(text + 11, ":set", 4) == 0);

    free(app.command);
    app.command = NULL;
    close_all_panels(&app);
}

static void catalan(void) {
    CHECK(use_locale("ca_ES.UTF-8"));
    all_errors_translated();
    CHECK(strcmp(_("Wrong command"), "Ordre incorrecta") == 0);

    ApplicationData app = {.config = {.bar_offset = 32, .stage_width = 3,
                                       .show_empty_rows = false}, .error = NO_ERROR};
    app.mode = COMMAND;
    app.command = strdup("q");
    render_status_bar(&app);
    char text[40];
    status_line(text, 20);
    /* "ORDRE" is shorter than "COMMAND", so the ':' moves left. */
    CHECK(strncmp(text, " ORDRE ", 7) == 0 && strncmp(text + 9, ":q", 2) == 0);

    app.mode = NORMAL;
    app.error = ERROR_WRONG_AMOUNT_ARGS;
    render_status_bar(&app);
    status_line(text, 40);
    CHECK(strncmp(text + 10, "Nombre incorrecte d'arguments", 29) == 0);
    free(app.command);
}

static void untranslated_locale(void) {
    CHECK(use_locale("C"));
    for (unsigned i = 0; i < ERRORS_AMOUNT; ++i)
        CHECK(strcmp(_(error_msg[i]), error_msg[i]) == 0);
    CHECK(strcmp(_("COMMAND"), "COMMAND") == 0);
}

int main(void) {
#ifndef ENABLE_NLS
    puts("SKIP: built with --disable-nls");
    return SKIP;
#else
    if (access(TEST_LOCALEDIR "/es/LC_MESSAGES/" PACKAGE ".mo", R_OK) != 0 ||
        access(TEST_LOCALEDIR "/ca/LC_MESSAGES/" PACKAGE ".mo", R_OK) != 0) {
        puts("SKIP: translations were not compiled (msgfmt missing)");
        return SKIP;
    }
    if (!use_locale("es_ES.UTF-8") || !use_locale("ca_ES.UTF-8")) {
        puts("SKIP: es_ES.UTF-8 or ca_ES.UTF-8 locale not installed");
        return SKIP;
    }
    CHECK(bindtextdomain(PACKAGE, TEST_LOCALEDIR) != NULL);
    CHECK(textdomain(PACKAGE) != NULL);

    begin_screen();
    RUN(spanish);
    RUN(catalan);
    RUN(untranslated_locale);
    endwin();
    delscreen(screen);
    CHECK(fclose(input) == 0 && fclose(output) == 0);
    return EXIT_SUCCESS;
#endif
}
