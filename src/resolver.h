#ifndef RESOLVER_H
#define RESOLVER_H

#include <stdint.h>
#include <stdbool.h>

#define DNS_MAX_PACKET_SIZE 512

/**
 * Sends DNS query to upstream server and receives response.
 * 
 * @param server      Upstream server IP or hostname
 * @param port        Upstream server port (default 53)
 * @param query       Pointer to DNS query packet
 * @param query_len   Length of query packet
 * @param response    Buffer to store response
 * @param response_len Pointer to store length of response received
 * @param timeout_sec Timeout in seconds for receiving response
 * @return true on success, false on error/timeout
 */
bool resolver_forward_query(const char *server,
                            const uint8_t *query, int query_len,
                            uint8_t *response, int *response_len,
                            int timeout_sec);

#endif
