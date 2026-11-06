#include <pwd.h>
#include <sys/stat.h>
#include "test.h"
#include "files.h"
#include "autocomplete.h"
#include "window.h"

/* A named user's home is a temporary directory, independent of the machine's
 * passwd entries. Also check the real current-user lookup below. */
static char *test_home;
struct passwd *__real_getpwnam(const char *);
struct passwd *__wrap_getpwnam(const char *name) {
    static struct passwd entry;
    if (strcmp(name, "izumi-test-user") == 0) {
        entry.pw_dir = test_home;
        return &entry;
    }
    if (strcmp(name, "izumi-no-such-user") == 0) return NULL;
    return __real_getpwnam(name);
}

static void expect_expansion(const char *input, const char *expected) {
    char *expanded = expand_path(input);
    if (expected == NULL) CHECK(expanded == NULL);
    else CHECK(expanded != NULL && strcmp(expanded, expected) == 0);
    free(expanded);
}

static void path_expansion(void) {
    expect_expansion(NULL, NULL);
    expect_expansion("relative.log", "relative.log");
    expect_expansion("/tmp/file", "/tmp/file");
    expect_expansion("~", test_home);
    expect_expansion("~izumi-test-user", test_home);
    char expected[1024];
    snprintf(expected, sizeof(expected), "%s/trace.log", test_home);
    expect_expansion("~/trace.log", expected);
    expect_expansion("~izumi-test-user/trace.log", expected);
    expect_expansion("~izumi-no-such-user/file", NULL);
    char long_user[300];
    memset(long_user, 'a', sizeof(long_user) - 1);
    long_user[0] = '~';
    long_user[sizeof(long_user) - 1] = '\0';
    expect_expansion(long_user, NULL);
    CHECK(unsetenv("HOME") == 0);
    expect_expansion("~/trace.log", NULL);
    CHECK(setenv("HOME", "", 1) == 0);
    expect_expansion("~/trace.log", NULL);
    CHECK(setenv("HOME", test_home, 1) == 0);
    struct passwd *current = getpwuid(getuid());
    CHECK(current != NULL);
    char *name = strdup(current->pw_name), *home = strdup(current->pw_dir);
    char input[512];
    snprintf(input, sizeof(input), "~%s", name);
    expect_expansion(input, home);
    free(name);
    free(home);
}

static void expect_suggestion(ApplicationData *app, const char *text, const char *expected) {
    free(app->command);
    app->command = strdup(text);
    char *suggestion = compute_autocomplete_suggestion(app);
    if (expected == NULL) CHECK(suggestion == NULL);
    else CHECK(suggestion != NULL && strcmp(suggestion, expected) == 0);
    free(suggestion);
}

static void path_completion_and_commands(void) {
    FILE *input = tmpfile(), *output = tmpfile();
    CHECK(input && output);
    SCREEN *screen = newterm("xterm", output, input);
    CHECK(screen != NULL);
    set_term(screen);
    CHECK(resizeterm(24, 80) == OK);
    ApplicationData app = {.mode = COMMAND};
    expect_suggestion(&app, "open ~/tra", "ce.log");
    expect_suggestion(&app, "open ~izumi-test-user/tra", "ce.log");
    expect_suggestion(&app, "attach ~izumi-test-user/tra", "ce.log");
    expect_suggestion(&app, "open ~izumi-no-such-user/tra", NULL);
    expect_suggestion(&app, "set invalid", NULL);
    expect_suggestion(&app, "set unknown extra", NULL);
    expect_suggestion(&app, "set color text red blue bold extra", NULL);
    expect_suggestion(&app, "panelcmd invalid", NULL);
    expect_suggestion(&app, "panelcmd invalid ", NULL);
    expect_suggestion(&app, "panelcmd j extra", NULL);
    /* No slash: exercise relative-directory completion in an isolated cwd. */
    char *cwd = getcwd(NULL, 0);
    CHECK(cwd != NULL && chdir(test_home) == 0);
    expect_suggestion(&app, "open tra", "ce.log");
    CHECK(chdir(cwd) == 0);
    free(cwd);

    CHECK(command(&app, "open ~/trace.log") == NO_ERROR);
    CHECK(app.windows_qtty == 1 && app.windows[0]->filename != NULL);
    CHECK(strcmp(get_instruction(app.windows[0]->tables_array, 0)->instruction, "nop") == 0);
    CHECK(command(&app, "attach ~izumi-test-user/trace.log") == NO_ERROR);
    CHECK(app.windows[0]->lane_sources_amount == 1);
    CHECK(command(&app, "open ~izumi-test-user/trace.log") == NO_ERROR);
    CHECK(app.windows[0]->lane_sources_amount == 0);
    CHECK(command(&app, "open ~") == ERROR_NOT_A_FILE);
    CHECK(command(&app, "open ~izumi-test-user") == ERROR_NOT_A_FILE);
    CHECK(command(&app, "open ~izumi-no-such-user/trace.log") == ERROR_FILE_NOT_EXISTS);
    CHECK(command(&app, "attach ~izumi-no-such-user/trace.log") == ERROR_FILE_NOT_EXISTS);
    CHECK(command(&app, "open ~/missing.log") == ERROR_FILE_NOT_EXISTS);
    CHECK(unsetenv("HOME") == 0);
    CHECK(command(&app, "open ~/trace.log") == ERROR_FILE_NOT_EXISTS);
    CHECK(command(&app, "attach ~/trace.log") == ERROR_FILE_NOT_EXISTS);
    expect_suggestion(&app, "open ~/tra", NULL);
    close_all_panels(&app);
    free(app.command);
    endwin();
    delscreen(screen);
    CHECK(fclose(input) == 0 && fclose(output) == 0);
}

int main(void) {
    char *old_home = getenv("HOME") ? strdup(getenv("HOME")) : NULL;
    char directory[] = "izumi-home-XXXXXX";
    CHECK(mkdtemp(directory) != NULL);
    test_home = realpath(directory, NULL);
    CHECK(test_home != NULL && setenv("HOME", test_home, 1) == 0);
    char path[1024];
    snprintf(path, sizeof(path), "%s/trace.log", test_home);
    FILE *file = fopen(path, "w");
    CHECK(file != NULL);
    CHECK(fputs("Kanata\t0004\nI\t0\t0\t0\nL\t0\t0\t0x10: nop\nS\t0\t0\tF\nC\t1\nR\t0\t0\t0\n", file) >= 0);
    CHECK(fclose(file) == 0);
    RUN(path_expansion);
    RUN(path_completion_and_commands);
    CHECK(unlink(path) == 0 && rmdir(directory) == 0);
    free(test_home);
    if (old_home) { CHECK(setenv("HOME", old_home, 1) == 0); free(old_home); }
    else CHECK(unsetenv("HOME") == 0);
    return EXIT_SUCCESS;
}
