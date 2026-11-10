#include <locale.h>

#include "test.h"
#include "window.h"
#include "interact.h"

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

static void cleanup_app(ApplicationData *app) {
    close_all_panels(app);
    free(app->command);
    app->command = NULL;
}

static void panel_lifecycle(void) {
    ApplicationData app = {0};
    new_window(&app);
    new_window(&app);
    CHECK(app.windows_qtty == 2 && app.window_focused == 1);
    CHECK(app.windows[0]->height == 29 && app.windows[1]->height == 14);
    close_panel(&app, 0);
    CHECK(app.windows_qtty == 1 && app.window_focused == 0 && app.windows[0]->index == 0);
    new_window(&app);
    app.window_focused = 0;
    close_panel(&app, 0);
    CHECK(app.windows_qtty == 1 && app.window_focused == 0);
    CHECK(command(&app, "closepanel 9") == ERROR_IDX_BIGGER);
    CHECK(command(&app, "closepanel 0 1") == ERROR_WRONG_AMOUNT_ARGS);
    CHECK(command(&app, "closepanel") == NO_ERROR);
    CHECK(app.windows_qtty == 0 && app.windows == NULL && app.window_focused == 0);
    CHECK(command(&app, "closepanel") == ERROR_NO_WINDOW);
    new_window(&app);
    new_window(&app);
    // Close the focused last panel while an earlier panel remains.
    close_panel(&app, 1);
    CHECK(app.windows_qtty == 1 && app.window_focused == 0);
    close_all_panels(&app);
    new_window(&app);
    new_window(&app);
    close_all_panels(&app);
    CHECK(app.window_focused == 0 && app.windows_qtty == 0);
    cleanup_app(&app);
}

static void populate(WindowData *win) {
    get_instruction(win->tables_array, 0);
    line_of_data(win->tables_array, "L\t0\t0\t0x10: add a0");
    new_stage(win->tables_array, 10, "S\t0\t0\tF");
    new_stage(win->tables_array, 10, "S\t0\t7\tstall");
    retire_instruction(win->tables_array, 13, "R\t0\t0\t0");
    get_instruction(win->tables_array, 256);
    line_of_data(win->tables_array, "L\t256\t0\t0x20: add a1");
    new_stage(win->tables_array, 12, "S\t256\t0\tX");
    retire_instruction(win->tables_array, 14, "R\t256\t0\t1");
    get_instruction(win->tables_array, 1025);
    new_stage(win->tables_array, 14, "S\t1025\t0\tExecute");
}

static void navigation_and_input(void) {
    ApplicationData app = {.config = {.show_empty_rows = false}};
    new_window(&app);
    WindowData *win = app.windows[0];
    populate(win);
    move_visible_instruction(win, true, 1);
    CHECK(win->first_instruction == 256);
    move_visible_instruction(win, true, UINT64_MAX);
    CHECK(win->first_instruction == 1025);
    move_visible_instruction(win, false, 1);
    CHECK(win->first_instruction == 256);
    move_visible_instruction(win, false, UINT64_MAX);
    CHECK(win->first_instruction == 0);
    scroll_instruction_rows(win, true, 1, false);
    CHECK(win->first_instruction == 0 && win->first_subrow == 1);
    scroll_instruction_rows(win, true, 2, false);
    CHECK(win->first_instruction == 256 && win->first_subrow == 0);
    scroll_instruction_rows(win, false, 1, false);
    CHECK(win->first_instruction == 0 && win->first_subrow == 2);
    win->first_instruction = 0;
    win->first_subrow = 0;
    CHECK(parse_input(&app, '2') && parse_input(&app, 'j'));
    CHECK(win->first_instruction == 1025 && app.number == 0);
    parse_input(&app, 'k');
    CHECK(win->first_instruction == 256);
    app.config.show_empty_rows = true;
    win->first_instruction = UINT64_MAX - 1;
    app.number = 10;
    parse_input(&app, 'j');
    CHECK(win->first_instruction == UINT64_MAX);
    app.number = UINT64_MAX;
    parse_input(&app, 'k');
    CHECK(win->first_instruction == 0);
    app.number = UINT64_MAX / 10;
    parse_input(&app, '9');
    CHECK(app.number == UINT64_MAX);
    parse_input(&app, ':');
    CHECK(app.mode == COMMAND && strcmp(app.command, "") == 0);
    parse_input(&app, 'q');
    parse_input(&app, KEY_LEFT);
    parse_input(&app, ERR);
    CHECK(strcmp(app.command, "q") == 0);
    parse_input(&app, 127);
    CHECK(strcmp(app.command, "") == 0);
    parse_input(&app, KEY_BACKSPACE);
    parse_input(&app, 'q');
    parse_input(&app, '\n');
    CHECK(app.quit_requested && app.mode == NORMAL && app.error == NO_ERROR);
    parse_input(&app, ':');
    parse_input(&app, 27);
    CHECK(app.mode == NORMAL);
    win->first_instruction = 0;
    app.config.show_empty_rows = false;
    parse_input(&app, 'J');
    CHECK(win->first_subrow == 1);
    parse_input(&app, 'K');
    CHECK(win->first_subrow == 0);
    parse_input(&app, KEY_NPAGE);
    parse_input(&app, KEY_PPAGE);
    parse_input(&app, KEY_UP);
    parse_input(&app, KEY_DOWN);
    parse_input(&app, 'n');
    CHECK(app.error == ERROR_NO_RESULT);
    parse_input(&app, 'N');
    CHECK(app.error == ERROR_NO_RESULT);
    parse_input(&app, ':');
    const char *partial = "set show_empty_rows of";
    for (const char *p = partial; *p; ++p) parse_input(&app, *p);
    parse_input(&app, '\t');
    CHECK(strcmp(app.command, "set show_empty_rows off") == 0);
    parse_input(&app, KEY_ENTER);
    CHECK(app.mode == NORMAL && !app.config.show_empty_rows);
    new_window(&app);
    app.window_focused = 0;
    app.windows_synced = true;
    app.config.show_empty_rows = true;
    win->first_instruction = 0;
    parse_input(&app, 'j');
    CHECK(win->first_instruction == 1 && app.windows[1]->first_instruction == 1);
    uint64_t before = win->first_instruction;
    parse_input(&app, KEY_RESIZE);
    CHECK(win->first_instruction == before);
    cleanup_app(&app);
}

