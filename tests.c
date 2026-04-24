#include "unity.h"
#include "arithmetic.h"
#include "utils.h"

// Função que roda ANTES de cada teste
void setUp(void) {}

// Função que roda DEPOIS de cada teste
void tearDown(void) {}

void test_GoldenReference_AAAAABBB(void) {
    uint8_t input[] = "AAAAABBB";
    float freq[256];
    prepare_test(input, 8, freq);
    build_cumulative_table(freq);

    double expected = 0.090338289737701;
    double result = encode(input, 8);

    // O Unity tem uma macro específica para comparar doubles com margem de erro (Epsilon)
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, expected, result);
}

void test_GoldenReference_BANANA(void) {
    uint8_t input[] = "BANANA";
    float freq[256];
    prepare_test(input, 6, freq);
    build_cumulative_table(freq);

    // Valor teórico calculado manualmente
    double expected = 0.564814814814815;
    double result = encode(input, 6);

    TEST_ASSERT_DOUBLE_WITHIN(1e-12, expected, result);
}

void run_unity_tests(void) {
    printf("\n--- FASE 1: CERTIFICACAO (Referencia de Ouro) ---\n");
    printf("Status esperado: [PASS]\n");
    printf("-------------------------------------------------\n");

    UNITY_BEGIN();
    RUN_TEST(test_GoldenReference_AAAAABBB);
    RUN_TEST(test_GoldenReference_BANANA);
    int status = UNITY_END(); // O UNITY_END retorna 0 se tudo passar

    printf("-------------------------------------------------\n");
    if (status == 0) {
        printf("RESULTADO: [ OK ] - Algoritmo validado com sucesso.\n");
    } else {
        printf("RESULTADO: [ ATENCAO ] - Divergencia de precisao detectada.\n");
    }
}
