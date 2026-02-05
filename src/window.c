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
#include <unistd.h>
#include <stdint.h>

#include "window.h"
#include "config.h"
#include "interact.h"
#include "configure.h"
#include "error.h"

void get_window_data(WindowData *win_data, ApplicationData *app_data) {
    win_data->width = getmaxx(stdscr);
    win_data->height = (getmaxy(stdscr)-1)/app_data->windows_qtty; // one pixel for the status bar

    win_data->x = 0;
    win_data->y = win_data->height * win_data->index;
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
    win_data->first_cycle = 0;

    win_data->tables_array = malloc(sizeof(InstructionTableArray));
    win_data->tables_array->qtty_tables = 0;
    win_data->tables_array->avail_tables = 0;
    win_data->tables_array->tables = NULL;

    win_data->last_search.pattern = NULL;
    win_data->last_search.data_kind = PC;

    win_data->timelines_amount = 0;
    win_data->timelines = NULL;

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

    free(win_data);
    win_data = NULL;
}

void close_application(ApplicationData *app_data) {
    close_all_panels(app_data);

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

        if (app_data->window_focused >= panel_id) {
            app_data->window_focused--;
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

    set_color(app_data, COLOR_COMMANDS, COLOR_BLACK, COLOR_WHITE, false);
    set_color(app_data, COLOR_BOX, COLOR_BLACK, COLOR_WHITE, false);
    set_color(app_data, COLOR_TEXT, COLOR_BLACK, COLOR_WHITE, false);
    set_color(app_data, COLOR_TEXT_BOLD, COLOR_BLACK, COLOR_WHITE, true);
    set_color(app_data, COLOR_STATUS, COLOR_BLUE, COLOR_BLACK, true);
    set_color(app_data, COLOR_STAGES + 0, COLOR_BLUE, COLOR_BLACK, true);
    set_color(app_data, COLOR_STAGES + 1, COLOR_RED, COLOR_BLACK, true);
    set_color(app_data, COLOR_STAGES + 2, COLOR_GREEN, COLOR_BLACK, true);
    set_color(app_data, COLOR_STAGES + 3, COLOR_YELLOW, COLOR_BLACK, true);
    set_color(app_data, COLOR_STAGES + 4, COLOR_MAGENTA, COLOR_BLACK, true);
    set_color(app_data, COLOR_STAGES + 5, COLOR_CYAN, COLOR_BLACK, true);
    set_color(app_data, COLOR_ERROR_STATUS, COLOR_RED, COLOR_BLACK, true);
    set_color(app_data, COLOR_ERROR_TEXT, COLOR_BLACK, COLOR_RED, true);
    set_color(app_data, COLOR_TIMELINE, COLOR_BLACK, COLOR_YELLOW, false);
    set_color(app_data, COLOR_TIMELINE_SELECTED, COLOR_YELLOW, COLOR_BLACK, false);

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

void print_instruction(ApplicationData *app_data, WindowData *win_data, Configuration *config, Instruction *inst, uint64_t y, uint64_t *first_cycle, uint64_t index) {
    enable_colors_win(app_data, win_data, COLOR_TEXT);

    for (uint64_t i = 0; i < win_data->width; ++i) {
        mvwprintw(win_data->win, y, i, " ");
        mvwprintw(win_data->win, y+1, i, " ");
    }


    for (uint64_t i = config->bar_offset; i < win_data->width; i += config->stage_width + 1) {
        for (uint64_t j = 0; j < 2; ++j) {
            mvwaddch(win_data->win, y+j, i, ACS_VLINE);
        }
    }


    if (inst != NULL && inst->valid) {
        if (inst->mem_addr != NULL) {
            mvwprintw(win_data->win, y, 1, "%lu\t%s", index, inst->mem_addr);
        }
        if (inst->instruction != NULL) {
            mvwprintw(win_data->win, y+1, 1, "\t%s", inst->instruction);
        }
    }

    disable_colors_win(app_data, win_data, COLOR_TEXT);

    if (inst != NULL && inst->valid && inst->stages != NULL) {
        for (uint64_t i = 0; i < inst->qtty_stages; ++i) {
            Stage *stage = &inst->stages[i];

            if (stage->cycle < *first_cycle) {
                *first_cycle = stage->cycle;
            }

            uint64_t stage_offset = config->bar_offset + 1 + (config->stage_width + 1)*(stage->cycle - *first_cycle);

            enable_colors_win(app_data, win_data, COLOR_STAGES + i%6);


            if (strlen(stage->name) < config->stage_width) {
                mvwprintw(win_data->win, y+1, stage_offset, "%s", stage->name);

                for (uint64_t j = strlen(stage->name); j < config->stage_width; ++j) {
                    mvwprintw(win_data->win, y+1, stage_offset+j, " ");
                }
            }
            else {
                char name_short[config->stage_width + 1];

                strncpy(name_short, stage->name, config->stage_width);
                name_short[config->stage_width] = '\0';

                mvwprintw(win_data->win, y+1, stage_offset, "%s", name_short);
            }

            if (stage->duration > 1) {
                for (uint64_t j = 1; j < stage->duration; ++j) {
                    for (uint64_t k = 0; k < config->stage_width + 1; ++k) {
                        mvwprintw(win_data->win, y+1, stage_offset+(config->stage_width+1)*j - 1+k, " ");
                    }
                }
            }

            if (i == inst->qtty_stages - 1) {
                if (inst->flushed) {
                    mvwprintw(win_data->win, y+1, stage_offset + ((config->stage_width+1)*stage->duration) - 2, "X");
                }
            }

            disable_colors_win(app_data, win_data, COLOR_STAGES + i%6);
        }
    }
}

void render_window(ApplicationData *app_data, WindowData *win_data) {
    werase(win_data->win);

    get_window_data(win_data, app_data);

    wresize(win_data->win, win_data->height, win_data->width);
    mvwin(win_data->win, win_data->y, win_data->x);

    win_data->first_cycle = UINT64_MAX;

    if (win_data->tables_array != NULL) {
        uint64_t amount_of_insts = (win_data->height - 1)/2;
        
        if (win_data->timelines_amount != 0) {
            amount_of_insts = (win_data->height - 1 - 1 - win_data->timelines_amount)/2;
        }

        for (uint64_t i = 0; i < amount_of_insts; ++i) {
            Instruction *inst = NULL;

            uint64_t index = win_data->first_instruction + i;

            if (index/256 < win_data->tables_array->qtty_tables) {
                if (win_data->tables_array->tables[index/256] != NULL) {
                    inst = &win_data->tables_array->tables[index/256]->content[index%256];
                }
            }

            print_instruction(app_data, win_data, &app_data->config, inst, i*2+1, &win_data->first_cycle, index);
        }
    }

    if (win_data->timelines_amount != 0) {
        for (uint64_t i = 1; i < win_data->width - 1; ++i) {
            mvwaddch(win_data->win, win_data->height - 2 - win_data->timelines_amount, i, ACS_HLINE);
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

        mvwprintw(win_data->win, 0, 1, "%s", win_data->filename);

        if (win_data->index == app_data->window_focused) {
            wattroff(win_data->win, A_BOLD);
        }
    }

    disable_colors_win(app_data, win_data, COLOR_BOX);
    
    uint64_t init_line_timelines = win_data->height - 1 - win_data->timelines_amount;

    for (uint64_t i = 0; i < win_data->timelines_amount; ++i) {
        uint64_t cycle_timeline = win_data->timelines[i];

        mvwprintw(win_data->win, init_line_timelines + i, 1, "Timeline %lu at cycle %lu", i, cycle_timeline);

        if ((cycle_timeline >= win_data->first_cycle) && 
            ((cycle_timeline <= win_data->first_cycle + (win_data->width - app_data->config.bar_offset - 1)/(app_data->config.stage_width + 1)))) {
            enable_colors_win(app_data, win_data, COLOR_TIMELINE_SELECTED);

            uint64_t bar_pos = app_data->config.bar_offset + 1 + (cycle_timeline - win_data->first_cycle)*(app_data->config.stage_width+1) + (app_data->config.stage_width)/2;
    
            for (uint64_t j = 1; j < init_line_timelines + i + 1; ++j) {
                mvwaddch(win_data->win, j, bar_pos, ACS_VLINE);
            }

            for (uint64_t j = app_data->config.bar_offset + 1; j < bar_pos; j++) {
                mvwaddch(win_data->win, init_line_timelines + i, j, ACS_HLINE);
            }
            
            mvwaddch(win_data->win, init_line_timelines + i, bar_pos, ACS_LRCORNER);
            
            disable_colors_win(app_data, win_data, COLOR_TIMELINE_SELECTED);
        }

    }

    enable_colors_win(app_data, win_data, COLOR_TEXT_BOLD);
    mvwprintw(win_data->win, 0, app_data->config.bar_offset + 1, "v %lu", win_data->first_cycle);

    uint64_t last_cycle = win_data->first_cycle + ((win_data->width - 2 - app_data->config.bar_offset)/(app_data->config.stage_width + 1));

    uint64_t start_text = 2;

    uint64_t num = last_cycle;

    while (num != 0) {
        start_text++;
        num /= 10;
    }

    mvwprintw(win_data->win, 0, win_data->width - start_text - 1, "%lu v", last_cycle);

    disable_colors_win(app_data, win_data, COLOR_TEXT_BOLD);

    wrefresh(win_data->win);
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

    char *mode;

    switch (app_data->mode) {
        case NORMAL:
            mode = "NORMAL";
            break;
        case COMMAND:
            mode = "COMMAND";
            break;
    }

    enable_colors_app(app_data, COLOR_STATUS);
    mvprintw(getmaxy(stdscr)-1, 0, " %s ", mode);
    disable_colors_app(app_data, COLOR_STATUS);

    if (app_data->mode == COMMAND) {
        enable_colors_app(app_data, COLOR_COMMANDS);
        mvprintw(getmaxy(stdscr)-1, 11, ":%s", app_data->command);
        disable_colors_app(app_data, COLOR_COMMANDS);
    }

    if (app_data->error != NO_ERROR) {
        enable_colors_app(app_data, COLOR_ERROR_TEXT);
        mvprintw(getmaxy(stdscr)-1, strlen(mode) + 4, "%s", error_msg[app_data->error]);
        disable_colors_app(app_data, COLOR_ERROR_TEXT);
        

        app_data->error = NO_ERROR;
    }

}

void render(ApplicationData *app_data) {
    for (uint64_t i = 0; i < app_data->windows_qtty; i++) {
        if (app_data->windows[i] != NULL && app_data->windows[i]->win != NULL) {
            render_window(app_data, app_data->windows[i]);
        }
    }

    render_status_bar(app_data);

    refresh();
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
        init_pair(i+1, colors[i].bg, colors[i].fg);
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