static void searches_and_trace_commands(void) {
    ApplicationData app = {0};
    new_window(&app);
    WindowData *win = app.windows[0];
    CHECK(command(&app, "attach missing") == ERROR_NO_TRACE_OPEN);
    CHECK(command(&app, "next") == ERROR_NO_RESULT);
    CHECK(command(&app, "prev") == ERROR_NO_RESULT);
    populate(win);
    CHECK(command(&app, "findinst add") == NO_ERROR && win->first_instruction == 0);
    CHECK(command(&app, "findinst add") == NO_ERROR);
    CHECK(command(&app, "next") == NO_ERROR && win->first_instruction == 256);
    CHECK(command(&app, "next") == ERROR_NO_RESULT);
    CHECK(command(&app, "prev") == NO_ERROR && win->first_instruction == 0);
    CHECK(command(&app, "prev") == ERROR_NO_RESULT);
    CHECK(command(&app, "findpc 0x0000000000000020") == NO_ERROR && win->first_instruction == 256);
    CHECK(command(&app, "findinst absent") == ERROR_NO_RESULT);
    new_window(&app);
    CHECK(command(&app, "panelcmd k") == NO_ERROR && app.window_focused == 0);
    CHECK(command(&app, "panelcmd j") == NO_ERROR && app.window_focused == 1);
    char *path = trace_file("Kanata\t0004\nI\t0\t0\t0\nS\t0\t1\tF\nC\t1\nR\t0\t0\t0\n");
    char text[256];
    snprintf(text, sizeof(text), "open %s", path);
    CHECK(command(&app, text) == NO_ERROR && app.windows[1]->filename != NULL);
    snprintf(text, sizeof(text), "attach %s", path);
    CHECK(command(&app, text) == NO_ERROR && app.windows[1]->lane_sources_amount == 1);
    CHECK(get_instruction(app.windows[1]->tables_array, 0)->qtty_stages == 2);
    snprintf(text, sizeof(text), "open %s", path);
    CHECK(command(&app, text) == NO_ERROR && app.windows[1]->lane_sources_amount == 0);
    CHECK(command(&app, "open /izumi-missing-test-path") == ERROR_FILE_NOT_EXISTS);
    CHECK(command(&app, "open .") == ERROR_NOT_A_FILE);
    CHECK(command(&app, "attach .") == ERROR_NOT_A_FILE);
    CHECK(command(&app, "attach /izumi-missing-test-path") == ERROR_FILE_NOT_EXISTS);
    app.windows[1]->first_instruction = 1025;
    app.windows[1]->first_cycle = 100;
    app.windows[1]->last_search.pattern = strdup("old search");
    snprintf(text, sizeof(text), "open %s", path);
    CHECK(command(&app, text) == NO_ERROR && app.windows[1]->first_instruction == 0);
    CHECK(app.windows[1]->first_cycle == 0 && app.windows[1]->last_search.pattern == NULL);
    CHECK(command(&app, "set color text red blue bold") == NO_ERROR);
    short fg, bg;
    CHECK(pair_content(COLOR_TEXT + 1, &fg, &bg) == OK && fg == COLOR_RED && bg == COLOR_BLUE);
    CHECK(app.config.colors[COLOR_TEXT].fg == COLOR_RED && app.config.colors[COLOR_TEXT].bg == COLOR_BLUE);
    CHECK(command(&app, "set color text bogus blue") == ERROR_WRONG_ARGS);
    CHECK(command(&app, "set color unknown red blue") == ERROR_WRONG_ARGS);
    CHECK(command(&app, "set color text red blue invalid") == ERROR_WRONG_ARGS);
    CHECK(command(&app, "set color text red cyan") == NO_ERROR);
    CHECK(pair_content(COLOR_TEXT + 1, &fg, &bg) == OK && bg == COLOR_CYAN);
    CHECK(command(&app, "set color text red magenta") == NO_ERROR);
    CHECK(pair_content(COLOR_TEXT + 1, &fg, &bg) == OK && bg == COLOR_MAGENTA);
    CHECK(command(&app, "set color text red bogus") == ERROR_WRONG_ARGS);
    remove_trace(path);
    cleanup_app(&app);
}

