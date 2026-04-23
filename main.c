// #include <stdio.h>
// #include "utils.h"

// int main() {
//     printf("=========================================\n");
//     printf("DEMONSTRACAO - CODIFICACAO ARITMETICA\n");
//     printf("Equipe: Paulo Vinicius & Pedro Lucas\n");
//     printf("=========================================\n");

//     // Aqui você adiciona ou remove testes facilmente
//     run_test("Caso 1 - Redundancia", "AAAAABBB");
//     run_test("Caso 2 - Texto Curto", "Sistemas Embarcados");
//     run_test("Caso 3 - Simbolos", "123!@#ABC");

//     printf("\n=========================================\n");
//     return 0;
// }

#include <stdio.h>
#include "utils.h"
#include "tests.h" // Inclua o novo cabeçalho

int main() {
    printf("=========================================\n");
    printf("SISTEMA DE COMPRESSAO ARITMETICA - T1\n");
    printf("Equipe: Paulo Vinicius & Pedro Lucas\n");
    printf("=========================================\n");

    // 1. A PROVA DE FOGO (O que o Elias quer ver primeiro)
    run_certification_tests();

    printf("\n--- DEMONSTRACAO DE USO REAL ---\n");

    // 2. A DEMONSTRACAO (O que o algoritmo faz na prática)
    run_test("Caso 1 - Redundancia", "AAAAABBB");
    run_test("Caso 2 - Frase Curta", "Sistemas Embarcados");

    printf("\n=========================================\n");
    return 0;
}
