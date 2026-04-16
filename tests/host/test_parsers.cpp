/**
 * Host-side unit tests for NATSParsers (typed response parsers)
 * Compile: g++ -std=c++17 -I../../include -I. -DHOST_TEST test_parsers.cpp -lcjson -o test_parsers
 */
// Include the actual library headers (fake ESP-IDF headers in fake_esp_idf/)
#include "espidf_nats/types.h"
#include "espidf_nats/parsers.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("  TEST: %s ... ", name)
#define PASS() do { tests_passed++; printf("PASS\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("FAIL: %s\n", msg); } while(0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)

// ===== parse_pub_ack Tests =====

void test_pub_ack_success() {
    TEST("parse_pub_ack success");
    const char* json = "{\"stream\":\"test-stream\",\"seq\":42,\"duplicate\":false}";
    const char* stream = NULL;
    uint64_t seq = 0;
    bool dup = true;
    const char* error = NULL;

    bool ok = NATSParsers::parse_pub_ack(json, strlen(json), &stream, &seq, &dup, &error);
    ASSERT(ok, "should succeed");
    ASSERT(stream != NULL, "stream not null");
    ASSERT(strcmp(stream, "test-stream") == 0, "stream name");
    ASSERT(seq == 42, "sequence number");
    ASSERT(dup == false, "not duplicate");

    free((void*)stream);
    PASS();
}

void test_pub_ack_duplicate() {
    TEST("parse_pub_ack duplicate");
    const char* json = "{\"stream\":\"mystream\",\"seq\":100,\"duplicate\":true}";
    const char* stream = NULL;
    uint64_t seq = 0;
    bool dup = false;
    const char* error = NULL;

    bool ok = NATSParsers::parse_pub_ack(json, strlen(json), &stream, &seq, &dup, &error);
    ASSERT(ok, "should succeed");
    ASSERT(seq == 100, "seq");
    ASSERT(dup == true, "should be duplicate");

    free((void*)stream);
    PASS();
}

void test_pub_ack_error() {
    TEST("parse_pub_ack error response");
    const char* json = "{\"error\":{\"code\":400,\"description\":\"bad request\"}}";
    const char* stream = NULL;
    uint64_t seq = 0;
    bool dup = false;
    const char* error = NULL;

    bool ok = NATSParsers::parse_pub_ack(json, strlen(json), &stream, &seq, &dup, &error);
    ASSERT(!ok, "should fail for error response");
    ASSERT(error != NULL, "error desc not null");
    ASSERT(strcmp(error, "bad request") == 0, "error message");

    free((void*)error);
    PASS();
}

void test_pub_ack_invalid_json() {
    TEST("parse_pub_ack invalid JSON");
    const char* json = "not json at all";
    bool ok = NATSParsers::parse_pub_ack(json, strlen(json), NULL, NULL, NULL, NULL);
    ASSERT(!ok, "should fail on invalid JSON");
    PASS();
}

void test_pub_ack_null_input() {
    TEST("parse_pub_ack NULL input");
    ASSERT(!NATSParsers::parse_pub_ack(NULL, 0, NULL, NULL, NULL, NULL), "NULL data");
    ASSERT(!NATSParsers::parse_pub_ack("", 0, NULL, NULL, NULL, NULL), "zero size");
    PASS();
}

// ===== parse_account_info Tests =====

void test_account_info() {
    TEST("parse_account_info");
    const char* json =
        "{"
        "\"memory\":1024,"
        "\"storage\":2048,"
        "\"streams\":5,"
        "\"consumers\":10,"
        "\"limits\":{"
            "\"max_memory\":1048576,"
            "\"max_storage\":10485760,"
            "\"max_streams\":100,"
            "\"max_consumers\":1000"
        "}"
        "}";

    jetstream_account_info_t info;
    bool ok = NATSParsers::parse_account_info(json, strlen(json), &info);
    ASSERT(ok, "should succeed");
    ASSERT(info.memory == 1024, "memory");
    ASSERT(info.storage == 2048, "storage");
    ASSERT(info.streams == 5, "streams");
    ASSERT(info.consumers == 10, "consumers");
    ASSERT(info.memory_max == 1048576, "memory_max");
    ASSERT(info.storage_max == 10485760, "storage_max");
    ASSERT(info.streams_max == 100, "streams_max");
    ASSERT(info.consumers_max == 1000, "consumers_max");
    PASS();
}

