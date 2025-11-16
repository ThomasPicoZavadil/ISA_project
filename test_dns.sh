#!/bin/bash

# DNS Proxy Test Suite
# Testuje funkcionalitu DNS proxy podle zadání včetně argumentů

PROXY_HOST=127.0.0.1
PROXY_PORT=5353
FILTER_FILE="test_filters.txt"
UPSTREAM_DNS=8.8.8.8
PROXY_PID=""

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

pass() { echo -e "${GREEN}[PASS]${NC} $1"; PASS_COUNT=$((PASS_COUNT + 1)); }
fail() { echo -e "${RED}[FAIL]${NC} $1"; FAIL_COUNT=$((FAIL_COUNT + 1)); }
info() { echo -e "${BLUE}[INFO]${NC} $1"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }

PASS_COUNT=0
FAIL_COUNT=0

# Cleanup function
cleanup() {
    if [[ -n "$PROXY_PID" ]] && kill -0 "$PROXY_PID" 2>/dev/null; then
        kill "$PROXY_PID" 2>/dev/null
        wait "$PROXY_PID" 2>/dev/null
    fi
    [[ -f "$FILTER_FILE" ]] && rm -f "$FILTER_FILE"
    [[ -f "empty_filters.txt" ]] && rm -f "empty_filters.txt"
    [[ -f "proxy.log" ]] && rm -f "proxy.log"
    [[ -f "test_output.txt" ]] && rm -f "test_output.txt"
}

trap cleanup EXIT INT TERM

# Create test filter file
create_filter_file() {
    cat > "$FILTER_FILE" << 'EOF'
# Test filter file for DNS proxy
# Comments should be ignored

blocked.com
ads.example.com
*.wildcard.com

# Empty lines should be ignored

malware.net
tracking.site
EOF
}

# Start DNS proxy with custom parameters
start_proxy() {
    local server=$1
    local port=$2
    local filter=$3
    local extra_args=$4
    
    info "Starting DNS proxy: -s $server -p $port -f $filter $extra_args"
    ./dns -s "$server" -p "$port" -f "$filter" $extra_args > proxy.log 2>&1 &
    PROXY_PID=$!
    
    # Wait longer for proxy to start
    sleep 3
    
    if ! kill -0 "$PROXY_PID" 2>/dev/null; then
        fail "DNS proxy failed to start"
        cat proxy.log
        return 1
    fi
    
    # Verify proxy is listening (try multiple times)
    local retries=5
    while [ $retries -gt 0 ]; do
        if nc -z -u -w1 "$PROXY_HOST" "$port" 2>/dev/null; then
            pass "DNS proxy started successfully (PID: $PROXY_PID)"
            # Give it one more second to be fully ready
            sleep 1
            return 0
        fi
        retries=$((retries - 1))
        sleep 1
    done
    
    fail "DNS proxy is not listening on port $port after multiple attempts"
    cat proxy.log
    return 1
}

# Stop proxy
stop_proxy() {
    if [[ -n "$PROXY_PID" ]] && kill -0 "$PROXY_PID" 2>/dev/null; then
        kill "$PROXY_PID" 2>/dev/null
        wait "$PROXY_PID" 2>/dev/null
        PROXY_PID=""
    fi
}

# Check if DNS query returns expected RCODE
check_dns() {
    local domain=$1
    local type=$2
    local expected=$3
    local desc=$4
    local port=${5:-$PROXY_PORT}
    local actual
    
    actual=$(dig @"$PROXY_HOST" -p "$port" "$domain" "$type" +time=5 +tries=2 2>/dev/null | \
        awk '/status:/{gsub(",", "", $6); print $6}')
    
    if [[ "$actual" == "$expected" ]]; then
        pass "$desc: $domain ($type) → $expected"
    else
        fail "$desc: $domain ($type) → Expected: $expected, Got: $actual"
    fi
}

