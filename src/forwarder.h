#ifndef FORWARDER_H
#define FORWARDER_H

#include <stdint.h>
#include <stdbool.h>

#define DNS_MAX_PACKET_SIZE 512  // Standard UDP DNS packet limit

/**
 * @brief Forwards a DNS query to an upstream resolver and returns its response.
 *
 * Sends a UDP DNS packet to the specified upstream DNS server and waits up to
 * `timeout_sec` for a reply. If a response is received, the DNS transaction ID
 * (first two bytes) is overwritten to match the original query's TXID so that
 * callers can correctly match responses to requests.
 *
 * @param server        Upstream DNS server (IP or hostname)
 * @param query         DNS query packet
 * @param query_len     Length of query data
 * @param response      Output buffer for DNS response
 * @param response_len  Receives length of response data
 * @param timeout_sec   Maximum wait time for a DNS answer
 *
 * @return true on success, false on error or timeout.
 */
bool resolver_forward_query(const char *server,
                            const uint8_t *query, int query_len,
                            uint8_t *response, int *response_len,
                            int timeout_sec);

#endif
