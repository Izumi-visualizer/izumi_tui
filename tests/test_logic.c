#include <limits.h>
#include <sys/stat.h>
#include "test.h"
#include "utils.h"
#include "configure.h"
#include "files.h"
#include "autocomplete.h"

extern int command_arg_count(const char *);
extern void split_command_arguments(char *, int, char **);
extern ErrorKind traverse_command_tree(ApplicationData *, const Command *, size_t,
                                       const char *, int, const char **);

static void numeric_arguments(void) {
    const char *valid[] = {"0", "1", "00042", "18446744073709551615"};
    const uint64_t expected[] = {0, 1, 42, UINT64_MAX};
    for (unsigned i = 0; i < 4; ++i) {
        uint64_t out = 99;
        CHECK(parse_u64(valid[i], &out) && out == expected[i]);
    }
    const char *invalid[] = {NULL, "", "-1", "+1", " 1", "1 ", "1x",
                             "18446744073709551616", "-18446744073709551615"};
    for (unsigned i = 0; i < sizeof(invalid)/sizeof(invalid[0]); ++i) {
        uint64_t out = 99;
        CHECK(!parse_u64(invalid[i], &out) && out == 99);
    }
    CHECK(!parse_u64("1", NULL));
}

static void tokenization(void) {
    const char *inputs[] = {"", " \t\r\n", "quit", " quit \t", "set\tstage_width 3",
                            "  panelcmd   j  ", "open file\r\n"};
    const int counts[] = {-1, -1, 0, 0, 2, 1, 1};
    for (unsigned i = 0; i < 7; ++i) {
        char *copy = strdup(inputs[i]);
        CHECK(command_arg_count(copy) == counts[i]);
        if (counts[i] >= 0) {
            char *args[3];
            split_command_arguments(copy, counts[i], args);
            for (int j = 0; j <= counts[i]; ++j) {
                CHECK(args[j][0] != '\0');
                CHECK(strcspn(args[j], " \t\r\n") == strlen(args[j]));
            }
        }
        free(copy);
    }
}

static ErrorKind callback_none(ApplicationData *app) {
    app->number = 123;
    return NO_ERROR;
}
static ErrorKind callback_fixed(ApplicationData *app, const char **args) {
    CHECK(strcmp(args[0], "value") == 0);
    app->number = 456;
    return NO_ERROR;
}
static ErrorKind callback_list(ApplicationData *app, int argc, const char **args) {
    CHECK(argc == 1 && strcmp(args[0], "value") == 0);
    app->number = 789;
    return ERROR_WRONG_ARGS;
}

static void dispatch_tree(void) {
    const Command children[] = {CMD_NO_ARGS("child", callback_none)};
    const Command commands[] = {CMD_ALIAS("alias", "none"),
        CMD_NO_ARGS("none", callback_none), CMD_FIXED_ARGLIST("fixed", 1, callback_fixed),
        CMD_ARGLIST("list", callback_list), CMD_SUBCOMMAND("sub", children)};
    ApplicationData app = {0};
    const char *args[] = {"value"};
    CHECK(traverse_command_tree(&app, commands, 5, "none", 0, NULL) == NO_ERROR);
    CHECK(app.number == 123);
    CHECK(traverse_command_tree(&app, commands, 5, "none", 1, args) == ERROR_WRONG_AMOUNT_ARGS);
    CHECK(traverse_command_tree(&app, commands, 5, "fixed", 0, NULL) == ERROR_WRONG_AMOUNT_ARGS);
    CHECK(traverse_command_tree(&app, commands, 5, "fixed", 1, args) == NO_ERROR && app.number == 456);
    CHECK(traverse_command_tree(&app, commands, 5, "list", 1, args) == ERROR_WRONG_ARGS && app.number == 789);
    CHECK(traverse_command_tree(&app, commands, 5, "alias", 0, NULL) == NO_ERROR && app.number == 123);
    CHECK(traverse_command_tree(&app, commands, 5, "sub", 0, NULL) == ERROR_SUBCOMMAND_EMPTY);
    const char *child[] = {"child"};
    CHECK(traverse_command_tree(&app, commands, 5, "sub", 1, child) == NO_ERROR);
    CHECK(traverse_command_tree(&app, commands, 5, "missing", 0, NULL) == ERORR_WRONG_COMMAND);
    size_t count;
    CHECK(get_root_commands(&count) != NULL && count > 10);
    CHECK(get_panel_commands(&count) != NULL && count == 2);
    CHECK(get_root_commands(NULL) != NULL && get_panel_commands(NULL) != NULL);
}