static char cell(WindowData *win, int y, int x) {
    return (char)(mvwinch(win->win, y, x) & A_CHARTEXT);
}

static void rendering_and_resize(void) {
    ApplicationData app = {.config = {.bar_offset = 32, .stage_width = 3, .show_empty_rows = false},
                           .error = NO_ERROR};
    new_window(&app);
    WindowData *win = app.windows[0];
    populate(win);
    win->filename = strdup("primary.log");
    render(&app);
    CHECK(win->first_cycle == 10);
    CHECK(cell(win, 2, 33) == 'F');
    CHECK(cell(win, 3, 33) == 's');
    CHECK(cell(win, 5, 41) == 'X');
    /* Attached traces sharing a numeric lane retain their own labelled row. */
    Instruction *inst = get_instruction(win->tables_array, 0);
    inst->stages[1].source = 1;
    win->lane_sources = malloc(sizeof(char *));
    win->lane_sources[0] = strdup("fpu.log");
    win->lane_sources_amount = 1;
    render(&app);
    char label[32];
    CHECK(mvwinnstr(win->win, 3, 1, label, 20) != ERR);
    CHECK(strstr(label, "fpu.log:7") != NULL);
    inst->stages[1].source = 3;
    render(&app);
    CHECK(mvwinnstr(win->win, 3, 1, label, 20) != ERR);
    CHECK(strstr(label, "src3:7") != NULL);
    inst->stages[1].source = 0;
    CHECK(command(&app, "ctime 12") == NO_ERROR);
    render(&app);
    app.mode = COMMAND;
    free(app.command);
    app.command = strdup("panelcmd ");
    render_status_bar(&app);
    CHECK(resizeterm(12, 60) == OK);
    render(&app);
    CHECK(win->height == 11 && win->width == 60);
    CHECK(cell(win, 2, 33) == 'F');
    /* A stage starting before the horizontal viewport still intersects it. */
    uint64_t origin = 11;
    werase(win->win);
    print_instruction(&app, win, &app.config, NULL, 1, &origin, 0);
    CHECK(origin == 11);
    print_instruction(&app, win, &app.config, inst, 1, &origin, 0);
    CHECK(origin == 10);
    app.config.bar_offset = UINT64_MAX;
    app.config.stage_width = UINT64_MAX;
    render(&app);
    CHECK(resizeterm(2, 10) == OK);
    render(&app);
    for (unsigned i = 0; i < 5; ++i) new_window(&app);
    render(&app);
    for (uint64_t i = 0; i < app.windows_qtty; ++i)
        CHECK(app.windows[i]->height >= 1 && app.windows[i]->win != NULL);
    CHECK(resizeterm(30, 100) == OK);
    cleanup_app(&app);
}

