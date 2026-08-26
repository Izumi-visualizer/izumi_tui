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

#include "system_curses.h"
#define _GNU_SOURCE

#include <linux/limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <izumi/finder.h>

#include "files.h"
#include "window.h"
#include "utils.h"
#include "autocomplete.h"
#include <dirent.h>
#include <pwd.h>
#include <ctype.h>
#include <sys/stat.h>
#include "command_tree.h"

// COMMANDS and PANEL_COMMANDS are defined in include/commands.h; declare
// them here as extern to avoid duplicate-definition issues.
extern const Command COMMANDS[];
extern const Command PANEL_COMMANDS[];

ErrorKind newpanel_cb(ApplicationData *app_data) {
    new_window(app_data);
    return NO_ERROR;
}

ErrorKind closepanel_cb(ApplicationData *app_data, const int argc, const char * argv[]) {
    uint64_t panel_id;

    if (argc == 1) {
        if (!parse_u64(argv[0], &panel_id)) return ERROR_WRONG_ARGS;
    }
    else {
        panel_id = app_data->window_focused;
    }

    close_panel(app_data, panel_id);

    return NO_ERROR;
}

ErrorKind closeallpanels_cb(ApplicationData *app_data) {
    close_all_panels(app_data);

    return NO_ERROR;
}

ErrorKind open_cb(ApplicationData *app_data, const char * argv[]) {
    const char *file_name = argv[0];

    if (app_data->windows == NULL) {
        new_window(app_data);
    }

    char *path = realpath(file_name, NULL);
    FileData file_data = check_file(path);
    free(path);
    path = NULL;

    if (!file_data.exists) return ERROR_FILE_NOT_EXISTS;

    if (!file_data.is_file) return ERROR_NOT_A_FILE;


    if (app_data->windows[app_data->window_focused]->filename != NULL) {
        free(app_data->windows[app_data->window_focused]->filename);
    }

    app_data->windows[app_data->window_focused]->filename = read_file(file_name, app_data->windows[app_data->window_focused]->tables_array);
    return NO_ERROR;
}

ErrorKind panelcmd_j_cb(ApplicationData *app_data) {
    if (app_data->window_focused < app_data->windows_qtty - 1) {
        app_data->window_focused++;
    }

    return NO_ERROR;
}

ErrorKind panelcmd_k_cb(ApplicationData *app_data) {
    if (app_data->window_focused > 0) {
        app_data->window_focused--;
    }

    return NO_ERROR;
}