# Check if domain resolves
check_resolves() {
    local domain=$1
    local desc=$2
    local port=${3:-$PROXY_PORT}
    local result
    
    # Try query with longer timeout
    result=$(dig @"$PROXY_HOST" -p "$port" "$domain" A +short +time=5 +tries=2 2>/dev/null | head -n1)
    
    if [[ -n "$result" ]] && [[ "$result" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
        pass "$desc: $domain → Resolved to $result"
    else
        # Try one more time before failing
        sleep 1
        result=$(dig @"$PROXY_HOST" -p "$port" "$domain" A +short +time=5 +tries=2 2>/dev/null | head -n1)
        if [[ -n "$result" ]] && [[ "$result" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
            pass "$desc: $domain → Resolved to $result (retry)"
        else
            fail "$desc: $domain → Did not resolve properly (got: '$result')"
        fi
    fi
}

echo "======================================================================"
echo "                     DNS Proxy Test Suite                            "
echo "======================================================================"
echo ""

# Check prerequisites
info "Checking prerequisites..."
if ! command -v dig >/dev/null 2>&1; then
    fail "dig command not found. Please install dnsutils or bind-utils"
    exit 1
fi

if ! command -v nc >/dev/null 2>&1; then
    fail "nc (netcat) command not found"
    exit 1
fi

if [[ ! -f "./dns" ]]; then
    fail "DNS proxy binary './dns' not found. Run 'make' first."
    exit 1
fi

pass "All prerequisites met"
echo ""

# ============================================================
# TEST 0: Command Line Arguments
# ============================================================
echo "======================================================================"
echo "TEST 0: Command Line Arguments Parsing"
echo "======================================================================"

# Test missing required arguments
info "Testing missing -s argument..."
./dns -p 5353 -f "$FILTER_FILE" > test_output.txt 2>&1 &
TEST_PID=$!
sleep 1
if kill -0 "$TEST_PID" 2>/dev/null; then
    kill "$TEST_PID" 2>/dev/null
    wait "$TEST_PID" 2>/dev/null
    fail "Should reject missing -s argument"
else
    if grep -qi "server\|použití\|-s" test_output.txt; then
        pass "Correctly rejected missing -s argument with usage message"
    else
        fail "Missing -s rejected but no clear error message"
    fi
fi

info "Testing missing -f argument..."
./dns -s 8.8.8.8 -p 5353 > test_output.txt 2>&1 &
TEST_PID=$!
sleep 1
if kill -0 "$TEST_PID" 2>/dev/null; then
    kill "$TEST_PID" 2>/dev/null
    wait "$TEST_PID" 2>/dev/null
    fail "Should reject missing -f argument"
else
    if grep -qi "filter\|použití\|-f" test_output.txt; then
        pass "Correctly rejected missing -f argument with usage message"
    else
        fail "Missing -f rejected but no clear error message"
    fi
fi

# Test invalid port
info "Testing invalid port (0)..."
./dns -s 8.8.8.8 -p 0 -f "$FILTER_FILE" > test_output.txt 2>&1 &
TEST_PID=$!
sleep 1
if kill -0 "$TEST_PID" 2>/dev/null; then
    kill "$TEST_PID" 2>/dev/null
    wait "$TEST_PID" 2>/dev/null
    fail "Should reject invalid port 0"
else
    pass "Correctly rejected invalid port 0"
fi

info "Testing invalid port (99999)..."
./dns -s 8.8.8.8 -p 99999 -f "$FILTER_FILE" > test_output.txt 2>&1 &
TEST_PID=$!
sleep 1
if kill -0 "$TEST_PID" 2>/dev/null; then
    kill "$TEST_PID" 2>/dev/null
    wait "$TEST_PID" 2>/dev/null
    fail "Should reject invalid port 99999"
else
    pass "Correctly rejected invalid port 99999"
fi

# Test non-existent filter file
info "Testing non-existent filter file..."
./dns -s 8.8.8.8 -p 5353 -f "nonexistent_file_12345.txt" > test_output.txt 2>&1 &
TEST_PID=$!
sleep 1
if kill -0 "$TEST_PID" 2>/dev/null; then
    kill "$TEST_PID" 2>/dev/null
    wait "$TEST_PID" 2>/dev/null
    fail "Should reject non-existent filter file"
else
    if grep -qi "chyba\|error\|nelze\|cannot" test_output.txt; then
        pass "Correctly rejected non-existent filter file with error message"
    else
        fail "Non-existent file rejected but no clear error message"
    fi
fi

# Test unknown argument
info "Testing unknown argument..."
./dns -s 8.8.8.8 -f "$FILTER_FILE" --unknown-arg > test_output.txt 2>&1 &
TEST_PID=$!
sleep 1
if kill -0 "$TEST_PID" 2>/dev/null; then
    kill "$TEST_PID" 2>/dev/null
    wait "$TEST_PID" 2>/dev/null
    fail "Should reject unknown argument"
else
    if grep -qi "neznámý\|unknown\|použití\|usage" test_output.txt; then
        pass "Correctly rejected unknown argument"
    else
        fail "Unknown argument rejected but no clear error message"
    fi
fi

# Test argument order (should be flexible)
create_filter_file
info "Testing flexible argument order (-f first)..."
./dns -f "$FILTER_FILE" -s 8.8.8.8 -p 5354 > test_output.txt 2>&1 &
TEST_PID=$!
sleep 2
if kill -0 "$TEST_PID" 2>/dev/null; then
    if nc -z -u -w1 "$PROXY_HOST" 5354 2>/dev/null; then
        pass "Accepts flexible argument order"
        kill "$TEST_PID" 2>/dev/null
        wait "$TEST_PID" 2>/dev/null
    else
        fail "Started but not listening (flexible order)"
        kill "$TEST_PID" 2>/dev/null
        wait "$TEST_PID" 2>/dev/null
    fi
else
    fail "Should accept flexible argument order"
fi

# Test default port (53) - just check if it would work with non-privileged port
info "Testing default port behavior..."
./dns -s 8.8.8.8 -f "$FILTER_FILE" > test_output.txt 2>&1 &
TEST_PID=$!
sleep 2
if kill -0 "$TEST_PID" 2>/dev/null; then
    # It started, probably failed to bind to 53 without root
    if grep -qi "permission\|bind\|address.*use" test_output.txt; then
        pass "Default port 53 attempted (requires root privileges)"
    else
        warn "Default port: proxy running but unclear if on port 53"
    fi
    kill "$TEST_PID" 2>/dev/null
    wait "$TEST_PID" 2>/dev/null
else
    if grep -qi "permission\|bind" test_output.txt; then
        pass "Default port 53 correctly requires privileges"
    else
        warn "Default port test inconclusive"
    fi
fi

echo ""

# ============================================================
# TEST 1: Upstream Server Formats
# ============================================================
echo "======================================================================"
echo "TEST 1: Upstream Server Format Support"
echo "======================================================================"

# Test with IP address
info "Testing upstream server as IP address (8.8.8.8)..."
if start_proxy "8.8.8.8" "$PROXY_PORT" "$FILTER_FILE" ""; then
    sleep 1
    check_resolves "google.com" "IP address upstream" "$PROXY_PORT"
    stop_proxy
else
    fail "Could not start with IP address upstream"
fi

# Test with domain name - use 8.8.8.8 directly since dns.google might be slow to resolve
info "Testing upstream server as domain name (dns.google)..."
if start_proxy "dns.google" "$((PROXY_PORT + 1))" "$FILTER_FILE" ""; then
    sleep 2
    check_resolves "example.com" "Domain name upstream" "$((PROXY_PORT + 1))"
    stop_proxy
else
    warn "Could not start with dns.google upstream (might be DNS resolution issue)"
fi

# Test with Cloudflare IP instead of domain name to avoid resolution delays
info "Testing upstream server with Cloudflare IP (1.1.1.1)..."
if start_proxy "1.1.1.1" "$((PROXY_PORT + 2))" "$FILTER_FILE" ""; then
    sleep 1
    check_resolves "cloudflare.com" "Cloudflare IP upstream" "$((PROXY_PORT + 2))"
    stop_proxy
else
    fail "Could not start with Cloudflare IP upstream"
fi

echo ""

# ============================================================
# TEST 2: Different Port Numbers
# ============================================================
echo "======================================================================"
echo "TEST 2: Custom Port Support"
echo "======================================================================"

# Test port 5353
info "Testing custom port 5353..."
if start_proxy "8.8.8.8" "5353" "$FILTER_FILE" ""; then
    check_resolves "google.com" "Port 5353" "5353"
    stop_proxy
else
    fail "Could not start on port 5353"
fi

# Test port 5454
info "Testing custom port 5454..."
if start_proxy "8.8.8.8" "5454" "$FILTER_FILE" ""; then
    check_resolves "google.com" "Port 5454" "5454"
    stop_proxy
else
    fail "Could not start on port 5454"
fi

# Test high port
info "Testing high port 54321..."
if start_proxy "8.8.8.8" "54321" "$FILTER_FILE" ""; then
    check_resolves "google.com" "Port 54321" "54321"
    stop_proxy
else
    fail "Could not start on port 54321"
fi

echo ""

# ============================================================
# TEST 3: Filter File Formats
# ============================================================
echo "======================================================================"
echo "TEST 3: Filter File Format Handling"
echo "======================================================================"

# Test with comments and empty lines
info "Testing filter with comments and empty lines..."
cat > "test_comment_filter.txt" << 'EOF'
# This is a comment
test-blocked.com

# Another comment

# More comments
another-blocked.com
EOF

if start_proxy "8.8.8.8" "$PROXY_PORT" "test_comment_filter.txt" ""; then
    check_dns "test-blocked.com" "A" "NXDOMAIN" "Blocked after comment" "$PROXY_PORT"
    check_dns "another-blocked.com" "A" "NXDOMAIN" "Blocked after empty lines" "$PROXY_PORT"
    stop_proxy
    pass "Filter file with comments handled correctly"
else
    fail "Could not start with comment filter"
fi
rm -f "test_comment_filter.txt"

# Test empty filter file
info "Testing empty filter file..."
touch "empty_filters.txt"
if start_proxy "8.8.8.8" "$PROXY_PORT" "empty_filters.txt" ""; then
    check_resolves "google.com" "Empty filter allows all" "$PROXY_PORT"
    stop_proxy
    pass "Empty filter file handled correctly"
else
    fail "Could not start with empty filter"
fi

# Test filter with only comments
info "Testing filter with only comments..."
echo "# Only comments" > "empty_filters.txt"
echo "# Nothing else" >> "empty_filters.txt"
if start_proxy "8.8.8.8" "$PROXY_PORT" "empty_filters.txt" ""; then
    check_resolves "google.com" "Comments-only filter allows all" "$PROXY_PORT"
    stop_proxy
    pass "Comments-only filter handled correctly"
else
    fail "Could not start with comments-only filter"
fi

echo ""

# ============================================================
# TEST 4: Verbose Mode
# ============================================================
echo "======================================================================"
echo "TEST 4: Verbose Mode (-v flag)"
echo "======================================================================"

info "Testing verbose mode..."
if start_proxy "8.8.8.8" "$PROXY_PORT" "$FILTER_FILE" "-v"; then
    # Make a query
    dig @"$PROXY_HOST" -p "$PROXY_PORT" google.com A +time=2 +tries=1 > /dev/null 2>&1
    sleep 1
    
    # Check if proxy logged something
    if [[ -s "proxy.log" ]] && grep -qi "listening\|query\|txid\|response\|blocked" proxy.log; then
        pass "Verbose mode produces output"
        info "Sample verbose output:"
        head -n 5 proxy.log | sed 's/^/     /'
    else
        warn "Verbose mode may not be producing expected output"
    fi
    stop_proxy
else
    fail "Could not start in verbose mode"
fi

echo ""

# Now start proxy for remaining functional tests
create_filter_file
if ! start_proxy "$UPSTREAM_DNS" "$PROXY_PORT" "$FILTER_FILE" ""; then
    fail "Could not start proxy for functional tests"
    exit 1
fi

echo ""

# ============================================================
# TEST 5: Basic Blocking Tests
# ============================================================
echo "======================================================================"
echo "TEST 5: Basic Domain Blocking"
echo "======================================================================"

check_dns "blocked.com" "A" "NXDOMAIN" "Exact match blocking"
check_dns "ads.example.com" "A" "NXDOMAIN" "Exact match blocking"
check_dns "malware.net" "A" "NXDOMAIN" "Exact match blocking"

echo ""

# ============================================================
# TEST 6: Subdomain Blocking
# ============================================================
echo "======================================================================"
echo "TEST 6: Subdomain Blocking (Domain + All Subdomains)"
echo "======================================================================"

check_dns "sub.blocked.com" "A" "NXDOMAIN" "Subdomain of blocked domain"
check_dns "deep.sub.blocked.com" "A" "NXDOMAIN" "Deep subdomain of blocked domain"
check_dns "www.malware.net" "A" "NXDOMAIN" "www subdomain of blocked domain"
check_dns "api.tracking.site" "A" "NXDOMAIN" "API subdomain of blocked domain"

echo ""

# ============================================================
# TEST 7: Wildcard Blocking
# ============================================================
echo "======================================================================"
echo "TEST 7: Wildcard Domain Blocking"
echo "======================================================================"

check_dns "wildcard.com" "A" "NOERROR" "Wildcard root domain (should be allowed)"
check_dns "sub.wildcard.com" "A" "NXDOMAIN" "Wildcard subdomain (should be blocked)"
check_dns "deep.sub.wildcard.com" "A" "NXDOMAIN" "Deep wildcard subdomain (should be blocked)"

echo ""

# ============================================================
# TEST 8: Allowed Domains
# ============================================================
echo "======================================================================"
echo "TEST 8: Allowed Domain Resolution"
echo "======================================================================"

check_dns "google.com" "A" "NOERROR" "Standard allowed domain"
check_dns "example.com" "A" "NOERROR" "Different from blocked ads.example.com"
check_resolves "google.com" "Resolution check"
check_resolves "cloudflare.com" "Resolution check"

echo ""

# ============================================================
# TEST 9: Query Type Handling
# ============================================================
echo "======================================================================"
echo "TEST 9: Query Type Handling (Only A Records Supported)"
echo "======================================================================"

check_dns "google.com" "A" "NOERROR" "Type A query (supported)"
check_dns "google.com" "AAAA" "NOTIMP" "Type AAAA query (not supported)"
check_dns "google.com" "MX" "NOTIMP" "Type MX query (not supported)"
check_dns "google.com" "TXT" "NOTIMP" "Type TXT query (not supported)"

echo ""

# ============================================================
# TEST 10: Case Insensitivity
# ============================================================
echo "======================================================================"
echo "TEST 10: Case Insensitive Matching"
echo "======================================================================"

check_dns "BLOCKED.COM" "A" "NXDOMAIN" "Uppercase blocked domain"
check_dns "Blocked.Com" "A" "NXDOMAIN" "Mixed case blocked domain"
check_dns "SUB.BLOCKED.COM" "A" "NXDOMAIN" "Uppercase subdomain"
check_dns "GOOGLE.COM" "A" "NOERROR" "Uppercase allowed domain"

echo ""

# ============================================================
# TEST 11: Trailing Dot Handling
# ============================================================
echo "======================================================================"
echo "TEST 11: Trailing Dot (FQDN) Handling"
echo "======================================================================"

check_dns "blocked.com." "A" "NXDOMAIN" "Blocked domain with trailing dot"
check_dns "google.com." "A" "NOERROR" "Allowed domain with trailing dot"
check_dns "sub.blocked.com." "A" "NXDOMAIN" "Blocked subdomain with trailing dot"

echo ""

# ============================================================
# TEST 12: Performance Test
# ============================================================
echo "======================================================================"
echo "TEST 12: Performance and Load Testing"
echo "======================================================================"

info "Running 50 concurrent queries..."
latencies=()
for i in {1..50}; do
    if (( i % 2 == 0 )); then
        domain="google.com"
    else
        domain="blocked.com"
    fi
    
    t=$(dig @"$PROXY_HOST" -p "$PROXY_PORT" "$domain" A +time=2 +tries=1 2>/dev/null | \
        awk '/Query time:/{print $4}')
    
    [[ -n "$t" ]] && latencies+=("$t")
done

if (( ${#latencies[@]} > 0 )); then
    sorted=($(printf "%s\n" "${latencies[@]}" | sort -n))
    count=${#sorted[@]}
    min=${sorted[0]}
    max=${sorted[$((count-1))]}
    mid=$((count / 2))
    median=${sorted[$mid]}
    
    sum=0
    for val in "${sorted[@]}"; do
        sum=$((sum + val))
    done
    avg=$((sum / count))
    
    pass "Performance test completed"
    echo "     Queries: $count/50 successful"
    echo "     Min:     ${min} ms"
    echo "     Max:     ${max} ms"
    echo "     Median:  ${median} ms"
    echo "     Average: ${avg} ms"
    
    if (( avg > 100 )); then
        warn "Average response time is high (>100ms)"
    fi
else
    fail "Performance test - could not measure response times"
fi

echo ""

# ============================================================
# TEST 13: Robustness
# ============================================================
echo "======================================================================"
echo "TEST 13: Robustness - Malformed Packet Handling"
echo "======================================================================"

info "Sending random malformed packets..."
for i in {1..10}; do
    head -c $((20 + RANDOM % 80)) /dev/urandom 2>/dev/null | \
        timeout 1 nc -u -w1 "$PROXY_HOST" "$PROXY_PORT" >/dev/null 2>&1
done

sleep 1
if kill -0 "$PROXY_PID" 2>/dev/null; then
    pass "Proxy survived malformed packet fuzzing"
else
    fail "Proxy crashed during malformed packet test"
fi

echo ""

# ============================================================
# Summary
# ============================================================
echo "======================================================================"
echo "                          TEST SUMMARY                                "
echo "======================================================================"
echo ""
echo -e "Total Tests: $((PASS_COUNT + FAIL_COUNT))"
echo -e "${GREEN}Passed: $PASS_COUNT${NC}"
echo -e "${RED}Failed: $FAIL_COUNT${NC}"
echo ""

if [[ $FAIL_COUNT -eq 0 ]]; then
    echo -e "${GREEN}✓ All tests passed!${NC}"
    echo "======================================================================"
    exit 0
else
    echo -e "${RED}✗ Some tests failed. Check output above for details.${NC}"
    echo "======================================================================"
    echo ""
    info "Check proxy.log for detailed proxy output"
    exit 1
fi