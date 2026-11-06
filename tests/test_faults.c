#include <errno.h>
#include "test.h"
#include "configure.h"
#include "files.h"

/* GNU ld wraps only application-object references. libc/ncurses/sanitizer
 * allocations remain untouched; failures are one-shot and opt-in. */
static int malloc_after = -1, realloc_after = -1, strdup_after = -1;
void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
char *__real_strdup(const char *);

static bool fail_now(int *remaining) {
    if (*remaining < 0) return false;
    if ((*remaining)-- > 0) return false;
    errno = ENOMEM;
    return true;
}
void *__wrap_malloc(size_t size) {
    return fail_now(&malloc_after) ? NULL : __real_malloc(size);
}
void *__wrap_realloc(void *pointer, size_t size) {
    return fail_now(&realloc_after) ? NULL : __real_realloc(pointer, size);
}
char *__wrap_strdup(const char *text) {
    return fail_now(&strdup_after) ? NULL : __real_strdup(text);
}

static void config_allocation_failures(void) {
    char *path = trace_file("set stage_width 3\nset bar_offset 10\n");
    char *copy = strdup(path);
    malloc_after = 0;
    CHECK(read_config_file(copy) == NULL);
    copy = strdup(path);
    strdup_after = 1; // Clean up the first stored command when the second fails.
    CHECK(read_config_file(copy) == NULL);
    FILE *file = fopen(path, "w");
    CHECK(file != NULL);
    for (unsigned i = 0; i < 15; ++i) CHECK(fputs("quit\n", file) >= 0);
    CHECK(fclose(file) == 0);
    copy = strdup(path);
    realloc_after = 0;
    CHECK(read_config_file(copy) == NULL);
    char **commands = read_config_file(strdup(path));
    CHECK(commands != NULL);
    ApplicationData app = {.command = strdup("old command")};
    malloc_after = 0;
    CHECK(!execute_config_commands(&app, commands) && app.command == NULL);
    /* Reading a directory opens successfully but getline fails with EISDIR. */
    CHECK(read_config_file(strdup(".")) == NULL);
    remove_trace(path);
}

static void path_allocation_failures(void) {
    char *saved_home = getenv("HOME") ? strdup(getenv("HOME")) : NULL;
    CHECK(setenv("HOME", "/temporary-home", 1) == 0);
    malloc_after = 0;
    CHECK(get_config_path() == NULL);
    malloc_after = 0;
    CHECK(expand_path("~/trace") == NULL);
    if (saved_home) { CHECK(setenv("HOME", saved_home, 1) == 0); free(saved_home); }
    else CHECK(unsetenv("HOME") == 0);
}

int main(void) {
    RUN(config_allocation_failures);
    RUN(path_allocation_failures);
    return EXIT_SUCCESS;
}