bool set_cb(ApplicationData *app_data, const int argc, const char *argv[]) {
    const char *config = argv[0];

    if (strcmp(config, "bar_offset") == 0) { // :set bar_offset (number)
        if (argc != 2) return ERROR_WRONG_AMOUNT_ARGS;
        const char *value = argv[1];

        if (!parse_u64(value, &app_data->config.bar_offset)) return ERROR_WRONG_ARGS;
    }
    else if (strcmp(config, "stage_width") == 0) { // :set stage_width (number)
        if (argc != 2) return ERROR_WRONG_AMOUNT_ARGS;
        const char* value = argv[1];
        if (!parse_u64(value, &app_data->config.stage_width)) return ERROR_WRONG_ARGS;
    }
    else if (strcmp(config, "color") == 0) { // :set color (element) blue|red|yellow|green|white|black|cyan|magenta blue|red|yellow|green|white|black|cyan|magenta <bold>
        if (argc != 4 && argc != 5) return ERROR_WRONG_AMOUNT_ARGS;

        const char* element = argv[1];
        const char* fg = argv[2];
        const char* bg = argv[3];

        bool set_bold = false;

        if (argc == 5) {
            const char* bold = argv[4];

            if (strcmp(bold, "bold") == 0) {
                set_bold = true;
            }
            else return ERROR_WRONG_ARGS;
        }

        int color_idx = 0;
        uint64_t i;
        for (i = 0; i < COLORS_AMOUNT; i++) {
            if (strcmp(element, colors_names[i]) == 0) {
                color_idx = i;
                break;
            }
        }

        if (i == COLORS_AMOUNT) return ERROR_WRONG_ARGS;

        short set_fg = COLOR_BLACK;

        if (strcmp(fg, "black")        == 0) set_fg = COLOR_BLACK;
        else if (strcmp(fg, "white")   == 0) set_fg = COLOR_WHITE;
        else if (strcmp(fg, "red")     == 0) set_fg = COLOR_RED;
        else if (strcmp(fg, "green")   == 0) set_fg = COLOR_GREEN;
        else if (strcmp(fg, "yellow")  == 0) set_fg = COLOR_YELLOW;
        else if (strcmp(fg, "blue")    == 0) set_fg = COLOR_BLUE;
        else if (strcmp(fg, "cyan")    == 0) set_fg = COLOR_CYAN;
        else if (strcmp(fg, "magenta") == 0) set_fg = COLOR_MAGENTA;
        else return ERROR_WRONG_ARGS;

        short set_bg = COLOR_BLACK;

        if (strcmp(bg, "black")        == 0) set_bg = COLOR_BLACK;
        else if (strcmp(bg, "white")   == 0) set_bg = COLOR_WHITE;
        else if (strcmp(bg, "red")     == 0) set_bg = COLOR_RED;
        else if (strcmp(bg, "green")   == 0) set_bg = COLOR_GREEN;
        else if (strcmp(bg, "yellow")  == 0) set_bg = COLOR_YELLOW;
        else if (strcmp(bg, "blue")    == 0) set_bg = COLOR_BLUE;
        else if (strcmp(bg, "cyan")    == 0) set_bg = COLOR_CYAN;
        else if (strcmp(bg, "magenta") == 0) set_bg = COLOR_MAGENTA;
        else return ERROR_WRONG_ARGS;

        set_color(app_data, color_idx, set_bg, set_fg, set_bold);
        apply_colors(app_data);
    }


    return NO_ERROR;
}

ErrorKind panelsync_cb(ApplicationData *app_data) {
    app_data->windows_synced = true;
    return NO_ERROR;
}

ErrorKind paneldesync_cb(ApplicationData *app_data) {
    app_data->windows_synced = false;
    return NO_ERROR;
}

ErrorKind findpc_cb(ApplicationData *app_data, const char * argv[]) {
    const char *pattern = argv[0];

    if (app_data->windows == NULL) return ERROR_NO_WINDOW;

    FindResult result = find(app_data->windows[app_data->window_focused]->tables_array, pattern, PC, DOWN, app_data->windows[app_data->window_focused]->first_instruction);
    if (!result.valid) return ERROR_NO_RESULT;

    app_data->windows[app_data->window_focused]->first_instruction = result.position;

    if (app_data->windows[app_data->window_focused]->last_search.pattern != NULL) {
        free(app_data->windows[app_data->window_focused]->last_search.pattern);
    }

    app_data->windows[app_data->window_focused]->last_search.pattern = malloc(strlen(pattern) + 1);

    strcpy(app_data->windows[app_data->window_focused]->last_search.pattern, pattern);

    app_data->windows[app_data->window_focused]->last_search.data_kind = PC;

    return NO_ERROR;
}

ErrorKind findinst_cb(ApplicationData *app_data, const char * argv[]) {
    const char *pattern = argv[0];

    if (app_data->windows == NULL) return ERROR_NO_WINDOW;

    FindResult result = find(app_data->windows[app_data->window_focused]->tables_array, pattern, INST, DOWN, app_data->windows[app_data->window_focused]->first_instruction);
    if (!result.valid) return ERROR_NO_RESULT;

    app_data->windows[app_data->window_focused]->first_instruction = result.position;

    if (app_data->windows[app_data->window_focused]->last_search.pattern != NULL) {
        free(app_data->windows[app_data->window_focused]->last_search.pattern);
    }

    app_data->windows[app_data->window_focused]->last_search.pattern = malloc(strlen(pattern) + 1);

    strcpy(app_data->windows[app_data->window_focused]->last_search.pattern, pattern);

    app_data->windows[app_data->window_focused]->last_search.data_kind = INST;

    return NO_ERROR;
}

