/*
 * This file is par of Izumi.
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

#include <complex.h>
#include "system_curses.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <unistd.h>
#include <stdint.h>

#include "window.h"
#include "config.h"
#include "interact.h"
#include "configure.h"
#include "error.h"
#include "autocomplete.h"
#include "i18n.h"

void get_window_data(WindowData *win_data, ApplicationData *app_data) {
    uint64_t screen_height = getmaxy(stdscr);
    win_data->width = getmaxx(stdscr);
    win_data->height = app_data->windows_qtty ?
        (screen_height > 1 ? screen_height-1 : 1)/app_data->windows_qtty : 1;
    if (win_data->height == 0) win_data->height = 1;

    win_data->x = 0;
    win_data->y = win_data->height * win_data->index;
    if (win_data->y >= screen_height) win_data->y = screen_height - 1;
}

void new_window(ApplicationData *app_data) {
    WindowData *win_data = malloc(sizeof(WindowData));
    win_data->index = app_data->windows_qtty;

    app_data->windows_qtty++;
    app_data->windows = realloc(app_data->windows, app_data->windows_qtty * sizeof(WindowData *));
    app_data->windows[app_data->windows_qtty - 1] = win_data;

    get_window_data(win_data, app_data);

    win_data->win = newwin(win_data->height, win_data->width, win_data->y, win_data->x);

    win_data->filename = NULL;
    win_data->first_instruction = 0;
    win_data->first_subrow = 0;
    win_data->first_cycle = 0;

    win_data->tables_array = malloc(sizeof(InstructionTableArray));
    win_data->tables_array->qtty_tables = 0;
    win_data->tables_array->avail_tables = 0;
    win_data->tables_array->tables = NULL;

    win_data->last_search.pattern = NULL;
    win_data->last_search.data_kind = PC;

    win_data->timelines_amount = 0;
    win_data->timelines = NULL;

    win_data->lane_sources_amount = 0;
    win_data->lane_sources = NULL;

    app_data->window_focused = app_data->windows_qtty - 1;
}

void close_window(WindowData *win_data) {
    if (win_data->tables_array != NULL) {
        free_InstructionTableArray(win_data->tables_array);
        free(win_data->tables_array);
        win_data->tables_array = NULL;
    }

    if (win_data->win != NULL) {
        delwin(win_data->win);
        win_data->win = NULL;
    }

    if (win_data->filename != NULL) {
        free(win_data->filename);
        win_data->filename = NULL;
    }

    if (win_data->last_search.pattern != NULL) {
        free(win_data->last_search.pattern);
        win_data->last_search.pattern = NULL;
    }

    clear_lane_sources(win_data);
    free(win_data->timelines);

    free(win_data);
    win_data = NULL;
}

void close_application(ApplicationData *app_data) {
    close_all_panels(app_data);
    free(app_data->command);

    endwin();
    exit(0);
}

void close_all_panels(ApplicationData *app_data) {
    for (uint64_t i = 0; i < app_data->windows_qtty; i++) {
        if (app_data->windows[i] != NULL) {
            close_window(app_data->windows[i]);
        }
    }

    app_data->windows_qtty = 0;
    app_data->window_focused = 0;
    free(app_data->windows);
    app_data->windows = NULL;
}

void close_panel(ApplicationData *app_data, uint64_t panel_id) {
    if (app_data->windows == NULL) return;

        if (panel_id >= app_data->windows_qtty) return;

        close_window(app_data->windows[panel_id]);

        app_data->windows_qtty--;

        for (uint64_t i = panel_id; i < app_data->windows_qtty; ++i) {
            app_data->windows[i] = app_data->windows[i+1];
            app_data->windows[i]->index--;
        }

        app_data->windows = realloc(app_data->windows, app_data->windows_qtty * sizeof(WindowData *));

        if (app_data->window_focused > panel_id) {
            app_data->window_focused--;
        }
        if (app_data->windows_qtty == 0) {
            free(app_data->windows);
            app_data->windows = NULL;
            app_data->window_focused = 0;
        } else if (app_data->window_focused >= app_data->windows_qtty) {
            app_data->window_focused = app_data->windows_qtty - 1;
        }
}

void init_application(ApplicationData *app_data) {
    initscr();
    cbreak();
    noecho();
    refresh();
    keypad(stdscr, TRUE);
    curs_set(0);
    start_color();

    set_color(app_data, COLOR_COMMANDS, COLOR_WHITE, COLOR_BLACK, false);
    set_color(app_data, COLOR_BOX, COLOR_WHITE, COLOR_BLACK, false);
    set_color(app_data, COLOR_TEXT, COLOR_WHITE, COLOR_BLACK, false);
    set_color(app_data, COLOR_TEXT_BOLD, COLOR_WHITE, COLOR_BLACK, true);
    set_color(app_data, COLOR_STATUS, COLOR_BLACK, COLOR_BLUE, true);
    set_color(app_data, COLOR_STAGES + 0, COLOR_BLACK, COLOR_BLUE, true);
    set_color(app_data, COLOR_STAGES + 1, COLOR_BLACK, COLOR_RED, true);
    set_color(app_data, COLOR_STAGES + 2, COLOR_BLACK, COLOR_GREEN, true);
    set_color(app_data, COLOR_STAGES + 3, COLOR_BLACK, COLOR_YELLOW, true);
    set_color(app_data, COLOR_STAGES + 4, COLOR_BLACK, COLOR_MAGENTA, true);
    set_color(app_data, COLOR_STAGES + 5, COLOR_BLACK, COLOR_CYAN, true);
    set_color(app_data, COLOR_ERROR_TEXT, COLOR_RED, COLOR_BLACK, true);
    set_color(app_data, COLOR_TIMELINE, COLOR_BLACK, COLOR_YELLOW, false);

    apply_colors(app_data);

    app_data->windows = NULL;
    app_data->windows_qtty = 0;
    app_data->window_focused = 0;

    app_data->mode = NORMAL;
    app_data->command = NULL;
    app_data->number = 0;
    app_data->windows_synced = false;

    app_data->config.bar_offset = 32;
    app_data->config.stage_width = 3;
    app_data->config.show_empty_rows = true;

    app_data->quit_requested = false;

    app_data->error = NO_ERROR;

    execute_config_commands(app_data, read_config_file(get_config_path()));
}

void main_loop(ApplicationData *app_data) {
    render(app_data);
    while (!app_data->quit_requested) {

        parse_input(app_data, getch());
        render(app_data);
    }
}

void clear_lane_sources(WindowData *win_data) {
    for (uint64_t i = 0; i < win_data->lane_sources_amount; ++i)
        free(win_data->lane_sources[i]);
    free(win_data->lane_sources);
    win_data->lane_sources = NULL;
    win_data->lane_sources_amount = 0;
}

typedef struct {
    uint32_t source;
    uint64_t lane;
} LaneId;

static int compare_lane(const void *a, const void *b) {
    const LaneId *x = a, *y = b;
    if (x->source != y->source) return x->source < y->source ? -1 : 1;
    return (x->lane > y->lane) - (x->lane < y->lane);
}

// Only lane/source pairs which have stage records occupy space. Values need
// not be consecutive or start at zero; sort them, never allocate by max ID.
static uint64_t instruction_lanes(Instruction *inst, LaneId **lanes) {
    *lanes = NULL;
    if (inst == NULL || !inst->valid || inst->stages == NULL || inst->qtty_stages == 0)
        return 0;
    *lanes = malloc(inst->qtty_stages * sizeof(LaneId));
    uint64_t stages = 0;
    for (uint64_t i = 0; i < inst->qtty_stages; ++i)
        if (inst->stages[i].open || inst->stages[i].duration > 0) {
            (*lanes)[stages].source = inst->stages[i].source;
            (*lanes)[stages].lane = inst->stages[i].lane;
            ++stages;
        }
    if (stages == 0) return 0;
    qsort(*lanes, stages, sizeof(LaneId), compare_lane);
    uint64_t count = 1;
    for (uint64_t i = 1; i < stages; ++i)
        if ((*lanes)[i].source != (*lanes)[count-1].source ||
            (*lanes)[i].lane != (*lanes)[count-1].lane)
            (*lanes)[count++] = (*lanes)[i];
    return count;
}

static uint64_t instruction_height(Instruction *inst) {
    LaneId *lanes;
    uint64_t count = instruction_lanes(inst, &lanes);
    free(lanes);
    return count > 1 ? count + 1 : 2;
}

/* Byte length of each character as curses will draw it in the current
 * locale; bytes that are not a valid character are drawn one by one. */
