#ifndef IZUMI_TUI_TEST_H
#define IZUMI_TUI_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <izumi/parser.h>
#include "command_tree.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)
#define RUN(test) do { test(); puts("PASS: " #test); } while (0)

static inline ErrorKind command(ApplicationData *app, const char *text) {
    free(app->command);
    app->command = strdup(text);
    CHECK(app->command != NULL);
    return run_command(app);
}

static inline char *trace_file(const char *text) {
    char path[] = "izumi-trace-XXXXXX";
    int fd = mkstemp(path);
    CHECK(fd >= 0);
    FILE *file = fdopen(fd, "w");
    CHECK(file != NULL && fputs(text, file) >= 0);
    CHECK(fclose(file) == 0);
    return strdup(path);
}

static inline void remove_trace(char *path) {
    CHECK(unlink(path) == 0);
    free(path);
}

#endif