ErrorKind next_cb(ApplicationData *app_data) {
    if (app_data->windows == NULL) return ERROR_NO_WINDOW;

    if (app_data->windows[app_data->window_focused]->last_search.pattern == NULL) return false;

    char *pattern = app_data->windows[app_data->window_focused]->last_search.pattern;

    FindResult result = find(app_data->windows[app_data->window_focused]->tables_array, pattern, app_data->windows[app_data->window_focused]->last_search.data_kind, DOWN, app_data->windows[app_data->window_focused]->first_instruction + 1);
    if (!result.valid) return ERROR_NO_RESULT;

    app_data->windows[app_data->window_focused]->first_instruction = result.position;

    return NO_ERROR;
}

ErrorKind prev_cb(ApplicationData *app_data) {
    if (app_data->windows == NULL) return ERROR_NO_WINDOW;

    if (app_data->windows[app_data->window_focused]->last_search.pattern == NULL) return false;

    char *pattern = app_data->windows[app_data->window_focused]->last_search.pattern;

    if (app_data->windows[app_data->window_focused]->first_instruction <= 0) return false;

    FindResult result = find(app_data->windows[app_data->window_focused]->tables_array, pattern, app_data->windows[app_data->window_focused]->last_search.data_kind, UP, app_data->windows[app_data->window_focused]->first_instruction - 1);
    if (!result.valid) return ERROR_NO_RESULT;

    app_data->windows[app_data->window_focused]->first_instruction = result.position;

    return NO_ERROR;
}

ErrorKind quit_cb(ApplicationData *app_data) {
    app_data->quit_requested = true;
    return NO_ERROR;
}

ErrorKind createtimeline_cb(ApplicationData *app_data, const int argc, const char *argv[]) {
    // createtimeline [cycle]
    if (argc > 1) return ERROR_WRONG_AMOUNT_ARGS;

    if (app_data->windows == NULL) return ERROR_NO_WINDOW;

    WindowData *win_data = app_data->windows[app_data->window_focused];

    uint64_t cycle = win_data->first_cycle;

    if (argc == 1) {
        if (!parse_u64(argv[0], &cycle)) return ERROR_WRONG_ARGS;
    }

    win_data->timelines_amount++;
    win_data->timelines = realloc(win_data->timelines, win_data->timelines_amount*sizeof(uint64_t));

    win_data->timelines[win_data->timelines_amount - 1] = cycle;

    // Order timelines
    for (uint64_t i = 0; i < win_data->timelines_amount - 1; i++) {
        for (uint64_t j = 0; j < win_data->timelines_amount - 1 - i; j++) {
            if (win_data->timelines[j] > win_data->timelines[j + 1]) {
                uint64_t temp = win_data->timelines[j];
                win_data->timelines[j] = win_data->timelines[j + 1];
                win_data->timelines[j + 1] = temp;
            }
        }
    }

    return NO_ERROR;
}


ErrorKind removetimeline_cb(ApplicationData *app_data, const char *argv[]) {
    if (app_data->windows == NULL) return ERROR_NO_WINDOW;
    WindowData *win_data = app_data->windows[app_data->window_focused];

    uint64_t timeline_idx;
    if (!parse_u64(argv[0], &timeline_idx)) return ERROR_WRONG_ARGS;

    if ((timeline_idx > win_data->timelines_amount - 1) || (win_data->timelines_amount == 0)) return ERROR_IDX_BIGGER;

    uint64_t *new_timelines = malloc((win_data->timelines_amount - 1)*sizeof(uint64_t));

    for (uint64_t i = 0; i < timeline_idx; i++) {
        new_timelines[i] = win_data->timelines[i];
    }

    for (uint64_t i = timeline_idx + 1; i < win_data->timelines_amount - 1; i++) {
        new_timelines[i-1] = win_data->timelines[i];
    }
    win_data->timelines_amount--;

    free(win_data->timelines);
    win_data->timelines = new_timelines;

    return NO_ERROR;
}

