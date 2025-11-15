#!/bin/bash

PROXY_HOST=127.0.0.1
PROXY_PORT=5353
FILTER_FILE="filters.txt"
UPSTREAM_DNS=8.8.8.8

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# Function to check RCODE
check_rcode() {
    local domain=$1
    local type=$2
    local expected_rcode=$3
    local description=$4

    # Remove +short and +ttlid, keep +stats
    actual_rcode=$(dig @$PROXY_HOST -p $PROXY_PORT $domain $type +stats | grep "status:" | awk '{print $6}' | tr -d ',')
    if [ "$actual_rcode" = "$expected_rcode" ]; then
        echo -e "${GREEN}[PASS]${NC} $description ($domain $type)"
    else
        echo -e "${RED}[FAIL]${NC} $description ($domain $type) - got RCODE: $actual_rcode"
    fi
}

echo "Starting DNS proxy tests..."

# Make sure the proxy is running
if ! nc -z -u $PROXY_HOST $PROXY_PORT; then
    echo -e "${RED}Error:${NC} DNS proxy is not running on $PROXY_HOST:$PROXY_PORT"
    exit 1
fi

# 1. Test blocked domain
check_rcode "blocked.com" "A" "NXDOMAIN" "Blocked domain"

# 2. Test allowed domain
check_rcode "google.com" "A" "NOERROR" "Allowed domain"

# 3. Test non-A query
check_rcode "google.com" "AAAA" "NOTIMP" "Non-A query"

# 4. Test subdomain of blocked
check_rcode "sub.blocked.com" "A" "NXDOMAIN" "Subdomain of blocked domain"

# 5. Test malformed query (manual visual check)
echo "Testing malformed query..."
echo "garbage" | nc -u -w1 $PROXY_HOST $PROXY_PORT
echo "Sent garbage packet, proxy should not crash"

# 6. Test upstream failure (manual setup)
# Use unreachable IP, e.g., 192.0.2.1, temporarily restart proxy with that upstream

echo "All automatic tests completed."
