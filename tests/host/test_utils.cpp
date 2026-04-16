/**
 * Host-side unit tests for NATSUtil (Array, Queue, RingBuffer, base64, creds parser)
 * Compile: g++ -std=c++17 -I../../include -I. -DHOST_TEST test_utils.cpp -lcjson -o test_utils
 */
// Include the actual library header (fake ESP-IDF headers in fake_esp_idf/)
#include "espidf_nats/util.h"

#include <cassert>
#include <cstdio>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("  TEST: %s ... ", name)
#define PASS() do { tests_passed++; printf("PASS\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("FAIL: %s\n", msg); } while(0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)

// ===== Array Tests =====

void test_array_basic() {
    TEST("Array basic push_back and access");
    NATSUtil::Array<int> arr(4);
    size_t idx = arr.push_back(42);
    ASSERT(idx == 0, "first index should be 0");
    ASSERT(arr.size() == 1, "size should be 1");
    ASSERT(arr[0] == 42, "value should be 42");
    PASS();
}

void test_array_growth() {
    TEST("Array auto-growth");
    NATSUtil::Array<int> arr(2);
    for (int i = 0; i < 100; i++) {
        size_t idx = arr.push_back(i);
        ASSERT(idx != SIZE_MAX, "push_back should not fail");
    }
    ASSERT(arr.size() == 100, "size should be 100");
    ASSERT(arr[0] == 0, "first value");
    ASSERT(arr[99] == 99, "last value");
    PASS();
}

void test_array_const_bounds_check() {
    TEST("Array const operator[] bounds check");
    NATSUtil::Array<int> arr(4);
    arr.push_back(10);
    arr.push_back(20);
    const NATSUtil::Array<int>& carr = arr;
    ASSERT(carr[0] == 10, "index 0");
    ASSERT(carr[1] == 20, "index 1");
    // Out of bounds should return default (0 for int)
    int val = carr[999];
    ASSERT(val == 0, "OOB should return default");
    PASS();
}

void test_array_erase() {
    TEST("Array erase");
    NATSUtil::Array<int> arr(4);
    arr.push_back(1);
    arr.push_back(2);
    arr.push_back(3);
    arr.erase(1);
    ASSERT(arr.size() == 2, "size after erase");
    ASSERT(arr[0] == 1, "first element");
    ASSERT(arr[1] == 3, "shifted element");
    PASS();
}

void test_array_clear_and_empty() {
    TEST("Array clear() and empty()");
    NATSUtil::Array<int> arr(4);
    arr.push_back(1);
    arr.push_back(2);
    ASSERT(!arr.empty(), "should not be empty");
    arr.clear();
    ASSERT(arr.empty(), "should be empty after clear");
    ASSERT(arr.size() == 0, "size should be 0");
    // Should be usable again
    arr.push_back(99);
    ASSERT(arr.size() == 1, "size after re-push");
    ASSERT(arr[0] == 99, "value after re-push");
    PASS();
}

// ===== Queue Tests =====

void test_queue_basic() {
    TEST("Queue basic push/pop");
    NATSUtil::Queue<int> q;
    ASSERT(q.empty(), "new queue should be empty");
    q.push(1);
    q.push(2);
    q.push(3);
    ASSERT(q.size() == 3, "size should be 3");
    ASSERT(!q.empty(), "should not be empty");
    ASSERT(q.peek() == 1, "peek should be 1");

    int val;
    ASSERT(q.pop(val) == true, "pop should succeed");
    ASSERT(val == 1, "first pop should be 1");
    ASSERT(q.pop(val) == true, "pop should succeed");
    ASSERT(val == 2, "second pop should be 2");
    ASSERT(q.pop(val) == true, "pop should succeed");
    ASSERT(val == 3, "third pop should be 3");
    ASSERT(q.empty(), "should be empty now");
    PASS();
}

void test_queue_safe_pop_empty() {
    TEST("Queue safe pop on empty");
    NATSUtil::Queue<size_t> q;
    size_t val = 999;
    ASSERT(q.pop(val) == false, "pop on empty should return false");
    ASSERT(val == 999, "val should be unchanged on failed pop");
    PASS();
}