ErrorKind movetimeline_cb(ApplicationData *app_data, const char *argv[]) {
    if (app_data->windows == NULL) return ERROR_NO_WINDOW;

    WindowData *win_data = app_data->windows[app_data->window_focused];

    uint64_t timeline_idx;
    if (!parse_u64(argv[0], &timeline_idx)) return ERROR_WRONG_ARGS;

    if ((timeline_idx > win_data->timelines_amount - 1) || (win_data->timelines_amount == 0)) return ERROR_IDX_BIGGER;

    uint64_t new_cycle;
    if (!parse_u64(argv[1], &new_cycle)) return ERROR_WRONG_ARGS;

    win_data->timelines[timeline_idx] = new_cycle;
    
    // Order timelines
    for (uint64_t i = 0; i < win_data->timelines_amount - 1; i++) {
        for (uint64_t j = 0; j < win_data->timelines_amount - 1 - i; j++) {
            if (win_data->timelines[j] > win_data->timelines[j + 1]) {
                uint64_t temp = win_data->timelines[j];
                win_data->timelines[j] = win_data->timelines[j + 1];
                win_data->timelines[j + 1] = temp;
            }
        }
    }

    return NO_ERROR;
}

// -------------------- Autocomplete implementation --------------------

// Helper: check if 's' starts with prefix 'prefix'
static bool starts_with(const char *s, const char *prefix) {
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

// Tokenize a command string into tokens (returns count, tokens stored in
// provided argv array which must be large enough). Does not modify input.
// Works similarly to split_command_arguments but without destruction.
static int tokenize_command(const char *command, char **tokens, int max_tokens) {
    int argc = 0;
    const char *p = command;

    while (*p != '\0' && argc < max_tokens) {
        while (isspace((unsigned char)*p)) p++;
        if (*p == '\0') break;
        const char *start = p;
        while (*p != '\0' && !isspace((unsigned char)*p)) p++;
        int len = p - start;
        tokens[argc] = malloc(len + 1);
        memcpy(tokens[argc], start, len);
        tokens[argc][len] = '\0';
        argc++;
    }

    return argc;
}

static void free_tokens(char **tokens, int argc) {
    for (int i = 0; i < argc; ++i) free(tokens[i]);
}

// Expand leading ~ in a simple way. Returns malloc'd string or NULL on error.
static char *expand_path(const char *in) {
    if (in == NULL) return NULL;

    // Handle leading ~
    if (in[0] == '~') {
        const char *rest = in + 1;
        const char *slash = strchr(rest, '/');
        char user[256] = {0};
        if (slash == NULL) {
            strncpy(user, rest, sizeof(user)-1);
            rest = "";
        } else {
            size_t ulen = slash - rest;
            if (ulen >= sizeof(user)) ulen = sizeof(user)-1;
            memcpy(user, rest, ulen);
            user[ulen] = '\0';
            rest = slash;
        }

        const char *home = NULL;
        if (user[0] == '\0') {
            home = getenv("HOME");
        } else {
            struct passwd *pw = getpwnam(user);
            if (pw != NULL) home = pw->pw_dir;
        }
        if (home == NULL) return strdup(in); // leave unchanged if we can't expand

        char *out = malloc(strlen(home) + strlen(rest) + 1);
        strcpy(out, home);
        strcat(out, rest);
        return out;
    }

    return strdup(in);
}

// Filesystem completion: given token, return malloc'd remainder (or NULL)
// The remainder is what should be appended to the token to complete the match.
static char *complete_filesystem(const char *token) {
    char *expanded = expand_path(token);
    if (expanded == NULL) return NULL;

    const char *dir = NULL;
    const char *base = NULL;
    char dirbuf[PATH_MAX];

    char *slash = strrchr(expanded, '/');
    if (slash != NULL) {
        size_t dlen = slash - expanded + 1; // include '/'
        if (dlen >= sizeof(dirbuf)) { free(expanded); return NULL; }
        memcpy(dirbuf, expanded, dlen);
        dirbuf[dlen] = '\0';
        dir = dirbuf;
        base = slash + 1;
    } else {
        dir = "./";
        base = expanded;
    }

    DIR *d = opendir(dir[0] == '\0' ? "." : dir);
    if (!d) { free(expanded); return NULL; }

    struct dirent *ent;
    char *chosen = NULL;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue; // ignore hidden files/directories (., .., .git, ...)
        if (starts_with(ent->d_name, base)) {
            // pick the first match in directory iteration order
            chosen = strdup(ent->d_name);
            break;
        }
    }
    closedir(d);

    if (!chosen) { free(expanded); return NULL; }

    // If chosen is a directory, append '/'
    char fullpath[PATH_MAX];
    snprintf(fullpath, sizeof(fullpath), "%s%s", dir, chosen);
    struct stat st;
    bool is_dir = (stat(fullpath, &st) == 0) && S_ISDIR(st.st_mode);

    size_t baselen = strlen(base);
    size_t textlen = strlen(chosen) - baselen;
    char *remainder = malloc(textlen + (is_dir ? 1 : 0) + 1);
    memcpy(remainder, chosen + baselen, textlen);
    size_t pos = textlen;
    if (is_dir) remainder[pos++] = '/';
    remainder[pos] = '\0';

    free(chosen);
    free(expanded);
    return remainder;
}