static void command_errors_and_settings(void) {
    ApplicationData app = {0};
    CHECK(command(&app, "") == ERROR_EMPTY_COMMAND);
    CHECK(command(&app, "missing") == ERORR_WRONG_COMMAND);
    const char *fixed[] = {"open", "o", "attach", "findpc", "findinst", "rmtime", "mvtime"};
    for (unsigned i = 0; i < 7; ++i)
        CHECK(command(&app, fixed[i]) == ERROR_WRONG_AMOUNT_ARGS);
    CHECK(command(&app, "set") == ERROR_WRONG_AMOUNT_ARGS);
    CHECK(command(&app, "set nonexistent 1") == ERROR_WRONG_ARGS);
    CHECK(command(&app, "set bar_offset 15") == NO_ERROR && app.config.bar_offset == 15);
    CHECK(command(&app, "set stage_width 4") == NO_ERROR && app.config.stage_width == 4);
    CHECK(command(&app, "set stage_width -1") == ERROR_WRONG_ARGS && app.config.stage_width == 4);
    CHECK(command(&app, "set show_empty_rows on") == NO_ERROR && app.config.show_empty_rows);
    CHECK(command(&app, "toggleemptyrows") == NO_ERROR && !app.config.show_empty_rows);
    CHECK(command(&app, "set show_empty_rows invalid") == ERROR_WRONG_ARGS);
    CHECK(command(&app, "panelsync") == NO_ERROR && app.windows_synced);
    CHECK(command(&app, "paneldesync") == NO_ERROR && !app.windows_synced);
    CHECK(command(&app, "ca") == NO_ERROR && app.windows == NULL);
    CHECK(command(&app, "panelcmd j") == ERROR_NO_WINDOW && app.window_focused == 0);
    CHECK(command(&app, "panelcmd k") == ERROR_NO_WINDOW);
    CHECK(command(&app, "next") == ERROR_NO_WINDOW);
    CHECK(command(&app, "prev") == ERROR_NO_WINDOW);
    CHECK(command(&app, "ctime") == ERROR_NO_WINDOW);
    CHECK(command(&app, "rmtime 0") == ERROR_NO_WINDOW);
    CHECK(command(&app, "mvtime 0 1") == ERROR_NO_WINDOW);
    CHECK(command(&app, "attach missing") == ERROR_NO_WINDOW);
    CHECK(command(&app, "q") == NO_ERROR && app.quit_requested);
    free(app.command);
}

static void timeline_commands(void) {
    WindowData win = {.first_cycle = 100};
    WindowData *windows[] = {&win};
    ApplicationData app = {.windows = windows, .windows_qtty = 1};
    CHECK(command(&app, "ctime") == NO_ERROR);
    CHECK(command(&app, "ctime 30") == NO_ERROR);
    CHECK(command(&app, "ctime 70") == NO_ERROR);
    CHECK(win.timelines_amount == 3 && win.timelines[0] == 30 && win.timelines[2] == 100);
    CHECK(command(&app, "mvtime 0 200") == NO_ERROR && win.timelines[2] == 200);
    CHECK(command(&app, "rmtime 1") == NO_ERROR);
    CHECK(win.timelines_amount == 2 && win.timelines[0] == 70 && win.timelines[1] == 200);
    CHECK(command(&app, "rmtime 0") == NO_ERROR && win.timelines[0] == 200);
    CHECK(command(&app, "rmtime 0") == NO_ERROR && win.timelines_amount == 0);
    CHECK(command(&app, "rmtime 0") == ERROR_IDX_BIGGER);
    CHECK(command(&app, "ctime bad") == ERROR_WRONG_ARGS);
    CHECK(command(&app, "ctime 1 2") == ERROR_WRONG_AMOUNT_ARGS);
    CHECK(command(&app, "mvtime 0 1") == ERROR_IDX_BIGGER);
    free(win.timelines);
    free(app.command);
}

