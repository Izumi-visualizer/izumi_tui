/* Regression for empty-row commands, rendering and navigation. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "window.h"
#include "command_tree.h"
#include "interact.h"
#include "autocomplete.h"
#include <izumi/parser.h>

static ErrorKind command(ApplicationData *app, const char *text) {
    free(app->command);
    app->command = strdup(text);
    return run_command(app);
}

static void add_instruction(InstructionTableArray *tables, unsigned id,
                            const char *label, bool stages) {
    unsigned table = id / 256;
    if (tables->tables[table] == NULL) {
        tables->tables[table] = malloc(sizeof(InstructionTable));
        init_InstructionTable(tables->tables[table]);
    }
    Instruction *inst = &tables->tables[table]->content[id % 256];
    init_Instruction(inst);
    inst->instruction = strdup(label);
    if (stages) {
        inst->qtty_stages = 1;
        inst->stages[0].name = strdup("X");
        inst->stages[0].cycle = 100;
        inst->stages[0].duration = 1;
    }
}

static void expect_text(WINDOW *window, int y, const char *text) {
    char line[256];
    mvwinnstr(window, y, 0, line, 179);
    assert(strstr(line, text) != NULL);
}

int main(void) {
    // Core row IDs can jump over whole tables, then arrive out of order.
    // Empty placeholder rows are legal Kanata instructions with no stages.
    FILE *fixture = tmpfile();
    assert(fixture != NULL);
    fputs("Kanata\t0004\nI\t0\t0\t0\nS\t0\t0\tFP\nC\t1\nR\t0\t0\t0\n"
          "I\t4096\t4096\t0\nR\t4096\t0\t1\n"
          "I\t258\t258\t0\nL\t258\t0\tcore_id=258 FMADD.FP32\n"
          "S\t258\t0\tX_IN1\nC\t1\nE\t258\t0\tX_IN1\nR\t258\t1\t0\n", fixture);
    fflush(fixture);
    char path[64];
    snprintf(path, sizeof(path), "/proc/self/fd/%d", fileno(fixture));
    InstructionTableArray sparse = parse_file(path);
    assert(sparse.qtty_tables == 17);
    assert(sparse.tables[16]->content[0].valid);
    assert(sparse.tables[16]->content[0].finished);
    assert(sparse.tables[16]->content[0].qtty_stages == 0);
    assert(sparse.tables[1]->content[2].stages[0].duration == 1);
    assert(sparse.tables[2] == NULL);
    assert(find(&sparse, "core_id=258", INST, DOWN, 1).position == 258);
    assert(!find(&sparse, "missing", INST, DOWN, 259).valid);
    free_InstructionTableArray(&sparse);
    fclose(fixture);

    FILE *output = tmpfile(), *input = tmpfile();
    assert(output && input);
    SCREEN *screen = newterm("xterm", output, input);
    assert(screen != NULL);
    set_term(screen);
    resizeterm(40, 180);

    ApplicationData app = {0};
    app.config.show_empty_rows = true;
    app.config.bar_offset = 32;
    app.config.stage_width = 3;
    app.mode = NORMAL;
    new_window(&app);
    WindowData *win = app.windows[0];
    init_InstructionTableArray(win->tables_array);
    win->tables_array->qtty_tables = 4;
    add_instruction(win->tables_array, 2, "ROW_TWO", true);
    add_instruction(win->tables_array, 4, "NO_STAGES", false);
    add_instruction(win->tables_array, 7, "ROW_SEVEN", true);
    add_instruction(win->tables_array, 258, "ROW_258", true);
    // Entire table 2 is absent.
    add_instruction(win->tables_array, 770, "ROW_770", true);

    render_window(&app, win);
    expect_text(win->win, 6, "ROW_TWO");
    expect_text(win->win, 10, "NO_STAGES");
    expect_text(win->win, 16, "ROW_SEVEN");

    assert(command(&app, "toggleemptyrows") == NO_ERROR);
    assert(!app.config.show_empty_rows);
    render_window(&app, win);
    assert(win->first_instruction == 2);
    expect_text(win->win, 2, "ROW_TWO");
    expect_text(win->win, 4, "ROW_SEVEN");
    expect_text(win->win, 6, "ROW_258");
    expect_text(win->win, 8, "ROW_770");

    parse_input(&app, 'j'); assert(win->first_instruction == 7);
    app.number = 2;
    parse_input(&app, 'j'); assert(win->first_instruction == 770);
    parse_input(&app, 'k'); assert(win->first_instruction == 258);
    app.number = 999;
    parse_input(&app, 'k'); assert(win->first_instruction == 2);

    assert(command(&app, "toggleemptyrows") == NO_ERROR);
    assert(app.config.show_empty_rows);
    win->first_instruction = 0;
    render_window(&app, win);
    expect_text(win->win, 6, "ROW_TWO");
    expect_text(win->win, 10, "NO_STAGES");

    assert(command(&app, "set show_empty_rows off") == NO_ERROR);
    assert(!app.config.show_empty_rows);
    assert(command(&app, "set show_empty_rows on") == NO_ERROR);
    assert(app.config.show_empty_rows);
    assert(command(&app, "set show_empty_rows invalid") == ERROR_WRONG_ARGS);
    assert(app.config.show_empty_rows);
    assert(command(&app, "set show_empty_rows") == ERROR_WRONG_AMOUNT_ARGS);
    assert(command(&app, "toggleemptyrows unexpected") == ERROR_WRONG_AMOUNT_ARGS);

    app.mode = COMMAND;
    free(app.command);
    app.command = strdup("toggleempty");
    char *suggestion = compute_autocomplete_suggestion(&app);
    assert(suggestion && strcmp(suggestion, "rows") == 0);
    free(suggestion);
    free(app.command);
    app.command = strdup("set show_empty_rows o");
    suggestion = compute_autocomplete_suggestion(&app);
    assert(suggestion && strcmp(suggestion, "n") == 0);
    free(suggestion);
    app.mode = NORMAL;

    // An all-empty page keeps a finite horizontal origin in either mode.
    free_InstructionTableArray(win->tables_array);
    init_InstructionTableArray(win->tables_array);
    win->first_instruction = 0;
    win->first_cycle = 42;
    render_window(&app, win);
    assert(win->first_cycle == 42);
    assert(command(&app, "toggleemptyrows") == NO_ERROR);
    render_window(&app, win);
    assert(win->first_cycle == 42);
    parse_input(&app, 'j'); assert(win->first_instruction == 0);
    parse_input(&app, 'k'); assert(win->first_instruction == 0);

    close_all_panels(&app);
    free(app.command);
    endwin();
    delscreen(screen);
    fclose(input);
    fclose(output);
    puts("Empty-row command, autocomplete, rendering and navigation checks passed");
    return 0;
}