static size_t char_bytes(const char *s, mbstate_t *state) {
    size_t n = mbrlen(s, MB_CUR_MAX, state);
    if (n == 0 || n > MB_CUR_MAX) { /* invalid or incomplete sequence */
        memset(state, 0, sizeof(*state));
        return 1;
    }
    return n;
}

/* Number of characters in `s`. */
static size_t text_chars(const char *s) {
    mbstate_t state = { 0 };
    size_t chars = 0;
    for (size_t i = 0; s[i] != '\0'; i += char_bytes(s + i, &state)) ++chars;
    return chars;
}

/* Byte offset of the `n`-th character of `s`. */
static size_t text_offset(const char *s, size_t n) {
    mbstate_t state = { 0 };
    size_t i = 0;
    for (; s[i] != '\0' && n > 0; --n) i += char_bytes(s + i, &state);
    return i;
}

/* Prints `text` at (y, x) using at most `max` columns. Text that does not
 * fit is shortened in the middle with "..." so both its start and its end
 * (usually the distinguishing part of a file name) stay visible. */
static void print_truncated(WINDOW *win, int y, int x, const char *text, int max) {
    size_t chars = text_chars(text);

    if (max <= 0) return;

    if (chars <= (size_t) max) {
        mvwaddstr(win, y, x, text);
    } else if (max <= 3) {
        mvwaddnstr(win, y, x, text, text_offset(text, max));
    } else {
        size_t keep = max - 3;
        size_t head = text_offset(text, (keep + 1) / 2);
        size_t tail = text_offset(text, chars - keep / 2);

        mvwaddnstr(win, y, x, text, head);
        waddstr(win, "...");
        waddstr(win, text + tail);
    }
}