void test_queue_legacy_pop() {
    TEST("Queue legacy pop");
    NATSUtil::Queue<int> q;
    q.push(42);
    int val = q.pop();
    ASSERT(val == 42, "legacy pop should return value");
    int empty_val = q.pop();
    ASSERT(empty_val == 0, "legacy pop on empty returns T()");
    PASS();
}

// ===== Millis Timer Tests =====

void test_millis() {
    TEST("millis() returns non-zero");
    unsigned long ms = NATSUtil::millis();
    ASSERT(ms > 0, "millis should be > 0");
    PASS();
}

// ===== Random Tests =====

void test_random() {
    TEST("random() range check");
    for (int i = 0; i < 1000; i++) {
        int r = NATSUtil::random(10);
        ASSERT(r >= 0 && r < 10, "random should be in [0,10)");
    }
    ASSERT(NATSUtil::random(0) == 0, "random(0) should be 0");
    ASSERT(NATSUtil::random(1) == 0, "random(1) should be 0");
    PASS();
}

// ===== Secure Memory Tests =====

void test_secure_zero() {
    TEST("secure_zero");
    char buf[16] = "hello world";
    NATSUtil::secure_zero(buf, sizeof(buf));
    for (size_t i = 0; i < sizeof(buf); i++) {
        ASSERT(buf[i] == 0, "all bytes should be zero");
    }
    PASS();
}

void test_secure_free() {
    TEST("secure_free (no crash)");
    char* str = strdup("secret password");
    NATSUtil::secure_free(str);
    NATSUtil::secure_free(NULL);  // Should not crash
    PASS();
}

// ===== Base64 Tests =====

void test_base64_encode() {
    TEST("base64_encode");
    uint8_t data[] = {0x48, 0x65, 0x6c, 0x6c, 0x6f};  // "Hello"
    char out[32];
    int ret = NATSUtil::base64_encode(data, 5, out, sizeof(out));
    ASSERT(ret > 0, "should return positive length");
    ASSERT(strcmp(out, "SGVsbG8=") == 0, "Hello -> SGVsbG8=");
    PASS();
}

void test_base64_encode_empty() {
    TEST("base64_encode empty");
    char out[8];
    int ret = NATSUtil::base64_encode(NULL, 0, out, sizeof(out));
    // Empty input should produce empty output
    ASSERT(ret == 0, "empty input");
    PASS();
}

void test_base64_encode_buffer_too_small() {
    TEST("base64_encode buffer too small");
    uint8_t data[] = {1, 2, 3, 4, 5, 6, 7, 8};
    char out[4];  // Too small for 8 bytes of input
    int ret = NATSUtil::base64_encode(data, 8, out, sizeof(out));
    ASSERT(ret == -1, "should return -1 for too-small buffer");
    PASS();
}

// ===== Creds Parser Tests =====

void test_parse_creds_file() {
    TEST("parse_creds_file");
    const char* creds =
        "-----BEGIN NATS USER JWT-----\n"
        "eyJhbGciOiJlZDI1NTE5LW5rZXkiLCJ0eXAiOiJKV1QifQ.test.signature\n"
        "------END NATS USER JWT------\n"
        "\n"
        "************************* IMPORTANT *************************\n"
        "NKEY Seed printed below can be used to sign and prove identity.\n"
        "\n"
        "-----BEGIN USER NKEY SEED-----\n"
        "SUACSSL3UAHUDXKFSNVUZRF5UHPMWZ6BFDTJ7M6USDXIEDNPPQYYYCU3VY\n"
        "------END USER NKEY SEED------\n";

    char* jwt = NULL;
    char* seed = NULL;
    bool ok = NATSUtil::parse_creds_file(creds, &jwt, &seed);
    ASSERT(ok, "parse should succeed");
    ASSERT(jwt != NULL, "jwt should not be NULL");
    ASSERT(seed != NULL, "seed should not be NULL");
    ASSERT(strstr(jwt, "eyJhbGci") != NULL, "jwt should start with eyJ");
    ASSERT(strstr(seed, "SUACSSL3") != NULL, "seed should start with SUA");
    free(jwt);
    NATSUtil::secure_free(seed);
    PASS();
}

