#ifndef DNS_H
#define DNS_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief DNS record type for IPv4 address queries (A record).
 */
#define DNS_TYPE_A 1

/**
 * @brief DNS class: Internet (IN).
 */
#define DNS_CLASS_IN 1

/**
 * @struct DnsQuestion
 * @brief Represents a parsed DNS question section.
 *
 * Members:
 *  - qname:  Fully reconstructed domain name (null-terminated, max 255 chars).
 *  - qtype:  DNS record type (e.g., 1 = A).
 *  - qclass: DNS class (e.g., 1 = IN).
 *
 * The DNS parser intentionally does not support compressed domain names
 * in the question section since these are typically not used in queries.
 */
typedef struct {
    char qname[256];     /**< Extracted domain name as a C string */
    uint16_t qtype;      /**< DNS query type (usually DNS_TYPE_A = 1) */
    uint16_t qclass;     /**< DNS class (usually DNS_CLASS_IN = 1) */
} DnsQuestion;

/**
 * @brief Parse the question section of a DNS query packet.
 *
 * This function extracts a single domain name (QNAME), QTYPE, and QCLASS
 * from a DNS UDP query. It validates that the domain uses the standard
 * label format and does not contain DNS compression pointers.
 *
 * Limitations:
 *  - Only one question is parsed.
 *
 * @param packet      Raw DNS packet buffer.
 * @param packet_len  Length of the packet in bytes.
 * @param out         Pointer to structure that will receive the parsed data.
 *
 * @return true on success, false if the packet is malformed or unsupported.
 */
bool dns_parse_question(const uint8_t *packet, int packet_len, DnsQuestion *out);

/**
 * @brief Construct a DNS response packet containing an error (no answers).
 *
 * This function preserves the Transaction ID (TXID) and question section,
 * converts the query into a response (QR=1), and sets the given RCODE.
 * It produces a minimal valid error message such as NXDOMAIN or NOTIMP.
 *
 * Common DNS error RCODE values:
 *  - 1 = Format error
 *  - 2 = Server failure
 *  - 3 = NXDOMAIN
 *  - 4 = Not implemented
 *  - 5 = Refused
 *
 * @param request       Original DNS request (used to copy TXID and QNAME).
 * @param request_len   Length of the request in bytes.
 * @param response      Buffer to write the generated DNS response.
 * @param response_len  Output: length of the generated response.
 * @param rcode         DNS response code (lower 4 bits of Flags2).
 *
 * @return true on success, false if the request header is invalid.
 */
bool dns_build_error_response(const uint8_t *request, int request_len,
                              uint8_t *response, int *response_len,
                              uint8_t rcode);

#endif // DNS_H
