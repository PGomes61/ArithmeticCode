/**
 * @file demo.h
 * @brief Demonstrações na consola: compressão e verificação de reversibilidade (integração com @ref utils.h).
 *
 * @details
 * Usado pelo programa principal quando não há argumentos ou para cenários de teste integrados.
 * Requer execução a partir da raiz do repositório para localizar o script Python de decode.
 *
 * @section demo_plataforma Plataforma alvo
 * Unix/macOS (POSIX), compilado com GCC/Clang, padrão C99 ou superior.
 *
 * @section demo_direitos Copyright e permissões
 * Uso educacional no âmbito de trabalho de disciplina — Compressão Aritmética (T1).
 * Redistribuição permitida com menção dos autores originais.
 *
 * @author Paulo Vinícius, Pedro Lucas
 * @date 2026
 * @note Contexto: trabalho de disciplina — compressão aritmética (demonstração de round-trip).
 */

#ifndef DEMO_H
#define DEMO_H

#include <stdint.h>

/**
 * @brief Executa um caso de demonstração: pré-visualiza dados, comprime e verifica com Python.
 * @param[in] label Título do caso (texto livre para a consola).
 * @param[in] data Ponteiro para os bytes de teste.
 * @param[in] length Número de bytes.
 * @note Delegado a @ref verify_reversible_with_python; não altera variáveis globais neste módulo.
 */
void run_roundtrip_demo(const char *label, const uint8_t *data, int length);

#endif
