#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>

#include "arithmetic.h"

/**
 * Frequências empíricas (contagem / length), renormaisadas para somar 1.
 * length == 0: modelo uniforme (1/256), usado só para manter chamadas válidas.
 */
void compute_frequencies(const uint8_t *input, size_t length, double *freq);

void run_test(const char *label, const char *test_string);

/** Qualquer sequência de bytes até MAX_BUFFER_SIZE (inclui '\\0' no meio). */
void run_test_bytes(const char *label, const uint8_t *data, size_t length);

#endif