static void rendering_and_navigation_edges(void) {
    ApplicationData app = {.config = {.bar_offset = 32, .stage_width = 3,
                                       .show_empty_rows = false}, .error = NO_ERROR};
    new_window(&app);
    WindowData *win = app.windows[0];
    move_visible_instruction(win, true, 1);
    CHECK(win->first_instruction == 0);
    populate(win);
    win->first_instruction = UINT64_MAX;
    move_visible_instruction(win, false, 0);
    CHECK(win->first_instruction == 1025);
    win->first_instruction = 1;
    win->first_subrow = 1;
    render_window(&app, win);
    CHECK(win->first_instruction == 256 && win->first_subrow == 0);
    app.error = ERROR_NO_RESULT;
    render_status_bar(&app);
    CHECK(app.error == NO_ERROR);
    char text[40];
    CHECK(mvinnstr(getmaxy(stdscr)-1, 10, text, 30) != ERR);
    CHECK(strstr(text, "No result") != NULL);

    Instruction *inst = get_instruction(win->tables_array, 0);
    free(inst->instruction);
    inst->instruction = NULL;
    win->first_instruction = 0;
    render_window(&app, win);
    CHECK(mvwinnstr(win->win, 1, 1, text, 30) != ERR);
    CHECK(strstr(text, "0x0000000000000010") != NULL);
    free(inst->mem_addr);
    inst->mem_addr = NULL;
    inst->instruction = strdup("label without PC");
    render_window(&app, win);
    CHECK(mvwinnstr(win->win, 1, 1, text, 30) != ERR);
    CHECK(strstr(text, "label without PC") != NULL);
    win->first_subrow = 2;
    render_window(&app, win);
    CHECK(cell(win, 1, 33) == 's');
    win->first_subrow = 0;
    // Stages outside the panel's horizontal range must not paint any cells.
    inst->stages[1].cycle = 1000;
    render_window(&app, win);
    CHECK(cell(win, 3, 33) == ' ');
    cleanup_app(&app);
}

static void title(WindowData *win, char *text, int length) {
    CHECK(mvwinnstr(win->win, 0, 1, text, length) != ERR);
}

static void long_file_names(void) {
    ApplicationData app = {.config = {.bar_offset = 32, .stage_width = 3,
                                       .show_empty_rows = false}, .error = NO_ERROR};
    new_window(&app);
    WindowData *win = app.windows[0];
    populate(win);
    char text[128];

    /* Short names are printed as they are. */
    win->filename = strdup("short.log");
    render_window(&app, win);
    title(win, text, 9);
    CHECK(strcmp(text, "short.log") == 0);

    /* Long names are shortened in the middle to end before the vertical bar,
     * keeping the cycle label next to it intact. */
    free(win->filename);
    win->filename = strdup("sargantana_vector_benchmark_matmul_run_2026_11_05_konata_core0.kanata");
    render_window(&app, win);
    title(win, text, 31);
    CHECK(strcmp(text, "sargantana_vec...a_core0.kanata") == 0);
    CHECK(cell(win, 0, 33) == 'v');

    /* Too narrow for an ellipsis: plain cut. */
    app.config.bar_offset = 3;
    render_window(&app, win);
    title(win, text, 2);
    CHECK(strcmp(text, "sa") == 0);
    CHECK(cell(win, 0, 4) == 'v');

    /* No room at all: nothing is printed over the border. */
    app.config.bar_offset = 0;
    render_window(&app, win);
    CHECK(cell(win, 0, 1) != 's');

    /* A bar past the panel edge limits the name to the panel width. */
    app.config.bar_offset = 500;
    free(win->filename);
    win->filename = malloc(201);
    CHECK(win->filename != NULL);
    memset(win->filename, 'x', 200);
    win->filename[0] = 'a';
    win->filename[199] = 'z';
    win->filename[200] = '\0';
    render_window(&app, win);
    title(win, text, 60);
    CHECK(text[0] == 'a' && strstr(text, "...") != NULL);

    /* In a UTF-8 locale, bytes that are not valid characters count one
     * column each and the name still ends before the bar. */
    CHECK(setlocale(LC_CTYPE, "C.UTF-8") != NULL);
    app.config.bar_offset = 12;
    free(win->filename);
    win->filename = strdup("ab\xff\xfe" "cdefghijklmnop");
    render_window(&app, win);
    title(win, text, 2);
    CHECK(strcmp(text, "ab") == 0);
    CHECK(cell(win, 0, 13) == 'v');
    CHECK(setlocale(LC_CTYPE, "C") != NULL);
    cleanup_app(&app);
}

int main(void) {
    begin_screen();
    RUN(panel_lifecycle);
    RUN(navigation_and_input);
    RUN(searches_and_trace_commands);
    RUN(rendering_and_resize);
    RUN(rendering_and_navigation_edges);
    RUN(long_file_names);
    endwin();
    delscreen(screen);
    CHECK(fclose(input) == 0 && fclose(output) == 0);
    return EXIT_SUCCESS;
}