// Given a set of candidate strings, return the first that starts with partial
// and return malloc'd remainder or NULL.
static char *complete_from_list(const char *partial, const char * const candidates[], size_t n) {
    size_t plen = strlen(partial);
    for (size_t i = 0; i < n; ++i) {
        if (starts_with(candidates[i], partial)) {
            const char *cand = candidates[i];
            size_t clen = strlen(cand);
            char *rem = malloc(clen - plen + 1);
            memcpy(rem, cand + plen, clen - plen);
            rem[clen - plen] = '\0';
            return rem;
        }
    }
    return NULL;
}

// Main compute function. Returns malloc'd remainder or NULL.
char *compute_autocomplete_suggestion(ApplicationData *app_data) {
    if (app_data->mode != COMMAND) return NULL;
    const char *cmd = app_data->command ? app_data->command : "";

    // Determine if trailing whitespace
    size_t len = strlen(cmd);
    bool trailing_ws = (len > 0) && isspace((unsigned char)cmd[len - 1]);

    // Tokenize into small array
    char *tokens[32];
    int argc = tokenize_command(cmd, tokens, 32);

    // If the command is empty and no trailing space, suggest top-level
    size_t current_len = 0;
    const Command *current_commands = get_root_commands(&current_len);

    int idx = 0;
    // traverse full tokens except possibly last
    while (idx < argc) {
        // if this is the last token and user hasn't finished it, break to complete it
        if (idx == argc - 1 && !trailing_ws) break;

        // match tokens[idx]
        bool matched = false;
        for (size_t i = 0; i < current_len; ++i) {
            if (strcmp(tokens[idx], current_commands[i].cmd) != 0) continue;
            matched = true;
            // descend for subcommand
            if (current_commands[i].type == COMMAND_TYPE_SUBCOMMAND) {
                current_commands = current_commands[i].subcommand.subcommands;
                current_len = current_commands[i].subcommand.subcommands_length; // note: using previous i is fine
                break;
            } else {
                // leaf command (no subcommands): remaining tokens are arguments
                current_commands = NULL;
                current_len = 0;
                break;
            }
        }
        if (!matched) {
            // unmatched root token means no suggestions; deeper tokens are arguments
            if (idx == 0) {
                free_tokens(tokens, argc);
                return NULL;
            }
            break;
        }
        idx++;
        if (current_commands == NULL) break;
    }

    // cursor position: the token being typed (last token) or a new argument after trailing whitespace
    if (argc > 0) idx = trailing_ws ? argc : argc - 1;

    // now we want to complete the token at position idx (either an empty token if trailing_ws or a partial token)
    const char *partial = "";
    if (idx < argc && !trailing_ws) partial = tokens[idx];

    char *result = NULL;

    // Determine the command name (first token)
    const char *command_name = (argc > 0) ? tokens[0] : "";

    // If idx == 0, completing command name among current_commands
    if (idx == 0) {
        // complete from COMMANDS list
        for (size_t i = 0; i < current_len; ++i) {
            if (starts_with(current_commands[i].cmd, partial)) {
                result = malloc(strlen(current_commands[i].cmd) - strlen(partial) + 1);
                strcpy(result, current_commands[i].cmd + strlen(partial));
                free_tokens(tokens, argc);
                return result;
            }
        }
        free_tokens(tokens, argc);
        return NULL;
    }

    // Handle panelcmd subcommands
    if (strcmp(command_name, "panelcmd") == 0) {
        size_t pcnt = 0;
        const Command *pcmds = get_panel_commands(&pcnt);
        if (idx == 1) {
            // complete second token
            for (size_t i = 0; i < pcnt; ++i) {
                if (starts_with(pcmds[i].cmd, partial)) {
                    result = malloc(strlen(pcmds[i].cmd) - strlen(partial) + 1);
                    strcpy(result, pcmds[i].cmd + strlen(partial));
                    free_tokens(tokens, argc);
                    return result;
                }
            }
            free_tokens(tokens, argc);
            return NULL;
        }
    }

    // Handle open (filesystem completion) when command is "open" (or alias "o") and idx==1
    if ((strcmp(command_name, "open") == 0 || strcmp(command_name, "o") == 0) && idx == 1) {
        // token may be empty
        result = complete_filesystem(partial);
        free_tokens(tokens, argc);
        return result;
    }

    // Handle set command
    if (strcmp(command_name, "set") == 0) {
        // idx 1: first arg options
        if (idx == 1) {
            const char *opts[] = {"bar_offset", "stage_width", "color"};
            result = complete_from_list(partial, opts, 3);
            free_tokens(tokens, argc);
            return result;
        }
        // If first arg is "color", suggest element names, then fg/bg, then bold
        if (argc >= 2 && strcmp(tokens[1], "color") == 0) {
            if (idx == 2) {
                // element names from colors_names
                result = complete_from_list(partial, (const char * const *)colors_names, COLORS_AMOUNT);
                free_tokens(tokens, argc);
                return result;
            }
            if (idx == 3) {
                const char *cols[] = {"black","white","red","green","yellow","blue","cyan","magenta"};
                result = complete_from_list(partial, cols, 8);
                free_tokens(tokens, argc);
                return result;
            }
            if (idx == 4) {
                const char *cols[] = {"black","white","red","green","yellow","blue","cyan","magenta"};
                result = complete_from_list(partial, cols, 8);
                free_tokens(tokens, argc);
                return result;
            }
            if (idx == 5) {
                const char *opt[] = {"bold"};
                result = complete_from_list(partial, opt, 1);
                free_tokens(tokens, argc);
                return result;
            }
        }
        free_tokens(tokens, argc);
        return NULL;
    }

    // Not inside a command node list (leaf command reached): nothing generic to complete
    if (current_len == 0) {
        free_tokens(tokens, argc);
        return NULL;
    }

    // Default: try to complete among available commands at this node
    for (size_t i = 0; i < current_len; ++i) {
        if (starts_with(current_commands[i].cmd, partial)) {
            result = malloc(strlen(current_commands[i].cmd) - strlen(partial) + 1);
            strcpy(result, current_commands[i].cmd + strlen(partial));
            break;
        }
    }

    free_tokens(tokens, argc);
    return result;
}

// Accept the suggestion by appending it to the command buffer
bool accept_autocomplete_suggestion(ApplicationData *app_data) {
    char *sugg = compute_autocomplete_suggestion(app_data);
    if (sugg == NULL) return false;

    size_t cur = app_data->command ? strlen(app_data->command) : 0;
    size_t add = strlen(sugg);
    app_data->command = realloc(app_data->command, cur + add + 1);
    memcpy(app_data->command + cur, sugg, add + 1);
    free(sugg);
    return true;
}
