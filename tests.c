#include "unity.h"
#include "utils.h"
#include "arithmetic.h"
#include <stdint.h>
#include <string.h>

static uint8_t s_tests_decoded[MAX_BUFFER];
static uint8_t s_tests_large_input[MAX_BUFFER];

void setUp(void) {}

void tearDown(void) {}

static void assert_round_trip(const uint8_t *input, int length) {
    size_t packed = 0;
    int rc = codec_roundtrip(input, length, s_tests_decoded, sizeof(s_tests_decoded), &packed);
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_GREATER_THAN(0u, packed);
}

void test_round_trip_empty(void) {
    const uint8_t input[] = "";
    assert_round_trip(input, 0);
}

void test_round_trip_single_byte(void) {
    const uint8_t input[] = {0x42};
    assert_round_trip(input, 1);
}

void test_round_trip_sixteen_bytes(void) {
    const uint8_t input[] = "0123456789ABCDEF";
    assert_round_trip(input, 16);
}

void test_round_trip_long_phrase(void) {
    const uint8_t input[] = "Compressao Aritmetica eh Top";
    assert_round_trip(input, (int)strlen((const char *)input));
}

void test_round_trip_high_redundancy(void) {
    uint8_t input[200];
    memset(input, 'Z', sizeof(input));
    assert_round_trip(input, (int)sizeof(input));
}

void test_round_trip_binary_pattern(void) {
    uint8_t input[64];
    for (int i = 0; i < 64; i++) {
        input[i] = (uint8_t)(i * 17 + 3);
    }
    assert_round_trip(input, (int)sizeof(input));
}

void test_round_trip_one_kilobyte(void) {
    uint8_t input[1024];
    for (int i = 0; i < 1024; i++) {
        input[i] = (uint8_t)((i * 131u + 7u) & 0xFFu);
    }
    assert_round_trip(input, (int)sizeof(input));
}

void test_round_trip_scaled_frequencies(void) {
    uint8_t input[25000];
    memset(input, 'M', sizeof(input));
    assert_round_trip(input, (int)sizeof(input));
}

void test_round_trip_max_buffer(void) {
    int length = 0;
    fill_demo_buffer_8k(s_tests_large_input, &length);
    TEST_ASSERT_EQUAL_INT((int)MAX_BUFFER, length);

    size_t packed = 0;
    int rc =
        codec_roundtrip(s_tests_large_input, length, s_tests_decoded, sizeof(s_tests_decoded), &packed);
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_GREATER_THAN(ARITH_HEADER_BYTES, packed);
}

void run_unity_tests(void) {
    printf("\n--- TESTES Unity (round-trip) ---\n\n");
    UNITY_BEGIN();
    RUN_TEST(test_round_trip_empty);
    RUN_TEST(test_round_trip_single_byte);
    RUN_TEST(test_round_trip_sixteen_bytes);
    RUN_TEST(test_round_trip_long_phrase);
    RUN_TEST(test_round_trip_high_redundancy);
    RUN_TEST(test_round_trip_binary_pattern);
    RUN_TEST(test_round_trip_one_kilobyte);
    RUN_TEST(test_round_trip_scaled_frequencies);
    RUN_TEST(test_round_trip_max_buffer);
    int status = UNITY_END();

    if (status == 0) {
        printf("STATUS: [ OK ] - Todos os testes passaram.\n");
    } else {
        printf("STATUS: [ FALHA ] - Ver mensagens acima.\n");
    }
}
