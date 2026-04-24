#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include "arithmetic.h"

void prepare_counts(const uint8_t *input, int length, uint16_t counts[SYMBOL_BYTE_COUNT]);

/**
 * Unico caminho de codificacao/decodificacao: contagens -> modelo -> compress -> decompress.
 * Compara com memcmp. Retorna 0 em sucesso; -1 compressao invalida; -2 descompressao; -3 dados
 * diferentes; -4 argumentos invalidos; -5 ponteiros nulos.
 */
int codec_roundtrip(const uint8_t *input, int length, uint8_t *decoded_out, size_t decoded_cap,
                    size_t *packed_bytes_out);

void fill_demo_buffer_8k(uint8_t *buffer, int *length);

/**
 * Lê até MAX_BUFFER bytes do ficheiro, comprime, descomprime e imprime o resultado no stdout.
 * Retorna 0 em sucesso, -1 se não abrir o ficheiro, -2 se compressão/descompressão falhar.
 */
int run_from_file(const char *path);

#endif
