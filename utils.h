#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include "arithmetic.h"

/**
 * @brief Calcula frequências e prepara a tabela cumulativa baseada num input.
 */
void prepare_test(const uint8_t *input, int length, float *freq);

/**
 * @brief Executa um ciclo completo de Encode/Decode e valida os dados.
 */
void run_test(const char *label, const char *test_string);

#endif // UTILS_H
