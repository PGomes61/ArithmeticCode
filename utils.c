#include "utils.h"
#include "arithmetic.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

void prepare_test(const uint8_t *input, int length, float *freq) {
    for (int i = 0; i < SYMBOL_COUNT; i++) {
        freq[i] = 0.0f;
    }

    for (int i = 0; i < length; i++) {
        freq[input[i]] += 1.0f;
    }

    for (int i = 0; i < SYMBOL_COUNT; i++) {
        freq[i] /= (float)length;
    }
}

void run_test(const char *label, const char *test_string) {
    int len = strlen(test_string);
    // Uso do buffer estático de tamanho ajustado
    uint8_t input[MAX_BUFFER];
    uint8_t decoded[MAX_BUFFER];
    float freq[SYMBOL_COUNT];

    memcpy(input, test_string, len);

    printf("\n--- TESTE: %s ---\n", label);
    printf("Original: \"%s\" (%d bytes)\n", test_string, len);

    prepare_test(input, len, freq);
    build_cumulative_table(freq);

    double encoded_val = encode(input, len);
    printf("Valor Codificado: %.15f\n", encoded_val);

    memset(decoded, 0, MAX_BUFFER);
    decode(encoded_val, decoded, len);
    decoded[len] = '\0';

    printf("Decodificado: \"%s\"\n", (char*)decoded);

    if (memcmp(input, decoded, len) == 0) {
        printf("Status: [ OK ] - Sucesso!\n");
    } else {
        // Mensagem ajustada para refletir a análise de precisão do hardware
        printf("Status: [ ERRO ] - Falha na integridade (Precision Underflow)!\n");
    }
}
