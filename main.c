#include <stdio.h>
#include "utils.h"
#include "tests.h"

int main(void) {
    printf("=========================================\n");
    printf("SISTEMA DE COMPRESSAO ARITMETICA - T1\n");
    printf("Equipe: Paulo Vinicius & Pedro Lucas\n");
    printf("=========================================\n");

    // 1. FASE DE CERTIFICACAO (Unity Framework)
    // Roda os testes unitários independentes
    run_unity_tests();

    printf("\n--- DEMONSTRACAO DE USO REAL (Round-Trip) ---\n");

    // 2. FASE DE DEMONSTRACAO
    // Mostra o algoritmo funcionando com strings reais
    run_test("Caso 1 - Redundancia", "AAAAABBB");
    run_test("Caso 2 - Frase Curta", "Sistemas Embarcados");

    printf("\n=========================================\n");

    return 0;
}
