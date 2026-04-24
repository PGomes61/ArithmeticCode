#ifndef ARITHMETIC_H
#define ARITHMETIC_H

#include <stdint.h>
#include <stddef.h>

/**
 * @file arithmetic.h
 * @author Paulo Vinicius Holanda Gomes & Pedro Lucas Coutinho de Araujo
 * @brief Definições e protótipos para Codificação Aritmética (T1 - Sistemas Embarcados)
 */

/* --- Definições de Tamanho --- */
/* 7164 (buffer) + 1028 (tabela cum_prob) = 8192 bytes = 8KB */
#define MAX_BUFFER      7164
#define SYMBOL_COUNT    2047   // Quantidade de símbolos (0-255)

/* --- Protótipos das Funções --- */

/**
 * @brief Transforma a tabela de frequências simples em uma tabela cumulativa.
 * @param freq Ponteiro para um array de 2048 floats contendo as frequências de cada símbolo.
 * A soma das frequências deve ser 1.0.
 */
void build_cumulative_table(const float *freq);

/**
 * @brief Codifica uma sequência de bytes em um único número real (double).
 * @param input Ponteiro para o buffer de dados originais.
 * @param length Quantidade de bytes a serem codificados.
 * @return double O "tag" ou valor real que representa a mensagem no intervalo [0, 1).
 */
double encode(const uint8_t *input, int length);

/**
 * @brief Decodifica um valor real de volta para o buffer de bytes original.
 * @param value O valor real codificado.
 * @param output Ponteiro para o buffer onde os dados reconstruídos serão salvos.
 * @param length O número esperado de bytes na mensagem original.
 */
void decode(double value, uint8_t *output, int length);

#endif // ARITHMETIC_H
