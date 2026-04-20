#include <stdio.h>
#include "utils.h"

int main() {
    printf("=========================================\n");
    printf("DEMONSTRACAO - CODIFICACAO ARITMETICA\n");
    printf("Equipe: Paulo Vinicius & Pedro Lucas\n");
    printf("=========================================\n");

    // Aqui você adiciona ou remove testes facilmente
    run_test("Caso 1 - Redundancia", "AAAAABBB");
    run_test("Caso 2 - Texto Curto", "Sistemas Embarcados");
    run_test("Caso 3 - Simbolos", "123!@#ABC");

    printf("\n=========================================\n");
    return 0;
}
