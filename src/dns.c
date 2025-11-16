#include "dns.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define DNS_HEADER_SIZE 12
#define DNS_CLASS_IN 1

bool dns_parse_question(const uint8_t *buf, int len, DnsQuestion *out) {
    if (len < DNS_HEADER_SIZE + 5) // header + at least 1 label + qtype/qclass
        return false;

    int pos = DNS_HEADER_SIZE;
    int qname_len = 0;

    while (pos < len) {
        uint8_t label_len = buf[pos++];
        if (label_len == 0) break; // end of qname

        if (label_len & 0xC0) return false; // compression not supported
        if (pos + label_len > len) return false; // out-of-bounds
        if (qname_len + label_len + 1 >= (int)sizeof(out->qname)) return false;

        memcpy(out->qname + qname_len, &buf[pos], label_len);
        qname_len += label_len;
        out->qname[qname_len++] = '.';
        pos += label_len;
    }

    if (qname_len == 0) return false;
    out->qname[qname_len - 1] = '\0';

    // QTYPE + QCLASS
    if (pos + 4 > len) return false;
    out->qtype  = (buf[pos] << 8) | buf[pos + 1];
    out->qclass = (buf[pos + 2] << 8) | buf[pos + 3];

    if (out->qclass != DNS_CLASS_IN) return false;

    return true;
}

bool dns_build_error_response(const uint8_t *request, int request_len,
                              uint8_t *response, int *response_len,
                              uint8_t rcode)
{
    if (request_len < 12) return false;

    // Copy header
    memcpy(response, request, DNS_HEADER_SIZE);

    // QR = 1, keep OPCODE and RD, set RCODE
    response[2] |= 0x80;               // QR=1
    response[3] = (response[3] & 0xF0) | (rcode & 0x0F);

    // QDCOUNT
    int qdcount = (request[4] << 8) | request[5];

    // Copy question section(s)
    int pos = DNS_HEADER_SIZE;
    for (int q = 0; q < qdcount; q++) {
        while (pos < request_len) {
            uint8_t len = request[pos++];
            if (len == 0) break;
            pos += len;
            if (pos >= request_len) return false;
        }
        pos += 4; // QTYPE + QCLASS
        if (pos > request_len) return false;
    }

    int qsize = pos - DNS_HEADER_SIZE;
    if (qsize > 0)
        memcpy(response + DNS_HEADER_SIZE, request + DNS_HEADER_SIZE, qsize);

    // Set ANCOUNT/NSCOUNT/ARCOUNT = 0
    response[6] = response[7] = 0;
    response[8] = response[9] = 0;
    response[10] = response[11] = 0;

    *response_len = DNS_HEADER_SIZE + qsize;

    return true;
}
