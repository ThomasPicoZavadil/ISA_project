#ifndef DNS_H
#define DNS_H

#include <stdbool.h>
#include <stdint.h>

#define DNS_TYPE_A 1
#define DNS_CLASS_IN 1

typedef struct {
    char qname[256];   // extracted domain (null-terminated)
    uint16_t qtype;    // usually 1 = A
    uint16_t qclass;   // usually 1 = IN
} DnsQuestion;

// Parse DNS question
bool dns_parse_question(const uint8_t *packet, int packet_len, DnsQuestion *out);

// Build DNS error response (NXDOMAIN, NOTIMP, etc.)
bool dns_build_error_response(const uint8_t *request, int request_len,
                              uint8_t *response, int *response_len,
                              uint8_t rcode);

#endif
