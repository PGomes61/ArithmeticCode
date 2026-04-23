#include <stdio.h>
#include <string.h>
#include "utils.h"

int main(void) {
    printf("=========================================\n");
    printf("Demonstração — codificação aritmética\n");
    printf("Equipe: Paulo Vinicius & Pedro Lucas\n");
    printf("=========================================\n");

    run_test("Caso 1 — Redundância", "AAAAABBB");
    /* Textos mais longos e diversos tendem a esgotar precisão em float (ver README-DOCUMENTACAO). */
    run_test("Caso 2 — Texto curto", "Sistemas");
    run_test("Caso 3 — Símbolos", "123!@#ABC");
    run_test("Caso 4 — String vazia", "");

    /* Sequência com byte zero no meio (strlen não serve; usa run_test_bytes) */
    const uint8_t com_null[] = {'O', 'K', 0, 'F', 'I', 'M'};
    run_test_bytes("Caso 5 — Bytes arbitrários (\\0 no meio)", com_null, sizeof(com_null));

    printf("\n=========================================\n");
    return 0;
}
