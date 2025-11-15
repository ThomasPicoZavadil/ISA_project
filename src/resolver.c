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
                            int timeout_sec) {
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(53);  // always send to DNS server port 53

    // resolve server
    struct hostent *he = gethostbyname(server);
    if (!he) {
        fprintf(stderr, "Failed to resolve server: %s\n", server);
        return false;
    }
    memcpy(&dest.sin_addr, he->h_addr_list[0], he->h_length);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return false; }

    struct timeval tv = { .tv_sec = timeout_sec, .tv_usec = 0 };
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt");
        close(sock);
        return false;
    }

    if (sendto(sock, query, query_len, 0,
               (struct sockaddr*)&dest, sizeof(dest)) != query_len) {
        perror("sendto");
        close(sock);
        return false;
    }

    socklen_t addrlen = sizeof(dest);
    ssize_t recvd = recvfrom(sock, response, DNS_MAX_PACKET_SIZE, 0,
                             (struct sockaddr*)&dest, &addrlen);
    if (recvd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            fprintf(stderr, "Timeout waiting for upstream response\n");
        else
            perror("recvfrom");
        close(sock);
        return false;
    }

    *response_len = recvd;
    close(sock);
    return true;
}