static void lane_label(WindowData *win_data, LaneId lane, char *buffer, size_t size) {
    if (lane.source == 0) {
        int n = snprintf(buffer, size, "  ");
        snprintf(buffer + n, size - n, _("Lane %lu"), lane.lane);
    } else if (lane.source <= win_data->lane_sources_amount &&
             win_data->lane_sources[lane.source-1] != NULL)
        snprintf(buffer, size, "  %s:%lu",
                 win_data->lane_sources[lane.source-1], lane.lane);
    else
        snprintf(buffer, size, "  src%u:%lu", lane.source, lane.lane);
}

static void print_instruction_rows(ApplicationData *app_data, WindowData *win_data,
        Configuration *config, Instruction *inst, uint64_t y, uint64_t first_cycle,
        uint64_t index, uint64_t skip, uint64_t rows) {
    LaneId *lanes;
    uint64_t lane_count = instruction_lanes(inst, &lanes);
    bool multiple = lane_count > 1;
    uint64_t width = win_data->width;
    uint64_t sidebar = config->bar_offset < width ? config->bar_offset : width;
    uint64_t stride = config->stage_width + 1;
    enable_colors_win(app_data, win_data, COLOR_TEXT);
    for (uint64_t r = 0; r < rows; ++r) {
        uint64_t logical = skip + r;
        if (stride > 1 && sidebar < width)
            for (uint64_t x = sidebar; x < width-1; x += stride)
                mvwaddch(win_data->win, y+r, x, ACS_VLINE);
        if (inst == NULL || !inst->valid || sidebar < 2) continue;
        char *label = NULL;
        if (logical == 0) {
            if (multiple) {
                const char *pc = inst->mem_addr ? inst->mem_addr : "";
                const char *text = inst->instruction ? inst->instruction : "";
                label = malloc(strlen(pc) + strlen(text) + 64);
                if (pc[0] != '\0' && text[0] != '\0')
                    snprintf(label, strlen(pc) + strlen(text) + 64,
                             "%lu %s %s", index, pc, text);
                else if (pc[0] != '\0')
                    snprintf(label, strlen(pc) + strlen(text) + 64,
                             "%lu %s", index, pc);
                else
                    snprintf(label, strlen(pc) + strlen(text) + 64,
                             "%lu %s", index, text);
            } else {
                label = malloc(64);
                snprintf(label, 64, "%lu %s", index, inst->mem_addr ? inst->mem_addr : "");
            }
        } else if (multiple) {
            label = malloc(256);
            lane_label(win_data, lanes[logical-1], label, 256);
        } else if (inst->instruction != NULL) {
            label = malloc(strlen(inst->instruction) + 3);
            snprintf(label, strlen(inst->instruction) + 3, "  %s", inst->instruction);
        }
        if (label != NULL) {
            mvwaddnstr(win_data->win, y+r, 1, label, sidebar-1);
            free(label);
        }
    }
    disable_colors_win(app_data, win_data, COLOR_TEXT);

    if (lane_count != 0 && stride > 1 && sidebar + 1 < width-1) {
        uint64_t left = sidebar + 1, available = width-1-left;
        for (uint64_t i = 0; i < inst->qtty_stages; ++i) {
            Stage *stage = &inst->stages[i];
            if (stage->duration == 0 && !stage->open) continue;
            uint64_t lane = 0;
            while (lanes[lane].source != stage->source ||
                   lanes[lane].lane != stage->lane) ++lane;
            uint64_t logical = multiple ? lane+1 : 1;
            if (logical < skip || logical-skip >= rows || stage->cycle < first_cycle)
                continue;
            uint64_t delta = stage->cycle-first_cycle;
            if (delta > available/stride) continue;
            uint64_t offset = delta*stride;
            if (offset >= available) continue;
            uint64_t duration = stage->duration ? stage->duration : 1;
            uint64_t space = available-offset;
            uint64_t span = duration > space/stride ? space : duration*stride-1;
            uint64_t screen_y = y + logical-skip;
            enable_colors_win(app_data, win_data, COLOR_STAGES + i%6);
            mvwhline(win_data->win, screen_y, left+offset, ' ', span);
            uint64_t name_width = config->stage_width < span ? config->stage_width : span;
            mvwaddnstr(win_data->win, screen_y, left+offset, stage->name, name_width);
            // Mark the last stage on each lane, never another lane's block.
            bool last = true;
            for (uint64_t j = i+1; j < inst->qtty_stages; ++j)
                if (inst->stages[j].source == stage->source &&
                    inst->stages[j].lane == stage->lane) last = false;
            if (last && inst->flushed && duration <= space/stride && duration*stride >= 2)
                mvwaddch(win_data->win, screen_y, left+offset+duration*stride-2, 'X');
            disable_colors_win(app_data, win_data, COLOR_STAGES + i%6);
        }
    }
    free(lanes);
}

