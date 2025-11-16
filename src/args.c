#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "args.h"

void print_usage(const char *prog) {
    fprintf(stderr,
        "Použití: %s -s server [-p port] -f filter_file\n"
        "\nPopis parametrů:\n"
        "  -s server        IP adresa nebo doménové jméno DNS serveru\n"
        "  -p port          Port DNS serveru (výchozí 53)\n"
        "  -f filter_file   Soubor obsahující nežádoucí domény\n",
        prog
    );
}

bool parse_args(int argc, char **argv, Args *out) {
    out->server = NULL;
    out->filter_file = NULL;
    out->port = 53; // default port

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0) {
            if (i + 1 >= argc) return false;
            out->server = argv[++i];

        } else if (strcmp(argv[i], "-f") == 0) {
            if (i + 1 >= argc) return false;
            out->filter_file = argv[++i];

        } else if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 >= argc) return false;
            out->port = atoi(argv[++i]);
            if (out->port <= 0 || out->port > 65535) {
                fprintf(stderr, "Neplatný port: %d\n", out->port);
                return false;
            }
        } else if (strcmp(argv[i], "-v") == 0) {
            out->verbose = true; 
        } else {
            fprintf(stderr, "Neznámý parametr: %s\n", argv[i]);
            return false;
        }
    }

    // required args missing?
    if (!out->server || !out->filter_file) {
        return false;
    }

    return true;
}