static void files_and_attach(void) {
    CHECK(!check_file(NULL).exists);
    CHECK(!check_file("/izumi-test-missing-path").exists);
    CHECK(check_file(".").exists && !check_file(".").is_file);
    char *name = file_basename("/a/b/trace.log");
    CHECK(strcmp(name, "trace.log") == 0);
    free(name);
    char *primary = trace_file("Kanata\t0004\nI\t0\t0\t0\nL\t0\t0\t0x10: add\nS\t0\t0\tF\nC\t2\nR\t0\t0\t0\n");
    char *extra = trace_file("Kanata\t0004\nI\t0\t0\t0\nL\t0\t0\tother\nS\t0\t7\tExecute\nI\t1025\t1\t0\nS\t1025\t3\tX\nC\t3\nR\t0\t0\t1\nR\t1025\t1\t0\n");
    InstructionTableArray array = {0};
    CHECK(read_file("/izumi-test-missing-path", &array) == NULL && array.tables == NULL);
    name = read_file(primary, &array);
    CHECK(name != NULL && strcmp(name, primary) == 0);
    free(name);
    attach_trace(extra, &array, 2);
    Instruction *inst = get_instruction(&array, 0);
    CHECK(strcmp(inst->instruction, "add") == 0 && inst->finished && !inst->flushed);
    CHECK(inst->qtty_stages == 2 && inst->stages[0].source == 0 && inst->stages[1].source == 2);
    CHECK(inst->stages[1].lane == 7 && strcmp(inst->stages[1].name, "Execute") == 0);
    CHECK(array.tables[2] == NULL && get_instruction(&array, 1025)->stages[0].source == 2);
    name = read_file(primary, &array);
    CHECK(array.qtty_tables == 1 && get_instruction(&array, 0)->qtty_stages == 1);
    free(name);
    free_InstructionTableArray(&array);
    remove_trace(primary);
    remove_trace(extra);
}

static void config_files(void) {
    char *old_home = getenv("HOME") ? strdup(getenv("HOME")) : NULL;
    CHECK(unsetenv("HOME") == 0);
    CHECK(get_config_path() == NULL);
    CHECK(setenv("HOME", "/test-home", 1) == 0);
    char *path = get_config_path();
    CHECK(strcmp(path, "/test-home/.config/izumi/config") == 0);
    free(path);
    char home[8192];
    memset(home, 'a', sizeof(home) - 1);
    home[sizeof(home) - 1] = '\0';
    CHECK(setenv("HOME", home, 1) == 0);
    CHECK(get_config_path() == NULL);
    if (old_home) { CHECK(setenv("HOME", old_home, 1) == 0); free(old_home); }
    else CHECK(unsetenv("HOME") == 0);
    CHECK(read_config_file(NULL) == NULL);
    CHECK(read_config_file(strdup("/izumi-test-missing-path")) == NULL);
    path = trace_file("#comment\r\n\n  # indented\n \t\nset stage_width 7\r\nset show_empty_rows off\nquit");
    char **commands = read_config_file(strdup(path));
    CHECK(commands != NULL && strcmp(commands[0], "set stage_width 7") == 0);
    CHECK(commands[3] == NULL);
    ApplicationData app = {0};
    CHECK(execute_config_commands(&app, commands));
    CHECK(app.config.stage_width == 7 && !app.config.show_empty_rows && app.quit_requested);
    CHECK(!execute_config_commands(&app, NULL));
    free(app.command);
    app.command = NULL;
    remove_trace(path);
    path = trace_file("missing-command\n");
    CHECK(!execute_config_commands(&app, read_config_file(strdup(path))));
    free(app.command);
    remove_trace(path);
    path = trace_file("");
    commands = read_config_file(strdup(path));
    CHECK(commands != NULL && commands[0] == NULL);
    free(commands);
    remove_trace(path);
}

