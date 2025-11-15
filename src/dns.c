#include "dns.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define DNS_HEADER_SIZE 12
#define DNS_TYPE_A 1
#define DNS_CLASS_IN 1

static uint16_t read16(const uint8_t *buf) {
    return (buf[0] << 8) | buf[1];
}


// Parse a DNS question section from a UDP packet
bool dns_parse_question(const uint8_t *buf, int len, DnsQuestion *out) {
    if (len < DNS_HEADER_SIZE + 5) // header + at least 1 label + qtype/qclass
        return false;

    int pos = DNS_HEADER_SIZE;
    int qname_len = 0;

    while (pos < len) {
        uint8_t label_len = buf[pos];
        if (label_len == 0) {
            pos++; // end of name
            break;
        }

        if (label_len & 0xC0) {
            // Compression pointers not supported in query
            return false;
        }

        pos++;
        if (pos + label_len > len)
            return false; // name exceeds packet size

        if (qname_len + label_len + 1 >= sizeof(out->qname))
            return false; // qname buffer overflow

        memcpy(out->qname + qname_len, &buf[pos], label_len);
        qname_len += label_len;
        out->qname[qname_len++] = '.';
        pos += label_len;
    }

    if (qname_len == 0)
        return false;

    out->qname[qname_len - 1] = '\0'; // remove trailing dot

    // Ensure enough bytes remain for QTYPE and QCLASS
    if (pos + 4 > len)
        return false;

    out->qtype = (buf[pos] << 8) | buf[pos + 1];
    out->qclass = (buf[pos + 2] << 8) | buf[pos + 3];

    if (out->qclass != DNS_CLASS_IN)
        return false;

    return true;
}

bool dns_build_error_response(const uint8_t *request, int request_len,
                              uint8_t *response, int *response_len,
                              uint8_t rcode) {
    if (request_len < 12) return false;

    memcpy(response, request, 12);
    response[2] |= 0x80; // QR = 1
    response[3] = (response[3] & 0xF0) | (rcode & 0x0F);

    response[4] = 0; response[5] = 0; // QDCOUNT
    response[6] = 0; response[7] = 0; // ANCOUNT
    response[8] = 0; response[9] = 0; // NSCOUNT
    response[10] = 0; response[11] = 0; // ARCOUNT

    if (request_len > 12) {
        int qlen = request_len - 12;
        memcpy(response + 12, request + 12, qlen);
        response[4] = 0;
        response[5] = 1; // QDCOUNT = 1
        *response_len = 12 + qlen;
    } else {
        *response_len = 12;
    }

    return true;
}
