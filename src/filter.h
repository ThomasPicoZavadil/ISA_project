#ifndef FILTER_H
#define FILTER_H

#include <stdbool.h>

#define MAX_FILTER_DOMAINS 1024

// Holds loaded filter domain rules
typedef struct {
    char *domains[MAX_FILTER_DOMAINS];   // array of blocked patterns
    int count;        // number of entries
} FilterList;

/**
 * Loads filter file.
 * Returns true on success, false on error.
 */
bool filter_load(const char *filename, FilterList *out);

/**
 * Frees memory allocated for filter list.
 */
void filter_free(FilterList *list);

/**
 * Checks whether given domain is blocked.
 * Returns true if domain matches any filter entry.
 */
bool filter_is_blocked(const FilterList *list, const char *domain);

#endif
