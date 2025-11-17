/************************************
*Jméno autora: Tomáš Zavadil
*Login: xzavadt00
************************************/

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h> // for strcasecmp
#include "filter.h"

#define MAX_LINE_LEN 256

// Helper: convert string to lowercase in-place
static void strtolower_inplace(char *s) {
    for (; *s; ++s) *s = tolower((unsigned char)*s);
}

// Load filter file
bool filter_load(const char *filename, FilterList *out) {
    FILE *f = fopen(filename, "r");
    if (!f) return 0;

    out->count = 0;
    while (!feof(f) && out->count < MAX_FILTER_DOMAINS) {
        char line[MAX_LINE_LEN];
        if (!fgets(line, sizeof(line), f)) break;

        // remove trailing newline
        line[strcspn(line, "\r\n")] = '\0';

        // skip empty lines or comments
        if (line[0] == '\0' || line[0] == '#') continue;

        char *domain = strdup(line);
        if (!domain) {
            fclose(f);
            return 0;
        }

        strtolower_inplace(domain);
        out->domains[out->count] = domain;
        out->count++;
    }

    fclose(f);
    return 1;
}

// Free filter list
void filter_free(FilterList *list) {
    for (int i = 0; i < list->count; i++) {
        free(list->domains[i]);
    }
    list->count = 0;
}

// Check if domain is blocked
bool filter_is_blocked(const FilterList *list, const char *domain) {
    char tmp[strlen(domain) + 1];
    strcpy(tmp, domain);

    // remove trailing dot
    size_t len = strlen(tmp);
    if (len > 0 && tmp[len - 1] == '.') tmp[len - 1] = '\0';

    strtolower_inplace(tmp);

    for (int i = 0; i < list->count; i++) {
        const char *f = list->domains[i];
        size_t flen = strlen(f);
        if (flen == 0) continue;

        // Wildcard at the start (*.example.com)
        if (f[0] == '*') {
            const char *suffix = f + 1; // skip '*'
            size_t slen = flen - 1;
            size_t dlen = strlen(tmp);

            // Match if domain ends with the suffix
            if (dlen >= slen &&
                strcasecmp(tmp + dlen - slen, suffix) == 0) {
                return 1;
            }
        } else {
            // Exact match or subdomain match
            // example.com matches:
            //   - example.com (exact)
            //   - sub.example.com (subdomain)
            //   - deep.sub.example.com (subdomain)
            
            size_t dlen = strlen(tmp);
            
            // Exact match
            if (strcasecmp(tmp, f) == 0) {
                return 1;
            }
            
            // Subdomain match
            // Check if tmp ends with ".filter_domain"
            if (dlen > flen + 1) {  // +1 for the dot
                // Check if there's a dot before the matching part
                if (tmp[dlen - flen - 1] == '.' &&
                    strcasecmp(tmp + dlen - flen, f) == 0) {
                    return 1;
                }
            }
        }
    }

    return 0;
}