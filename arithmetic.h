#ifndef ARITHMETIC_H
#define ARITHMETIC_H

#include <stddef.h>
#include <stdint.h>

/**
 * Codificação aritmética em ponto fixo 16 bits com renormalização WNC (bitstream).
 * Sem malloc e sem recursão.
 *
 * Limite da mensagem (MAX_BUFFER): podes aumentar se tiveres RAM para os buffers
 * estáticos (ver MAX_COMPRESSED_BYTES). O cabeçalho guarda n em 32 bits (teórico até ~4 GiB).
 * O modelo WNC exige soma das frequencias escaladas <= WNC_FREQUENCY_TOTAL_MAX; mensagens
 * maiores sao aceites porque as contagens sao escaladas automaticamente.
 *
 * Descompressao: implementacao de referencia em scripts/external_arithmetic_decode.py (fora deste
 * modulo C).
 */
#define WNC_FREQUENCY_TOTAL_MAX ((1u << 14) - 1u)

#ifndef MAX_BUFFER
#define MAX_BUFFER (1u << 20)
#endif

#define SYMBOL_BYTE_COUNT    256

#define ARITH_HEADER_BYTES   (sizeof(uint32_t) + SYMBOL_BYTE_COUNT * sizeof(uint16_t))

#define MAX_COMPRESSED_BYTES (ARITH_HEADER_BYTES + (MAX_BUFFER * 2u) + 1048576u)

typedef struct {
    uint32_t cum[SYMBOL_BYTE_COUNT + 1];
    uint32_t total;
} ArithmeticModel;

void arithmetic_count_raw(const uint8_t *data, int length, uint32_t raw_counts[SYMBOL_BYTE_COUNT]);

void arithmetic_scale_counts_for_wnc(const uint32_t *raw_counts, int message_length,
                                     uint16_t scaled_counts[SYMBOL_BYTE_COUNT]);

void arithmetic_count_symbols(const uint8_t *data, int length, uint16_t counts[SYMBOL_BYTE_COUNT]);

void arithmetic_model_from_counts(const uint16_t counts[SYMBOL_BYTE_COUNT], ArithmeticModel *model);

void arithmetic_build_model_from_data(const uint8_t *data, int length, ArithmeticModel *model);

size_t arithmetic_compress(const uint8_t *input, int length, const ArithmeticModel *model,
                           uint8_t *out, size_t out_cap);

#endif
