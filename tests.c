#include <stdio.h>
#include <math.h>
#include <string.h>
#include "arithmetic.h"
#include "utils.h"

#define MAX_TEST_STRING 32 // Define um teto para as strings de teste

typedef struct {
    uint8_t input[MAX_TEST_STRING]; // Array fixo em vez de ponteiro
    int length;                     // Tamanho real ocupado
    double expected_gold;           // Valor de referência
} GoldenTestCase;

// Tabela de Referência de Ouro
// Inicialização "Blindada":
GoldenTestCase golden_table[] = {
    {
        .input = "AAAAABBB",
        .length = 8,
        .expected_gold = 0.090338289737701
    },
    {
        .input = "BANANA",
        .length = 6,
        .expected_gold = 0.564814823921080
    }
};

void run_certification_tests() {
    printf("\n=== INICIANDO CERTIFICACAO DE REFERENCIA DE OURO ===\n");

    for (int i = 0; i < 2; i++) {
        float freq[256];
        prepare_test((uint8_t*)golden_table[i].input, strlen((const char*)golden_table[i].input), freq);
        build_cumulative_table(freq);

        double result = encode((uint8_t*)golden_table[i].input, strlen((const char*)golden_table[i].input));

        printf("Teste [%s]:\n", golden_table[i].input);
        printf("  Obtido:   %.15f\n", result);
        printf("  Esperado: %.15f\n", golden_table[i].expected_gold);

        if (fabs(result - golden_table[i].expected_gold) < 1e-12) {
            printf("  STATUS: [PASS] - Alinhado com a Referencia de Ouro.\n");
        } else {
            printf("  STATUS: [FAIL] - Divergencia matematica detectada!\n");
        }
    }
}
