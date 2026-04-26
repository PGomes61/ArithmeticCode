#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include "arithmetic.h"

void prepare_counts(const uint8_t *input, int length, uint16_t counts[SYMBOL_BYTE_COUNT]);

/**
 * Comprime em memoria e confirma reversibilidade executando
 * scripts/external_arithmetic_decode.py --verify (decoder independente).
 * Executar a partir da raiz do repositorio. Variaveis de ambiente opcionais:
 * PYTHON (default python3), ARITH_DECODE_SCRIPT (default scripts/external_arithmetic_decode.py).
 * Retorna 0 em sucesso; -1 compressao invalida; -4 argumentos/tamanho; -5 input nulo;
 * -6 ficheiros temporarios; -7 falha do script Python (nao instalado, VERIFY falhou, etc.).
 */
int verify_reversible_with_python(const uint8_t *input, int length, size_t *packed_bytes_out);

void fill_demo_buffer_8k(uint8_t *buffer, int *length);

/**
 * Lê até MAX_BUFFER bytes do ficheiro, comprime, verifica com o decoder Python e imprime o
 * conteúdo original no stdout.
 * Retorna 0 em sucesso, -1 se não abrir o ficheiro, -2 se a verificação falhar.
 */
int run_from_file(const char *path);

/**
 * Lê o ficheiro de entrada, comprime, e grava o binário bruto (cabeçalho + corpo) no ficheiro de
 * saída. Útil para validar o formato com ferramentas externas (ex.: decoder Python).
 * Retorna 0 em sucesso; -1 I/O entrada; -2 compressão; -3 I/O saída.
 */
int export_compressed_binary(const char *input_path, const char *output_path);

#endif