void test_parse_creds_file_missing_seed() {
    TEST("parse_creds_file missing seed");
    const char* creds =
        "-----BEGIN NATS USER JWT-----\n"
        "eyJhbGciOiJ0ZXN0In0.payload.sig\n"
        "------END NATS USER JWT------\n";

    char* jwt = NULL;
    char* seed = NULL;
    bool ok = NATSUtil::parse_creds_file(creds, &jwt, &seed);
    ASSERT(!ok, "should fail without seed");
    ASSERT(jwt == NULL, "jwt should be cleaned up");
    ASSERT(seed == NULL, "seed should be NULL");
    PASS();
}

void test_parse_creds_file_null() {
    TEST("parse_creds_file NULL input");
    char* jwt = NULL;
    char* seed = NULL;
    ASSERT(!NATSUtil::parse_creds_file(NULL, &jwt, &seed), "NULL input");
    ASSERT(!NATSUtil::parse_creds_file("", &jwt, &seed), "empty input");
    PASS();
}

// ===== RingBuffer Tests =====

void test_ringbuffer_basic() {
    TEST("RingBuffer basic write/read");
    NATSUtil::RingBuffer rb(64);
    uint8_t write_data[] = "Hello NATS";
    size_t written = rb.write(write_data, 10);
    ASSERT(written == 10, "should write 10 bytes");
    ASSERT(rb.available() == 10, "10 bytes available");
    ASSERT(!rb.empty(), "not empty");

    uint8_t read_data[16] = {};
    size_t read_count = rb.read(read_data, 10);
    ASSERT(read_count == 10, "should read 10 bytes");
    ASSERT(memcmp(read_data, "Hello NATS", 10) == 0, "data should match");
    ASSERT(rb.empty(), "empty after full read");
    PASS();
}

void test_ringbuffer_overflow() {
    TEST("RingBuffer overflow drops oldest");
    NATSUtil::RingBuffer rb(8);  // Small buffer
    uint8_t data[16];
    memset(data, 'A', sizeof(data));
    // Write more than capacity
    size_t written = rb.write(data, 16);
    ASSERT(written == 16, "all bytes accepted (oldest dropped)");
    // Available should be capacity - 1 (ring buffer wastes one slot)
    ASSERT(rb.available() <= 8, "available <= capacity");
    PASS();
}

void test_ringbuffer_clear() {
    TEST("RingBuffer clear");
    NATSUtil::RingBuffer rb(32);
    uint8_t data[] = "test";
    rb.write(data, 4);
    ASSERT(!rb.empty(), "not empty");
    rb.clear();
    ASSERT(rb.empty(), "empty after clear");
    PASS();
}

// ===== Main =====

int main() {
    printf("=== NATSUtil Host Tests ===\n\n");

    printf("--- Array ---\n");
    test_array_basic();
    test_array_growth();
    test_array_const_bounds_check();
    test_array_erase();
    test_array_clear_and_empty();

    printf("\n--- Queue ---\n");
    test_queue_basic();
    test_queue_safe_pop_empty();
    test_queue_legacy_pop();

    printf("\n--- Millis/Random ---\n");
    test_millis();
    test_random();

    printf("\n--- Secure Memory ---\n");
    test_secure_zero();
    test_secure_free();

    printf("\n--- Base64 ---\n");
    test_base64_encode();
    test_base64_encode_empty();
    test_base64_encode_buffer_too_small();

    printf("\n--- Creds Parser ---\n");
    test_parse_creds_file();
    test_parse_creds_file_missing_seed();
    test_parse_creds_file_null();

    printf("\n--- RingBuffer ---\n");
    test_ringbuffer_basic();
    test_ringbuffer_overflow();
    test_ringbuffer_clear();

    printf("\n=== Results: %d passed, %d failed ===\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