void test_account_info_error() {
    TEST("parse_account_info error response");
    const char* json = "{\"error\":{\"code\":503,\"description\":\"unavailable\"}}";
    jetstream_account_info_t info;
    bool ok = NATSParsers::parse_account_info(json, strlen(json), &info);
    ASSERT(!ok, "should fail on error");
    PASS();
}

// ===== parse_object_meta Tests =====

void test_object_meta() {
    TEST("parse_object_meta");
    const char* json =
        "{"
        "\"name\":\"firmware.bin\","
        "\"description\":\"OTA update v2.1\","
        "\"size\":131072,"
        "\"chunks\":2,"
        "\"digest\":\"sha256=abc123\","
        "\"mtime\":1700000000000000000"
        "}";

    object_meta_t meta;
    bool ok = NATSParsers::parse_object_meta(json, strlen(json), &meta);
    ASSERT(ok, "should succeed");
    ASSERT(meta.name != NULL && strcmp(meta.name, "firmware.bin") == 0, "name");
    ASSERT(meta.description != NULL && strcmp(meta.description, "OTA update v2.1") == 0, "description");
    ASSERT(meta.size == 131072, "size");
    ASSERT(meta.chunks == 2, "chunks");
    ASSERT(meta.digest != NULL && strcmp(meta.digest, "sha256=abc123") == 0, "digest");

    // Free strdup'd strings
    free((void*)meta.name);
    free((void*)meta.description);
    free((void*)meta.digest);
    PASS();
}

// ===== is_error_response Tests =====

void test_is_error_response_yes() {
    TEST("is_error_response - error");
    const char* json = "{\"error\":{\"code\":404,\"description\":\"stream not found\"}}";
    int code = 0;
    const char* desc = NULL;

    bool is_err = NATSParsers::is_error_response(json, strlen(json), &code, &desc);
    ASSERT(is_err, "should be error");
    ASSERT(code == 404, "error code");
    ASSERT(desc != NULL && strcmp(desc, "stream not found") == 0, "error desc");

    free((void*)desc);
    PASS();
}

void test_is_error_response_no() {
    TEST("is_error_response - success response");
    const char* json = "{\"stream\":\"test\",\"seq\":1}";
    int code = 999;
    const char* desc = (const char*)0xDEAD;

    bool is_err = NATSParsers::is_error_response(json, strlen(json), &code, &desc);
    ASSERT(!is_err, "should not be error");
    ASSERT(code == 0, "code should be zeroed");
    ASSERT(desc == NULL, "desc should be NULL");
    PASS();
}

void test_is_error_response_null() {
    TEST("is_error_response - NULL input");
    ASSERT(!NATSParsers::is_error_response(NULL, 0, NULL, NULL), "NULL");
    PASS();
}

// ===== Main =====

int main() {
    printf("=== NATSParsers Host Tests ===\n\n");

    printf("--- parse_pub_ack ---\n");
    test_pub_ack_success();
    test_pub_ack_duplicate();
    test_pub_ack_error();
    test_pub_ack_invalid_json();
    test_pub_ack_null_input();

    printf("\n--- parse_account_info ---\n");
    test_account_info();
    test_account_info_error();

    printf("\n--- parse_object_meta ---\n");
    test_object_meta();

    printf("\n--- is_error_response ---\n");
    test_is_error_response_yes();
    test_is_error_response_no();
    test_is_error_response_null();

    printf("\n=== Results: %d passed, %d failed ===\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
