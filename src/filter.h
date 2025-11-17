#ifndef FILTER_H
#define FILTER_H

#include <stdbool.h>

/**
 * @brief Maximum number of domain patterns that can be loaded from the filter file.
 *
 * Each entry is stored as a dynamically allocated C string.
 */
#define MAX_FILTER_DOMAINS 1024

/**
 * @struct FilterList
 * @brief Stores a list of blocked domain patterns loaded from a filter file.
 *
 * Domains may be stored as:
 *  - exact matches (e.g., "example.com")
 *  - subdomain patterns using leading wildcard notation (e.g., "*.example.com")
 *
 * Memory ownership:
 *  Each string stored in the @ref domains array is dynamically allocated and must be
 *  released with @ref filter_free after use.
 *
 * Members:
 *  - domains: Array of pointers to domain rule strings.
 *  - count:   Number of valid entries in the list.
 */
typedef struct {
    char *domains[MAX_FILTER_DOMAINS]; /**< Array of dynamically allocated blocked domains */
    int  count;                        /**< Count of valid stored entries */
} FilterList;

/**
 * @brief Load a filter file containing blocked domain rules.
 *
 * The file should contain one domain rule per line. Leading/trailing whitespace
 * is trimmed. Empty lines and comment lines starting with `#` are ignored.
 *
 * Accepted formats include:
 *   `example.com`          – exact domain match
 *   `blocked.org`          – exact domain match
 *   `.sub.example.com`     – blocks subdomains only
 *   `*.example.net`        – wildcard blocking of all subdomains
 *
 * @param filename  Path to the filter file.
 * @param out       Pointer to FilterList to be populated.
 *
 * @return true on success, false on failure (I/O error, invalid format, too many entries).
 */
bool filter_load(const char *filename, FilterList *out);

/**
 * @brief Free all dynamically allocated entries in a FilterList.
 *
 * Must be called when the filter list is no longer needed to avoid memory leaks.
 *
 * @param list  Pointer to a FilterList previously initialized by @ref filter_load.
 */
void filter_free(FilterList *list);

/**
 * @brief Determine whether a domain name is blocked.
 *
 * Performs a rule-based lookup against all loaded filter patterns.
 * Matching behavior (typical implementation):
 *   - Exact match blocks the domain.
 *   - A pattern beginning with `"*."` or `"."` blocks the domain and its subdomains.
 *
 * Example:
 *   Rule: "example.com"      blocks "example.com" only
 *   Rule: "*.example.com"    blocks "test.example.com", "a.b.example.com", etc.
 *   Rule: ".example.com"     same behavior as "*.example.com"
 *
 * @param list    Pointer to initialized FilterList.
 * @param domain  Domain name to check (null-terminated).
 *
 * @return true if the domain is blocked, false otherwise.
 */
bool filter_is_blocked(const FilterList *list, const char *domain);

#endif // FILTER_H
