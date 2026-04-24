#include <stdio.h>
#include "utils.h"
#include "tests.h"

int main(void) {
    printf("=========================================\n");
    printf("SISTEMA DE COMPRESSAO ARITMETICA - T1\n");
    printf("Equipe: Paulo Vinicius & Pedro Lucas\n");
    printf("=========================================\n");

    // Validação matemática rigorosa
    run_unity_tests();

    printf("\n--- FASE 2: DEMONSTRACAO DE USO REAL (Round-Trip) ---\n");

    // 1. Caso Binário: Prova o uso de uint8_t
    printf("\n[Teste Binario - Dados nao-imprimiveis]");
    run_test("Dados Brutos", (char*)"\x01\x02\x03\x01");

    // 2. Caso de Sucesso Curto
    run_test("String Curta", "BANANA");

    // 3. Caso do Abismo de Precisão (Onde o 'Underflow' acontece)
    // Esta frase tem 25 caracteres. O double vai colapsar por volta do 16º.
    run_test("Abismo de Precisao", "Compressao Aritmetica eh Top");

    printf("\n=========================================\n");
    printf("O erro no ultimo caso ocorre devido\n");
    printf("ao limite de 52 bits da mantissa do tipo double.\n");
    printf("=========================================\n");

    return 0;
}
