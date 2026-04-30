/**
 * @file utils.h
 * @brief Utilitários de demonstração: compressão, verificação com decoder externo (Python) e I/O de ficheiros.
 *
 * @section utils_visao Visão geral
 * Este módulo orquestra o uso de @ref arithmetic_compress após construir o modelo a partir dos
 * dados, invoca opcionalmente o script Python de verificação de reversibilidade, e oferece
 * fluxos para processar ficheiros na linha de comandos.
 *
 * @section utils_entrada_saida Entrada e saída
 * - Entrada: bytes em memória ou caminhos de ficheiro (leitura binária até @ref MAX_BUFFER).
 * - Saída: mensagens em consola, ficheiro comprimido bruto (modo export), ou apenas estado de
 *   verificação (sucesso/erro).
 *
 * @section utils_ambiente Variáveis de ambiente (opcionais)
 * - @c PYTHON — interpretador (predefinição: @c python3).
 * - @c ARITH_DECODE_SCRIPT — caminho do decoder (predefinição: @c scripts/external_arithmetic_decode.py).
 *
 * @section utils_plataforma Plataforma
 * Unix/POSIX (@c unistd.h, @c mkstemp, @c fork/exec implícito via @c system). Para Windows seria
 * necessário adaptar ficheiros temporários e invocação do Python.
 *
 * @section utils_direitos Copyright
 * Uso educacional no âmbito da disciplina; ver cabeçalho do projeto em @ref arithmetic.h.
 *
 * @author Paulo Vinícius, Pedro Lucas
 * @date 2026
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include "arithmetic.h"

/**
 * @brief Prepara o vetor de contagens escaladas WNC a partir da mensagem (wrapper simples).
 * @param[in] input Dados de entrada.
 * @param[in] length Comprimento em octetos.
 * @param[out] counts Saída com 256 contagens escaladas.
 * @note Não altera variáveis globais deste módulo (apenas chama @ref arithmetic_count_symbols).
 */
void prepare_counts(const uint8_t *input, int length, uint16_t counts[SYMBOL_BYTE_COUNT]);

/**
 * @brief Comprime em memória e confirma reversibilidade executando o decoder Python em modo verify.
 * @param[in] input Ponteiro para a mensagem (não NULL exceto se length==0 conforme validação).
 * @param[in] length Comprimento; deve estar entre 0 e @ref MAX_BUFFER.
 * @param[out] packed_bytes_out Se não NULL, recebe o tamanho do pacote comprimido gerado.
 * @return 0 sucesso; -1 falha de compressão; -4 tamanho inválido; -5 @p input NULL;
 *         -6 temporários/comando; -7 falha do script Python ou verify.
 * @note Afeta buffers estáticos @c s_codec_compressed e @c s_file_input no @c utils.c (escrita
 *       temporária para ficheiros auxiliares na verificação).
 */
int verify_reversible_with_python(const uint8_t *input, int length, size_t *packed_bytes_out);

/**
 * @brief Preenche @p buffer com texto repetido até preencher @ref MAX_BUFFER octetos.
 * @param[out] buffer Destino com capacidade mínima de @ref MAX_BUFFER bytes;
 *                    deve ter sido alocado pelo chamador.
 * @param[out] length Definido para @ref MAX_BUFFER ao retornar.
 * @note Não afeta variáveis globais deste módulo.
 */
void fill_demo_buffer_8k(uint8_t *buffer, int *length);

/**
 * @brief Lê ficheiro, comprime, verifica com Python e imprime o conteúdo original no stdout.
 * @param[in] path Caminho do ficheiro em modo binário.
 * @return 0 sucesso; -1 erro ao abrir; -2 falha na verificação.
 * @note Usa o buffer estático @c s_file_input e @c s_codec_compressed em @c utils.c.
 */
int run_from_file(const char *path);

/**
 * @brief Lê entrada, comprime e grava o binário completo (cabeçalho + corpo) no ficheiro de saída.
 * @param[in] input_path  Caminho do ficheiro fonte (leitura binária).
 * @param[in] output_path Caminho do ficheiro comprimido a gerar (escrita binária).
 * @return  0 em sucesso.
 * @return -1 se @p input_path não puder ser aberto.
 * @return -2 se a compressão falhar (entrada não-vazia mas packed == 0).
 * @return -3 se @p output_path não puder ser aberto, a escrita for incompleta ou o fclose falhar.
 * @note Afeta buffers estáticos @c s_file_input e @c s_codec_compressed em @c utils.c.
 */
int export_compressed_binary(const char *input_path, const char *output_path);

#endif
