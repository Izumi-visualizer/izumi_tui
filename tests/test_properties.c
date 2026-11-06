#include <inttypes.h>
#include <ctype.h>
#include "test.h"
#include "utils.h"
#include "autocomplete.h"

extern int command_arg_count(const char *);
extern void split_command_arguments(char *, int, char **);

/* Fixed seed and unsigned arithmetic make failures repeatable. */
static uint64_t state = UINT64_C(0x495a554d49);
static uint64_t random_value(void) {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
}

static void numeric_round_trips(void) {
    for (unsigned i = 0; i < 10000; ++i) {
        uint64_t expected = random_value(), actual = 0;
        char text[64];
        snprintf(text, sizeof(text), "%" PRIu64, expected);
        CHECK(parse_u64(text, &actual) && actual == expected);
        size_t length = strlen(text);
        text[length] = 'x';
        text[length + 1] = '\0';
        CHECK(!parse_u64(text, &actual) && actual == expected);
    }
}

static void randomized_tokenization_and_completion(void) {
    ApplicationData app = {.mode = COMMAND};
    const char alphabet[] = " \t\n\rabcdefghijklmnopqrstuvwxyz0123456789_-";
    for (unsigned iteration = 0; iteration < 10000; ++iteration) {
        char text[128], copy[128];
        unsigned length = random_value() % sizeof(text);
        int words = 0;
        bool inside = false;
        for (unsigned i = 0; i < length; ++i) {
            text[i] = alphabet[random_value() % (sizeof(alphabet) - 1)];
            bool nonspace = !isspace((unsigned char)text[i]);
            if (nonspace && !inside) ++words;
            inside = nonspace;
        }
        text[length] = '\0';
        strcpy(copy, text);
        CHECK(command_arg_count(text) == words - 1);
        if (words) {
            char *args[128];
            split_command_arguments(copy, words - 1, args);
            const char *cursor = text;
            for (int i = 0; i < words; ++i) {
                cursor += strspn(cursor, " \t\n\r");
                size_t word = strcspn(cursor, " \t\n\r");
                CHECK(strlen(args[i]) == word && strncmp(args[i], cursor, word) == 0);
                cursor += word;
            }
        }
        free(app.command);
        app.command = strdup(text);
        char *suggestion = compute_autocomplete_suggestion(&app);
        free(suggestion);
    }
    free(app.command);
}

static int compare_u64(const void *a, const void *b) {
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}

static void randomized_timelines(void) {
    WindowData win = {0};
    WindowData *windows[] = {&win};
    ApplicationData app = {.windows = windows, .windows_qtty = 1};
    uint64_t expected[128];
    size_t count = 0;
    for (unsigned i = 0; i < 3000; ++i) {
        char text[100];
        uint64_t value = random_value();
        unsigned action = random_value() % 3;
        if (count == 0 || (action == 0 && count < 128)) {
            snprintf(text, sizeof(text), "ctime %" PRIu64, value);
            expected[count++] = value;
        } else if (action == 1) {
            size_t index = random_value() % count;
            snprintf(text, sizeof(text), "rmtime %zu", index);
            memmove(expected + index, expected + index + 1, (count - index - 1) * sizeof(*expected));
            --count;
        } else {
            size_t index = random_value() % count;
            snprintf(text, sizeof(text), "mvtime %zu %" PRIu64, index, value);
            expected[index] = value;
        }
        CHECK(command(&app, text) == NO_ERROR);
        qsort(expected, count, sizeof(*expected), compare_u64);
        CHECK(win.timelines_amount == count);
        CHECK(count == 0 || memcmp(win.timelines, expected, count * sizeof(*expected)) == 0);
    }
    free(win.timelines);
    free(app.command);
}

int main(void) {
    RUN(numeric_round_trips);
    RUN(randomized_tokenization_and_completion);
    RUN(randomized_timelines);
    return EXIT_SUCCESS;
}
