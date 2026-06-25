#!/bin/bash
SERVER="http://localhost:8888"
PASSED=0
FAILED=0
LOG_FILE="/tmp/server.log"
SERVER_PID=""

check() {
    local name="$1"
    local expected="$2"
    local actual="$3"
    if [[ "$actual" == *"$expected"* ]]; then
        echo "✓ $name"
        ((PASSED++))
    else
        echo "✗ $name"
        echo "  expected: $expected"
        echo "  actual: $actual"
        ((FAILED++))
    fi
}

# Run a curl, then read only the new bytes appended to the log since before the request
check_log() {
    local name="$1"
    local expected="$2"
    shift 2
    local start_pos
    start_pos=$(stat -c%s "$LOG_FILE" 2>/dev/null || echo 0)
    "$@" > /dev/null
    sleep 0.2
    local result
    result=$(tail -c +$((start_pos + 1)) "$LOG_FILE" 2>/dev/null)
    check "$name" "$expected" "$result"
}

echo ""
echo "=== JSON Parser Tests ==="
echo ""

check_log "String in object"   "parsed message: hello" \
    curl -s --max-time 2 -X POST $SERVER/echo -d '{"message":"hello"}'

check_log "Number in object"   "parsed count: 42" \
    curl -s --max-time 2 -X POST $SERVER/echo -d '{"count":42}'

check_log "Boolean true"       "parsed active: true" \
    curl -s --max-time 2 -X POST $SERVER/echo -d '{"active":true}'

check_log "Boolean false"      "parsed active: false" \
    curl -s --max-time 2 -X POST $SERVER/echo -d '{"active":false}'

check_log "Float number"       "parsed ratio: 3.14" \
    curl -s --max-time 2 -X POST $SERVER/echo -d '{"ratio":3.14}'

check_log "Null value"         "parsed value: null" \
    curl -s --max-time 2 -X POST $SERVER/echo -d '{"value":null}'

check_log "Nested object"      "parsed user: (object)" \
    curl -s --max-time 2 -X POST $SERVER/echo -d '{"user":{"name":"alice"}}'

# Array test - two checks from one request
start_pos=$(stat -c%s "$LOG_FILE" 2>/dev/null || echo 0)
curl -s --max-time 2 -X POST $SERVER/echo -d '["first","second"]' > /dev/null
sleep 0.2
arr_result=$(tail -c +$((start_pos + 1)) "$LOG_FILE" 2>/dev/null)
check "Array[0]" "parsed array[0]: first"  "$arr_result"
check "Array[1]" "parsed array[1]: second" "$arr_result"

# Response body checks (no log needed)
result=$(curl -s --max-time 2 -X POST $SERVER/echo -d '{}')
check "Empty object" "{}" "$result"

result=$(curl -s --max-time 2 -X POST $SERVER/echo -d '[]')
check "Empty array" "[]" "$result"

echo ""
echo "=== Basic HTTP Tests ==="
echo ""

result=$(curl -s --max-time 2 $SERVER/ping)
check "GET /ping" "ok" "$result"

result=$(curl -s --max-time 2 $SERVER/)
check "GET /" "hello from arena server" "$result"

code=$(curl -s --max-time 2 -w "%{http_code}" -o /dev/null $SERVER/nonexistent)
check "404 Not Found" "404" "$code"

code=$(curl -s --max-time 2 -w "%{http_code}" -o /dev/null -X DELETE $SERVER/echo)
check "405 Method Not Allowed" "405" "$code"

echo ""
echo "=== Results ==="
echo "Passed: $PASSED"
echo "Failed: $FAILED"
echo ""

if [[ $FAILED -eq 0 ]]; then
    echo "All tests passed!"
    exit 0
else
    echo "Some tests failed!"
    exit 1
fi
