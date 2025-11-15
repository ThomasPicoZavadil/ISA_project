#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "args.h"
#include "filter.h"
#include "dns.h"
#include "resolver.h"

#define BUF_SIZE 512
#define DEFAULT_TIMEOUT 5  // seconds

int main(int argc, char **argv) {
    Args args;
    if (!parse_args(argc, argv, &args)) {
        print_usage(argv[0]);
        return 1;
    }

    // Load filter file
    FilterList filters;
    if (!filter_load(args.filter_file, &filters)) {
        fprintf(stderr, "Chyba: nelze načíst filter file.\n");
        return 2;
    }

    // Open UDP socket to listen on specified port
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        filter_free(&filters);
        return 3;
    }

    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(args.port);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        perror("bind");
        close(sock);
        filter_free(&filters);
        return 4;
    }

    printf("DNS proxy listening on port %d\n", args.port);

    uint8_t buf[BUF_SIZE];

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        ssize_t r = recvfrom(sock, buf, sizeof(buf), 0,
                             (struct sockaddr*)&client_addr, &client_len);
        if (r < 0) {
            perror("recvfrom");
            continue;
        }

        DnsQuestion q;
        if (!dns_parse_question(buf, r, &q)) {
            fprintf(stderr, "Malformed DNS query received\n");
            continue; // ignore invalid queries
        }

        uint8_t response[BUF_SIZE];
        int response_len;

        // Only handle type A queries
        if (q.qtype != DNS_TYPE_A) {
            fprintf(stderr, "Non-A query received: %s\n", q.qname);
            dns_build_error_response(buf, r, response, &response_len, 4); // NOTIMP
            sendto(sock, response, response_len, 0,
                   (struct sockaddr*)&client_addr, client_len);
            continue;
        }

        // Check filter
        if (filter_is_blocked(&filters, q.qname)) {
            fprintf(stderr, "Blocked domain: %s\n", q.qname);
            dns_build_error_response(buf, r, response, &response_len, 3); // NXDOMAIN
            sendto(sock, response, response_len, 0,
                   (struct sockaddr*)&client_addr, client_len);
            continue;
        }

        // Forward query to upstream resolver
        if (!resolver_forward_query(args.server,
                                    buf, r, response, &response_len,
                                    DEFAULT_TIMEOUT)) {
            fprintf(stderr, "Failed to query upstream resolver for %s\n", q.qname);
            dns_build_error_response(buf, r, response, &response_len, 2); // SERVFAIL
            sendto(sock, response, response_len, 0,
                   (struct sockaddr*)&client_addr, client_len);
            continue;
        }

        // Send upstream response back to client
        ssize_t sent = sendto(sock, response, response_len, 0,
                              (struct sockaddr*)&client_addr, client_len);
        if (sent < 0) {
            perror("sendto");
        }
    }

    close(sock);
    filter_free(&filters);
    return 0;
}
