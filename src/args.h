#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>

typedef struct {
    const char *server;
    const char *filter_file;
    int port;
} Args;

bool parse_args(int argc, char **argv, Args *out);
void print_usage(const char *prog);

#endif
