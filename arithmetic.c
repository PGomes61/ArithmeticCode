/**
 * Núcleo da codificação aritmética em ponto flutuante (demonstração).
 * Intervalo inicial [0, 1); a cada símbolo o intervalo atual é repartido
 * proporcionalmente às probabilidades cumulativas.
 */
#include "arithmetic.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>

static long double cum_prob[SYMBOL_COUNT + 1];

void build_cumulative_table(const double *freq) {
    cum_prob[0] = 0.0L;
    for (int i = 0; i < SYMBOL_COUNT; i++) {
        cum_prob[i + 1] = cum_prob[i] + (long double)freq[i];
    }
}

long double encode(const uint8_t *input, size_t length) {
    if (input == NULL || length == 0U) {
        return 0.0L;
    }

    long double low = 0.0L;
    long double high = 1.0L;

    for (size_t i = 0; i < length; i++) {
        uint8_t symbol = input[i];
        long double range = high - low;
        long double new_low = low + range * cum_prob[symbol];
        long double new_high = low + range * cum_prob[symbol + 1];
        low = new_low;
        high = new_high;
    }

    return low;
}

void decode(long double value, uint8_t *output, size_t length) {
    if (output == NULL || length == 0U) {
        return;
    }

    if (value >= 1.0L) {
        value = nextafterl(1.0L, 0.0L);
    }
    if (value < 0.0L) {
        value = 0.0L;
    }

    for (size_t i = 0; i < length; i++) {
        int found = 0;
        for (int s = 0; s < SYMBOL_COUNT; s++) {
            long double lo = cum_prob[s];
            long double hi = cum_prob[s + 1];
            if (value >= lo && value < hi) {
                output[i] = (uint8_t)s;
                long double range = hi - lo;
                if (range <= 0.0L) {
                    return;
                }
                value = (value - lo) / range;
                found = 1;
                break;
            }
        }
        if (!found) {
            return;
        }
    }
}