void print_instruction(ApplicationData *app_data, WindowData *win_data, Configuration *config,
        Instruction *inst, uint64_t y, uint64_t *first_cycle, uint64_t index) {
    if (inst != NULL && inst->valid)
        for (uint64_t i = 0; i < inst->qtty_stages; ++i)
            if (inst->stages[i].cycle < *first_cycle) *first_cycle = inst->stages[i].cycle;
    if (win_data->height < 2 || y >= win_data->height-1) return;
    uint64_t rows = instruction_height(inst);
    if (rows > win_data->height-1-y) rows = win_data->height-1-y;
    print_instruction_rows(app_data, win_data, config, inst, y, *first_cycle, index, 0, rows);
}

static Instruction *instruction_at(WindowData *win_data, uint64_t index) {
    InstructionTableArray *tables = win_data->tables_array;
    if (tables == NULL || tables->tables == NULL ||
        index/256 >= tables->qtty_tables || tables->tables[index/256] == NULL)
        return NULL;
    return &tables->tables[index/256]->content[index%256];
}

static bool row_has_stages(WindowData *win_data, uint64_t index) {
    Instruction *inst = instruction_at(win_data, index);
    return inst != NULL && inst->valid && inst->stages != NULL && inst->qtty_stages > 0;
}

// Search actual instruction IDs, retaining those IDs for labels and searches.
// Skip absent table ranges in one step rather than scanning every empty slot.
static bool visible_instruction(WindowData *win_data, uint64_t *index, bool forward) {
    InstructionTableArray *tables = win_data->tables_array;
    if (tables == NULL || tables->tables == NULL || tables->qtty_tables == 0)
        return false;
    uint64_t limit = tables->qtty_tables * 256;
    if (*index >= limit) {
        if (forward) return false;
        *index = limit - 1;
    }
    for (;;) {
        if (row_has_stages(win_data, *index)) return true;
        if (tables->tables[*index/256] == NULL) {
            if (forward) {
                *index = (*index/256 + 1) * 256;
                if (*index >= limit) return false;
            } else {
                if (*index/256 == 0) return false;
                *index = (*index/256) * 256 - 1;
            }
        } else if (forward) {
            if (++*index >= limit) return false;
        } else {
            if (*index == 0) return false;
            --*index;
        }
    }
}

