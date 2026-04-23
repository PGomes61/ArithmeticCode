#ifndef ARITHMETIC_CODING_H
#define ARITHMETIC_CODING_H

#include <stddef.h>
#include <stdint.h>

#define MAX_BUFFER_SIZE 4096
#define SYMBOL_COUNT 256

/**
 * Constroi cum_prob a partir de freq[0..255] (soma ≈ 1).
 * Usa precisão estendida internamente para reduzir erro numérico.
 */
void build_cumulative_table(const double *freq);

/**
 * Codifica length bytes num ponto no intervalo [0, 1).
 * Retorna extremo inferior do intervalo final (tag).
 */
long double encode(const uint8_t *input, size_t length);

/** Decodifica usando a mesma cum_prob já montada por build_cumulative_table. */
void decode(long double tag, uint8_t *output, size_t length);

#endif
