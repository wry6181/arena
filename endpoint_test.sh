#!/bin/bash

SERVER="http://localhost:8888"
PASSED=0
FAILED=0
LOG_FILE="/tmp/server.log"
SKIP_SERVER_START=0

if [[ "$1" == "--no-start" ]]; then
    SKIP_SERVER_START=1
fi

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

echo ""
echo "=== JSON Parser Tests ==="
echo ""

# Test 1: String in object
> $LOG_FILE
curl -s --max-time 2 -X POST $SERVER/echo -d '{"message":"hello"}' > /dev/null
sleep 0.2
result=$(cat $LOG_FILE 2>/dev/null)
check "String in object" "parsed message: hello" "$result"

# Test 2: Number in object
> $LOG_FILE
curl -s --max-time 2 -X POST $SERVER/echo -d '{"count":42}' > /dev/null
sleep 0.2
result=$(cat $LOG_FILE 2>/dev/null)
check "Number in object" "parsed count: 42" "$result"

# Test 3: Boolean true
> $LOG_FILE
curl -s --max-time 2 -X POST $SERVER/echo -d '{"active":true}' > /dev/null
sleep 0.2
result=$(cat $LOG_FILE 2>/dev/null)
check "Boolean true" "parsed active: true" "$result"

# Test 4: Boolean false
> $LOG_FILE
curl -s --max-time 2 -X POST $SERVER/echo -d '{"active":false}' > /dev/null
sleep 0.2
result=$(cat $LOG_FILE 2>/dev/null)
check "Boolean false" "parsed active: false" "$result"

# Test 5: Float number
> $LOG_FILE
curl -s --max-time 2 -X POST $SERVER/echo -d '{"ratio":3.14}' > /dev/null
sleep 0.2
result=$(cat $LOG_FILE 2>/dev/null)
check "Float number" "parsed ratio: 3.14" "$result"

# Test 6: Null value
> $LOG_FILE
curl -s --max-time 2 -X POST $SERVER/echo -d '{"value":null}' > /dev/null
sleep 0.2
result=$(cat $LOG_FILE 2>/dev/null)
check "Null value" "parsed value: null" "$result"

# Test 7: Nested object
> $LOG_FILE
curl -s --max-time 2 -X POST $SERVER/echo -d '{"user":{"name":"alice"}}' > /dev/null
sleep 0.2
result=$(cat $LOG_FILE 2>/dev/null)
check "Nested object" "parsed user: (object)" "$result"

# Test 8: Array of strings
> $LOG_FILE
curl -s --max-time 2 -X POST $SERVER/echo -d '["first","second"]' > /dev/null
sleep 0.2
result=$(cat $LOG_FILE 2>/dev/null)
check "Array[0]" "parsed array[0]: first" "$result"
check "Array[1]" "parsed array[1]: second" "$result"

# Test 9: Empty object - just verify response
> $LOG_FILE
result=$(curl -s --max-time 2 -X POST $SERVER/echo -d '{}')
check "Empty object" "{}" "$result"

# Test 10: Empty array - just verify response
> $LOG_FILE
result=$(curl -s --max-time 2 -X POST $SERVER/echo -d '[]')
check "Empty array" "[]" "$result"

echo ""
echo "=== Basic HTTP Tests ==="
echo ""

# Test GET /ping
result=$(curl -s --max-time 2 $SERVER/ping)
check "GET /ping" "ok" "$result"

# Test GET /
result=$(curl -s --max-time 2 $SERVER/)
check "GET /" "hello from arena server" "$result"

# Test 404
code=$(curl -s --max-time 2 -w "%{http_code}" -o /dev/null $SERVER/nonexistent)
check "404 Not Found" "404" "$code"

# Test 405
code=$(curl -s --max-time 2 -w "%{http_code}" -o /dev/null -X DELETE $SERVER/echo)
check "405 Method Not Allowed" "405" "$code"

echo ""
echo "=== Results ==="
echo "Passed: $PASSED"
echo "Failed: $FAILED"
echo ""

stop_server

if [[ $FAILED -eq 0 ]]; then
    echo "All tests passed!"
    exit 0
else
    echo "Some tests failed!"
    exit 1
fi