void move_visible_instruction(WindowData *win_data, bool forward, uint64_t count) {
    win_data->first_subrow = 0;
    uint64_t index = win_data->first_instruction;
    if (!visible_instruction(win_data, &index, true) &&
        !visible_instruction(win_data, &index, false)) return;
    win_data->first_instruction = index;
    while (count-- > 0) {
        uint64_t next = win_data->first_instruction;
        if (forward) {
            if (next == UINT64_MAX) break;
            ++next;
        } else {
            if (next == 0) break;
            --next;
        }
        if (!visible_instruction(win_data, &next, forward)) break;
        win_data->first_instruction = next;
    }
}

void scroll_instruction_rows(WindowData *win_data, bool forward, uint64_t count,
                             bool show_empty_rows) {
    while (count-- > 0) {
        uint64_t height = instruction_height(instruction_at(win_data, win_data->first_instruction));
        if (forward && win_data->first_subrow + 1 < height) {
            ++win_data->first_subrow;
        } else if (!forward && win_data->first_subrow > 0) {
            --win_data->first_subrow;
        } else {
            uint64_t next = win_data->first_instruction;
            if ((forward && next == UINT64_MAX) || (!forward && next == 0)) break;
            next = forward ? next+1 : next-1;
            if (!show_empty_rows && !visible_instruction(win_data, &next, forward)) break;
            win_data->first_instruction = next;
            win_data->first_subrow = forward ? 0 :
                instruction_height(instruction_at(win_data, next))-1;
        }
    }
}

