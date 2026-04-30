/**
 * @file arithmetic.h
 * @brief API pública: modelo estatístico e compressão aritmética WNC (16 bits, sem heap).
 *
 * @section app_visao Visão geral
 * Este módulo implementa codificação aritmética em ponto fixo com 16 bits e renormalização
 * do tipo Witten–Neal–Cleary (WNC), produzindo um bitstream. Não utiliza @c malloc nem
 * recursão. O formato do pacote comprimido inclui cabeçalho (comprimento da mensagem +
 * frequências escaladas por símbolo) seguido do corpo binário. A descompressão de referência
 * encontra-se em @c scripts/external_arithmetic_decode.py (fora deste módulo C).
 *
 * @section app_entrada_saida Entrada e saída do algoritmo
 * - Entrada: sequência de bytes (@c uint8_t), comprimento @c int (número de bytes).
 * - Saída: buffer de bytes (@c uint8_t) com cabeçalho little-endian e corpo comprimido;
 *   o tamanho útil é o valor de retorno de @ref arithmetic_compress.
 *
 * @section app_plataforma Plataforma alvo
 * C99 ou superior; tipos de largura fixa (@c stdint.h). Compiladores típicos: GCC, Clang.
 *
 * @section app_direitos Permissões de uso (copyright)
 * Código desenvolvido no âmbito de trabalho acadêmico. Uso educacional permitido;
 * redistribuição deve preservar a indicação de autoria e o contexto acadêmico.
 * Consulte os docentes da disciplina quanto a regras específicas de entrega e citação.
 *
 * @author Paulo Vinícius, Pedro Lucas (equipa do projeto)
 * @date 2026
 * @note Contexto: documentação e implementação para disciplina (compressão aritmética / T1).
 *
 * @see arithmetic_compress
 * @see ArithmeticModel
 */

#ifndef ARITHMETIC_H
#define ARITHMETIC_H

#include <stddef.h>
#include <stdint.h>

/** Limite superior da soma das frequências escaladas para estabilidade do intervalo WNC (14 bits). */
#define WNC_FREQUENCY_TOTAL_MAX ((1u << 14) - 1u)

#ifndef MAX_BUFFER
/** Tamanho máximo da mensagem de entrada em bytes (buffer estático da aplicação). */
#define MAX_BUFFER (1u << 20)
#endif

/** Número de símbolos possíveis por byte (0–255). */
#define SYMBOL_BYTE_COUNT    256

/** Tamanho do cabeçalho binário: 4 bytes (comprimento) + 256×2 bytes (contagens por símbolo). */
#define ARITH_HEADER_BYTES   (sizeof(uint32_t) + SYMBOL_BYTE_COUNT * sizeof(uint16_t))

/** Limite superior teórico do buffer de saída comprimida (cabeçalho + corpo + margem). */
#define MAX_COMPRESSED_BYTES (ARITH_HEADER_BYTES + (MAX_BUFFER * 2u) + 1048576u)

/**
 * @brief Modelo de probabilidades acumuladas para codificação aritmética.
 *
 * @var cum[i] Soma acumulada até ao símbolo @e i (exclusive); @c cum[0]=0 e
 *             @c cum[256] = total de frequências escaladas.
 * @var total Soma de todas as frequências escaladas (deve ser ≤ @ref WNC_FREQUENCY_TOTAL_MAX
 *            durante a codificação quando a mensagem não é vazia).
 */
typedef struct {
    uint32_t cum[SYMBOL_BYTE_COUNT + 1];
    uint32_t total;
} ArithmeticModel;

/**
 * @brief Conta ocorrências brutas de cada byte na mensagem.
 * @param[in] data Ponteiro para os dados (pode ser NULL se @p length == 0).
 * @param[in] length Número de bytes a considerar.
 * @param[out] raw_counts Vetor de 256 elementos; cada posição i contém quantas vezes o byte i aparece.
 * @note Variáveis globais do arquivo: nenhuma (função pura sobre os argumentos).
 */
void arithmetic_count_raw(const uint8_t *data, int length, uint32_t raw_counts[SYMBOL_BYTE_COUNT]);

/**
 * @brief Escalona contagens brutas para que a soma não exceda @ref WNC_FREQUENCY_TOTAL_MAX.
 * @param[in] raw_counts Contagens por símbolo (tipicamente saída de @ref arithmetic_count_raw).
 * @param[in] message_length Comprimento da mensagem (usado na lógica de escalonamento).
 * @param[out] scaled_counts Frequências finais por símbolo, adequadas ao codificador WNC.
 * @note Variáveis globais do arquivo: nenhuma.
 */
void arithmetic_scale_counts_for_wnc(const uint32_t *raw_counts, int message_length,
                                     uint16_t scaled_counts[SYMBOL_BYTE_COUNT]);

/**
 * @brief Conta símbolos e aplica escalonamento WNC num único passo.
 * @param[in]  data    Buffer de entrada com os bytes a analisar.
 * @param[in]  length  Número de bytes em @p data.
 * @param[out] counts  Vetor de 256 frequências escalonadas, pronto para @ref arithmetic_model_from_counts.
 * @note Variáveis globais do arquivo: nenhuma. Combina internamente
 *       @ref arithmetic_count_raw e @ref arithmetic_scale_counts_for_wnc.
 */
void arithmetic_count_symbols(const uint8_t *data, int length, uint16_t counts[SYMBOL_BYTE_COUNT]);

/**
 * @brief Constrói tabela cumulativa @ref ArithmeticModel a partir de contagens escaladas.
 * @param[in] counts Frequências por símbolo (256 valores).
 * @param[out] model Modelo preenchido com @c cum[] e @c total.
 * @note Variáveis globais do módulo @c arithmetic.c: workspace interno de cópia temporária
 *       (não exposto); ver implementação.
 */
void arithmetic_model_from_counts(const uint16_t counts[SYMBOL_BYTE_COUNT], ArithmeticModel *model);

/**
 * @brief Deriva o modelo a partir dos dados (contagem + escalonamento + cumulativas).
 * @param[in]  data    Buffer de entrada com os bytes a processar.
 * @param[in]  length  Número de bytes em @p data; deve ser ≥ 0 e ≤ @ref MAX_BUFFER.
 * @param[out] model   Modelo cumulativo preenchido e pronto para @ref arithmetic_compress.
 * @note Variáveis globais do módulo @c arithmetic.c: workspace interno (@c g_cumulative_workspace)
 *       afetado indiretamente via @ref arithmetic_model_from_counts.
 */
void arithmetic_build_model_from_data(const uint8_t *data, int length, ArithmeticModel *model);

/**
 * @brief Codifica a mensagem usando o modelo fornecido e escreve cabeçalho + bitstream.
 * @param[in] input Bytes a comprimir.
 * @param[in] length Número de bytes (≥ 0, limitado por @ref MAX_BUFFER).
 * @param[in] model Modelo com frequências consistentes com a mensagem.
 * @param[out] out Buffer de saída.
 * @param[in] out_cap Capacidade máxima de @p out.
 * @return Número de bytes escritos em @p out, ou 0 em caso de erro (comprimento inválido,
 *         modelo inconsistente, ou buffer insuficiente).
 * @note Variáveis globais do arquivo: nenhuma na API; uso interno de buffers estáticos só na
 *       implementação do modelo durante montagem (ver @c arithmetic.c).
 */
size_t arithmetic_compress(const uint8_t *input, int length, const ArithmeticModel *model,
                           uint8_t *out, size_t out_cap);

#endif
