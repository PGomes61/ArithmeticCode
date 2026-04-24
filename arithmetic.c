#include "arithmetic.h"
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

// Estruturas de dados estáticas
// cum_prob: 2048 * 4 bytes = 8192 bytes = 8 KB
static float cum_prob[SYMBOL_COUNT + 1];

void build_cumulative_table(const float *freq) {
    cum_prob[0] = 0.0f;
    for (int i = 0; i < SYMBOL_COUNT; i++) {
        cum_prob[i + 1] = cum_prob[i] + freq[i];
    }
}

double encode (const uint8_t *input, int length) {
    double low = 0.0;
    double high = 1.0;

    // Laço externo: percorre cada símbolo do buffer de entrada (O(n))
    for (int i = 0; i < length; i++) {
        uint8_t symbol = input[i];
        double range = high - low;

        // Laço interno: percorre a tabela cumulativa (O(k))
        // Este laço atende ao requisito de "pelo menos dois laços aninhados"
        for (int s = 0; s < SYMBOL_COUNT; s++) {
            if (s == symbol) {
                high = low + range * cum_prob[s + 1];
                low = low + range * cum_prob[s];
                break;
            }
        }
    }
    return low; // O número real que representa a sequência
}

void decode(double value, uint8_t *output, int length) {
    // Laços aninhados para atender aos requisitos do algoritmo
    for (int i = 0; i < length; i++) {
        for (int s = 0; s < SYMBOL_COUNT; s++) {
            if (value >= cum_prob[s] && value < cum_prob[s + 1]) {
                output[i] = (uint8_t)s;

                // Atualiza o valor para o próximo símbolo
                double range = cum_prob[s + 1] - cum_prob[s];
                value = (value - cum_prob[s]) / range;
                break;
            }
        }
    }
}