void render_window(ApplicationData *app_data, WindowData *win_data) {
    werase(win_data->win);

    get_window_data(win_data, app_data);

    wresize(win_data->win, win_data->height, win_data->width);
    mvwin(win_data->win, win_data->y, win_data->x);

    uint64_t shown_timelines = win_data->height > 3 ? win_data->height-3 : 0;
    if (shown_timelines > win_data->timelines_amount) shown_timelines = win_data->timelines_amount;

    uint64_t previous_first_cycle = win_data->first_cycle;
    win_data->first_cycle = UINT64_MAX;

    if (win_data->tables_array != NULL && win_data->height > 2) {
        uint64_t reserved = shown_timelines + (shown_timelines != 0 ? 1 : 0);
        uint64_t budget = win_data->height-2;
        budget = reserved < budget ? budget-reserved : 0;
        typedef struct {
            Instruction *inst;
            uint64_t id, skip, rows;
        } VisibleInstruction;
        VisibleInstruction *visible = calloc(budget+1, sizeof(*visible));
        uint64_t used = 0, count = 0;
        uint64_t index = win_data->first_instruction;
        while (used < budget) {
            if (!app_data->config.show_empty_rows &&
                !visible_instruction(win_data, &index, true)) break;
            if (count == 0 && index != win_data->first_instruction) {
                win_data->first_instruction = index;
                win_data->first_subrow = 0;
            }
            Instruction *inst = instruction_at(win_data, index);
            uint64_t height = instruction_height(inst);
            uint64_t skip = count == 0 ? win_data->first_subrow : 0;
            if (skip >= height) skip = win_data->first_subrow = 0;
            uint64_t rows = height-skip;
            if (rows > budget-used) rows = budget-used;
            visible[count++] = (VisibleInstruction){inst, index, skip, rows};
            used += rows;
            if (inst != NULL && inst->valid)
                for (uint64_t i = 0; i < inst->qtty_stages; ++i)
                    if (inst->stages[i].cycle < win_data->first_cycle)
                        win_data->first_cycle = inst->stages[i].cycle;
            if (index == UINT64_MAX) break;
            ++index;
        }
        // Pick the shared cycle origin before painting any lane. Otherwise a
        // later lane/instruction with an earlier stage shifts only part of the view.
        if (win_data->first_cycle == UINT64_MAX) win_data->first_cycle = previous_first_cycle;
        uint64_t y = 1;
        for (uint64_t i = 0; i < count; ++i) {
            print_instruction_rows(app_data, win_data, &app_data->config,
                visible[i].inst, y, win_data->first_cycle, visible[i].id,
                visible[i].skip, visible[i].rows);
            y += visible[i].rows;
        }
        free(visible);
    }

    // A page containing only empty rows must not wrap the displayed clock to
    // UINT64_MAX. Keep its prior horizontal origin until stages are visible.
    if (win_data->first_cycle == UINT64_MAX)
        win_data->first_cycle = previous_first_cycle;

    if (shown_timelines != 0) {
        for (uint64_t i = 1; i < win_data->width - 1; ++i) {
            mvwaddch(win_data->win, win_data->height - 2 - shown_timelines, i, ACS_HLINE);
        }
    }

    for (uint64_t i = 1; i < win_data->height - 1; ++i) {
        mvwaddch(win_data->win, i, app_data->config.bar_offset, ACS_VLINE);
    }

    enable_colors_win(app_data, win_data, COLOR_BOX);
    box(win_data->win, 0, 0);

    if (win_data->filename != NULL) {
        if (win_data->index == app_data->window_focused) {
            wattron(win_data->win, A_BOLD);
        }

        /* Keep the name left of the vertical bar, where the cycle labels
         * start, and inside the panel border. */
        uint64_t limit = app_data->config.bar_offset;
        if (limit > win_data->width - 1) limit = win_data->width - 1;
        print_truncated(win_data->win, 0, 1, win_data->filename, (int) limit - 1);

        if (win_data->index == app_data->window_focused) {
            wattroff(win_data->win, A_BOLD);
        }
    }

    disable_colors_win(app_data, win_data, COLOR_BOX);
    
    uint64_t init_line_timelines = win_data->height - 1 - shown_timelines;

    for (uint64_t i = 0; i < shown_timelines; ++i) {
        uint64_t cycle_timeline = win_data->timelines[i];

        mvwprintw(win_data->win, init_line_timelines + i, 1, _("Timeline %lu at cycle %lu"), i, cycle_timeline);

        if (app_data->config.bar_offset + 1 < win_data->width &&
            app_data->config.stage_width > 0 && app_data->config.stage_width < UINT64_MAX &&
            (cycle_timeline >= win_data->first_cycle) &&
            ((cycle_timeline <= win_data->first_cycle + (win_data->width - app_data->config.bar_offset - 1)/(app_data->config.stage_width + 1)))) {
            enable_colors_win(app_data, win_data, COLOR_TIMELINE);

            uint64_t bar_pos = app_data->config.bar_offset + 1 + (cycle_timeline - win_data->first_cycle)*(app_data->config.stage_width+1) + (app_data->config.stage_width)/2;
    
            for (uint64_t j = 1; j < init_line_timelines + i + 1; ++j) {
                mvwaddch(win_data->win, j, bar_pos, ACS_VLINE);
            }

            for (uint64_t j = app_data->config.bar_offset + 1; j < bar_pos; j++) {
                mvwaddch(win_data->win, init_line_timelines + i, j, ACS_HLINE);
            }
            
            mvwaddch(win_data->win, init_line_timelines + i, bar_pos, ACS_LRCORNER);
            
            disable_colors_win(app_data, win_data, COLOR_TIMELINE);
        }

    }

    enable_colors_win(app_data, win_data, COLOR_TEXT_BOLD);
    mvwprintw(win_data->win, 0, app_data->config.bar_offset + 1, "v %lu", win_data->first_cycle);

    uint64_t last_cycle = win_data->first_cycle;
    if (app_data->config.bar_offset + 2 < win_data->width &&
        app_data->config.stage_width > 0 && app_data->config.stage_width < UINT64_MAX)
        last_cycle += (win_data->width - 2 - app_data->config.bar_offset)/(app_data->config.stage_width + 1);

    uint64_t start_text = 2;

    uint64_t num = last_cycle;

    while (num != 0) {
        start_text++;
        num /= 10;
    }

    mvwprintw(win_data->win, 0, win_data->width - start_text - 1, "%lu v", last_cycle);

    disable_colors_win(app_data, win_data, COLOR_TEXT_BOLD);

    wnoutrefresh(win_data->win);
}

