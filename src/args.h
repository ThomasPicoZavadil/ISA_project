#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>

/**
 * @struct Args
 * @brief Stores parsed command-line arguments for the DNS proxy application.
 *
 * Members:
 *  - server:       Hostname or IP address of the upstream DNS resolver (required).
 *  - filter_file:  Path to a file containing blocked domain names (required).
 *  - verbose:      If true, prints additional diagnostic information.
 *  - port:         Local port on which the proxy listens (default: 53).
 */
typedef struct {
    const char *server;
    const char *filter_file;
    bool verbose;
    int port;
} Args;

/**
 * @brief Parse command-line arguments and fill an Args structure.
 *
 * Supported options:
 *   -s <server>       Upstream DNS server (required)
 *   -f <filter_file>  File with list of blocked domains (required)
 *   -p <port>         Local listening port (optional, default 53)
 *   -v                Enable verbose diagnostic output (optional)
 *
 * @param argc  Number of command-line arguments.
 * @param argv  Array of argument strings.
 * @param out   Pointer to Args structure that will receive parsed values.
 *
 * @return true if parsing succeeded and required arguments are present,
 *         false otherwise (missing or invalid arguments).
 */
bool parse_args(int argc, char **argv, Args *out);

/**
 * @brief Print usage/help instructions for the program.
 *
 * This function outputs a short help message describing available
 * command-line parameters and their meaning.
 *
 * @param prog  Name of the executable, typically argv[0].
 */
void print_usage(const char *prog);

#endif // ARGS_H
