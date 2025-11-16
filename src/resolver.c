#define _POSIX_C_SOURCE 200112L
#include "resolver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>

bool resolver_forward_query(const char *server,
                            const uint8_t *query, int query_len,
                            uint8_t *response, int *response_len,
                            int timeout_sec)
{
    struct addrinfo hints, *result, *rp;
    int sock = -1;
    bool success = false;

    // Setup hints for getaddrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;        // IPv4
    hints.ai_socktype = SOCK_DGRAM;   // UDP
    hints.ai_protocol = IPPROTO_UDP;

    // Resolve server name/IP to address
    int ret = getaddrinfo(server, "53", &hints, &result);
    if (ret != 0) {
        fprintf(stderr, "getaddrinfo failed for %s: %s\n", 
                server, gai_strerror(ret));
        return false;
    }

    // Try each address until we successfully connect
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock < 0) {
            continue;
        }

        // Set receive timeout
        struct timeval tv = { .tv_sec = timeout_sec, .tv_usec = 0 };
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            close(sock);
            sock = -1;
            continue;
        }

        // Send query
        ssize_t sent = sendto(sock, query, query_len, 0, 
                             rp->ai_addr, rp->ai_addrlen);
        if (sent != query_len) {
            close(sock);
            sock = -1;
            continue;
        }

        // Receive response
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        ssize_t recvd = recvfrom(sock, response, DNS_MAX_PACKET_SIZE, 0,
                                (struct sockaddr*)&from_addr, &from_len);
        
        if (recvd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Timeout
                fprintf(stderr, "Timeout waiting for response from %s\n", server);
            }
            close(sock);
            sock = -1;
            continue;
        }

        // Success!
        *response_len = recvd;

        // Overwrite TXID to match our query (proxy behavior)
        response[0] = query[0];
        response[1] = query[1];

        success = true;
        break;
    }

    // Cleanup
    freeaddrinfo(result);
    if (sock >= 0) {
        close(sock);
    }

    return success;
}