static void autocomplete(void) {
    ApplicationData app = {.mode = COMMAND};
    const char *inputs[] = {"pan", "panelcmd ", "panelcmd k", "set sh", "set color st",
                            "set color text r", "set color text red b", "set color text red blue bo"};
    const char *expected[] = {"elcmd", "j", "", "ow_empty_rows", "atus", "ed", "lack", "ld"};
    for (unsigned i = 0; i < 8; ++i) {
        free(app.command);
        app.command = strdup(inputs[i]);
        char *suggestion = compute_autocomplete_suggestion(&app);
        CHECK(suggestion != NULL && strcmp(suggestion, expected[i]) == 0);
        free(suggestion);
    }
    free(app.command);
    app.command = strdup("set show_empty_rows of");
    CHECK(accept_autocomplete_suggestion(&app));
    CHECK(strcmp(app.command, "set show_empty_rows off") == 0);
    app.mode = NORMAL;
    CHECK(compute_autocomplete_suggestion(&app) == NULL);
    app.mode = COMMAND;
    free(app.command);
    app.command = strdup("no-such-command");
    CHECK(!accept_autocomplete_suggestion(&app));
    free(app.command);
}

static void filesystem_completion_and_config_growth(void) {
    char directory[] = "izumi-completion-XXXXXX";
    CHECK(mkdtemp(directory) != NULL);
    char file[256], child[256], text[512];
    snprintf(file, sizeof(file), "%s/unique.trace", directory);
    snprintf(child, sizeof(child), "%s/folder", directory);
    FILE *stream = fopen(file, "w");
    CHECK(stream != NULL && fclose(stream) == 0);
    CHECK(mkdir(child, 0700) == 0);
    ApplicationData app = {.mode = COMMAND};
    const char *names[] = {"open", "o", "attach", "at"};
    for (unsigned i = 0; i < 4; ++i) {
        snprintf(text, sizeof(text), "%s %s/uni", names[i], directory);
        app.command = strdup(text);
        char *suggestion = compute_autocomplete_suggestion(&app);
        CHECK(suggestion != NULL && strcmp(suggestion, "que.trace") == 0);
        free(suggestion);
        free(app.command);
    }
    snprintf(text, sizeof(text), "open %s/fol", directory);
    app.command = strdup(text);
    CHECK(accept_autocomplete_suggestion(&app));
    CHECK(strstr(app.command, "/folder/") != NULL);
    free(app.command);
    app.command = NULL;
    char *old_home = getenv("HOME") ? strdup(getenv("HOME")) : NULL;
    char *absolute = realpath(directory, NULL);
    CHECK(absolute != NULL && setenv("HOME", absolute, 1) == 0);
    app.command = strdup("open ~/uni");
    CHECK(accept_autocomplete_suggestion(&app));
    CHECK(strcmp(app.command, "open ~/unique.trace") == 0);
    free(app.command);
    app.command = NULL;
    free(absolute);
    if (old_home) { CHECK(setenv("HOME", old_home, 1) == 0); free(old_home); }
    else CHECK(unsetenv("HOME") == 0);
    CHECK(unlink(file) == 0 && rmdir(child) == 0 && rmdir(directory) == 0);
    char *path = trace_file("");
    stream = fopen(path, "w");
    CHECK(stream != NULL);
    for (unsigned i = 0; i < 45; ++i)
        CHECK(fprintf(stream, "set stage_width %u\n", i) > 0);
    CHECK(fclose(stream) == 0);
    char **commands = read_config_file(strdup(path));
    CHECK(commands != NULL && commands[45] == NULL);
    CHECK(execute_config_commands(&app, commands) && app.config.stage_width == 44);
    free(app.command);
    remove_trace(path);
}

int main(void) {
    RUN(numeric_arguments);
    RUN(tokenization);
    RUN(dispatch_tree);
    RUN(command_errors_and_settings);
    RUN(timeline_commands);
    RUN(files_and_attach);
    RUN(config_files);
    RUN(autocomplete);
    RUN(filesystem_completion_and_config_growth);
    return EXIT_SUCCESS;
}