void render_status_bar(ApplicationData *app_data) {
    // TODO: find a better way
    char clear_status_bar[getmaxx(stdscr) + 1];

    for (int i = 0; i < getmaxx(stdscr); ++i) {
        clear_status_bar[i] = ' ';
    }

    clear_status_bar[getmaxx(stdscr)] = '\0';

    enable_colors_app(app_data, COLOR_TEXT);
    mvprintw(getmaxy(stdscr)-1, 0, "%s", clear_status_bar);
    disable_colors_app(app_data, COLOR_TEXT);

    char *version = VERSION;

    uint64_t length = strlen(version);

    enable_colors_app(app_data, COLOR_BOX);
    mvprintw(getmaxy(stdscr)-1, getmaxx(stdscr) - 7 - length, "Izumi v%s", version);
    disable_colors_app(app_data, COLOR_BOX);

    const char *mode = "NORMAL";

    switch (app_data->mode) {
        case NORMAL:
            /* TRANSLATORS: name of the mode shown in the status bar. */
            mode = _("NORMAL");
            break;
        case COMMAND:
            /* TRANSLATORS: name of the mode shown in the status bar. */
            mode = _("COMMAND");
            break;
    }

    /* The command line and errors start after " <mode> " plus two spaces;
     * measure in characters so translated mode names may be non-ASCII. */
    size_t mode_width = mbstowcs(NULL, mode, 0);
    if (mode_width == (size_t) -1) mode_width = strlen(mode);
    int command_col = mode_width + 4;

    enable_colors_app(app_data, COLOR_STATUS);
    mvprintw(getmaxy(stdscr)-1, 0, " %s ", mode);
    /* The error message is printed over this, so skip it then. */
    if (app_data->mode == NORMAL && app_data->error == NO_ERROR) {
        printw(" ");
        printw(_("Empty rows: %s"),
               app_data->config.show_empty_rows ? _("on") : _("off"));
    }
    disable_colors_app(app_data, COLOR_STATUS);

    if (app_data->mode == COMMAND) {
        enable_colors_app(app_data, COLOR_COMMANDS);
        mvprintw(getmaxy(stdscr)-1, command_col, ":%s", app_data->command);
        /* show inline suggestion in dim color */
        char *suggestion = compute_autocomplete_suggestion(app_data);
        if (suggestion != NULL) {
            int col = command_col + 1 + strlen(app_data->command);
            attron(A_DIM);
            mvprintw(getmaxy(stdscr)-1, col, "%s", suggestion);
            attroff(A_DIM);
            free(suggestion);
        }
        disable_colors_app(app_data, COLOR_COMMANDS);
    }

    if (app_data->error != NO_ERROR) {
        enable_colors_app(app_data, COLOR_ERROR_TEXT);
        mvprintw(getmaxy(stdscr)-1, command_col, "%s", _(error_msg[app_data->error]));
        disable_colors_app(app_data, COLOR_ERROR_TEXT);
        

        app_data->error = NO_ERROR;
    }

}

void render(ApplicationData *app_data) {
    // Stage stdscr (status bar) before the panels: after a resize ncurses
    // clears stdscr, and staging it last would paint that blank screen over
    // the panels until the next keystroke.
    render_status_bar(app_data);
    wnoutrefresh(stdscr);

    for (uint64_t i = 0; i < app_data->windows_qtty; i++) {
        if (app_data->windows[i] != NULL && app_data->windows[i]->win != NULL) {
            render_window(app_data, app_data->windows[i]);
        }
    }

    doupdate();
}

void set_color(ApplicationData *app_data, uint64_t index, short fg, short bg, bool bold) {
    ColorData *colors = app_data->config.colors;

    colors[index].fg = fg;
    colors[index].bg = bg;
    colors[index].bold = bold;
}

void apply_colors(ApplicationData *app_data) {
    ColorData *colors = app_data->config.colors;

    for (uint64_t i = 0; i < COLORS_AMOUNT; ++i) {
        init_pair(i+1, colors[i].fg, colors[i].bg);
    }
}

void enable_colors_app(ApplicationData *app_data, uint64_t index) {
    attron(COLOR_PAIR(index+1));

    ColorData *colors = app_data->config.colors;

    if (colors[index].bold) {
        attron(A_BOLD);
    }
}

void disable_colors_app(ApplicationData *app_data, uint64_t index) {
    attroff(COLOR_PAIR(index+1));

    ColorData *colors = app_data->config.colors;

    if (colors[index].bold) {
        attroff(A_BOLD);
    }
}

void enable_colors_win(ApplicationData *app_data, WindowData *win_data, uint64_t index) {
    wattron(win_data->win, COLOR_PAIR(index+1));

    ColorData *colors = app_data->config.colors;

    if (colors[index].bold) {
        wattron(win_data->win, A_BOLD);
    }
}

void disable_colors_win(ApplicationData *app_data, WindowData *win_data, uint64_t index) {
    wattroff(win_data->win, COLOR_PAIR(index+1));

    ColorData *colors = app_data->config.colors;

    if (colors[index].bold) {
        wattroff(win_data->win, A_BOLD);
    }
}
