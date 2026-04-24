#include "unity.h"
#include "arithmetic.h"
#include "utils.h"

// Função que roda ANTES de cada teste
void setUp(void) {}

// Função que roda DEPOIS de cada teste
void tearDown(void) {}

// CASO 1: Redundância Máxima
void test_GoldenReference_Redundancy(void) {
    uint8_t input[] = "AAAAAAAA";
    float freq[256] = {0};
    prepare_test(input, 8, freq);
    build_cumulative_table(freq);

    double expected = 0.0; // Em redundância total, o intervalo não se move do zero
    double result = encode(input, 8);

    TEST_ASSERT_EQUAL_DOUBLE(expected, result);
}

// CASO 2: Simetria Perfeita (50/50)
void test_GoldenReference_Symmetry(void) {
    uint8_t input[] = "ABAB";
    float freq[256] = {0};
    prepare_test(input, 4, freq);
    build_cumulative_table(freq);

    double expected = 0.3125; // Valor teórico exato para ABAB
    double result = encode(input, 4);

    TEST_ASSERT_DOUBLE_WITHIN(1e-12, expected, result);
}

// CASO 3: O Clássico (BANANA)
void test_GoldenReference_Banana(void) {
    uint8_t input[] = "BANANA";
    float freq[256] = {0};
    prepare_test(input, 6, freq);
    build_cumulative_table(freq);

    double expected = 0.564814814814815;
    double result = encode(input, 6);

    // Epsilon de 1e-8 para acomodar o arredondamento da FPU do seu ASUS
    TEST_ASSERT_DOUBLE_WITHIN(1e-8, expected, result);
}

void run_unity_tests(void) {
    printf("\n--- FASE 1: CERTIFICACAO (Referencia de Ouro) ---\n\n");
    UNITY_BEGIN();
    RUN_TEST(test_GoldenReference_Redundancy);
    RUN_TEST(test_GoldenReference_Symmetry);
    RUN_TEST(test_GoldenReference_Banana);
    int status = UNITY_END();

    if (status == 0) {
        printf("STATUS: [ OK ] - Motor matematico validado.\n");
    } else {
        printf("STATUS: [ ATENCAO ] - Divergencia de hardware detectada.\n");
    }
}
