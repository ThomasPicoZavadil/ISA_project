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

    // Open UDP socket with IPv6 (dual-stack - supports both IPv4 and IPv6)
    int sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        filter_free(&filters);
        return 3;
    }

    // Enable dual-stack mode (accept both IPv4 and IPv6)
    int ipv6only = 0;
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &ipv6only, sizeof(ipv6only)) < 0) {
        perror("setsockopt IPV6_V6ONLY");
        close(sock);
        filter_free(&filters);
        return 3;
    }

    struct sockaddr_in6 local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin6_family = AF_INET6;
    local_addr.sin6_port = htons(args.port);
    local_addr.sin6_addr = in6addr_any;  // Listen on all interfaces (IPv4 and IPv6)

    if (bind(sock, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        perror("bind");
        close(sock);
        filter_free(&filters);
        return 4;
    }

    if(args.verbose) printf("DNS proxy listening on port %d (IPv4 and IPv6)\n", args.port);

    uint8_t buf[BUF_SIZE];

    while (1) {
        struct sockaddr_storage client_addr;  // Can hold both IPv4 and IPv6
        socklen_t client_len = sizeof(client_addr);

        ssize_t r = recvfrom(sock, buf, sizeof(buf), 0,
                             (struct sockaddr*)&client_addr, &client_len);
        if (r < 0) {
            perror("recvfrom");
            continue;
        }

        // Log client info if verbose
        if (args.verbose) {
            char client_ip[INET6_ADDRSTRLEN];
            void *addr_ptr;
            const char *addr_family;
            
            if (client_addr.ss_family == AF_INET) {
                struct sockaddr_in *s = (struct sockaddr_in *)&client_addr;
                addr_ptr = &s->sin_addr;
                addr_family = "IPv4";
            } else {
                struct sockaddr_in6 *s = (struct sockaddr_in6 *)&client_addr;
                addr_ptr = &s->sin6_addr;
                addr_family = "IPv6";
            }
            
            inet_ntop(client_addr.ss_family, addr_ptr, client_ip, sizeof(client_ip));
            fprintf(stderr, "Query from %s client: %s\n", addr_family, client_ip);
        }

        DnsQuestion q;
        if (!dns_parse_question(buf, r, &q)) {
            if(args.verbose) fprintf(stderr, "Malformed DNS query received\n");
            continue;
        }

        uint8_t response[BUF_SIZE];
        int response_len;

        // Only handle type A queries
        if (q.qtype != DNS_TYPE_A) {
            if(args.verbose) fprintf(stderr, "Non-A query received: %s\n", q.qname);
            dns_build_error_response(buf, r, response, &response_len, 4); // NOTIMP
            sendto(sock, response, response_len, 0,
                   (struct sockaddr*)&client_addr, client_len);
            continue;
        }

        // Check filter
        if (filter_is_blocked(&filters, q.qname)) {
            if(args.verbose) fprintf(stderr, "Blocked domain: %s\n", q.qname);
            dns_build_error_response(buf, r, response, &response_len, 3); // NXDOMAIN
            sendto(sock, response, response_len, 0,
                   (struct sockaddr*)&client_addr, client_len);
            continue;
        }

        // Forward query to upstream resolver
        if (!resolver_forward_query(args.server,
                                    buf, r, response, &response_len,
                                    DEFAULT_TIMEOUT)) {
            if(args.verbose) fprintf(stderr, "Failed to query upstream resolver for %s\n", q.qname);
            dns_build_error_response(buf, r, response, &response_len, 2); // SERVFAIL
            sendto(sock, response, response_len, 0,
                   (struct sockaddr*)&client_addr, client_len);
            continue;
        }

        if (args.verbose) {
            printf("TXID REQ=%02X%02X RESP=%02X%02X\n",
            buf[0], buf[1], response[0], response[1]);
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