/* Autocomplete utilities for izumi_tui
 * Provides functions to compute and accept inline suggestions for the
 * command line. Implemented in commands.c so it can access COMMANDS.
 */
#ifndef AUTOCOMPLETE_H
#define AUTOCOMPLETE_H

#include "window.h"

// Returns a malloc'd string containing the text that should be appended
// to app_data->command as a suggestion (i.e. the remainder after the
// currently typed token). The caller must free() the returned buffer.
// Returns NULL if there's no suggestion.
char *compute_autocomplete_suggestion(ApplicationData *app_data);

// Accepts the current suggestion and appends it to app_data->command.
// Returns true if anything was appended.
bool accept_autocomplete_suggestion(ApplicationData *app_data);

#endif
