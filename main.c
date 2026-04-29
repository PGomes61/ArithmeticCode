/**
 * @file main.c
 * @brief Ponto de entrada: compressão aritmética, demonstrações e modo ficheiro.
 *
 * @section main_visao Visão geral da aplicação
 * Programa de linha de comandos que comprime mensagens em memória com codificação aritmética
 * WNC (implementação em @c arithmetic.c), opcionalmente valida a reversibilidade com um
 * decoder de referência em Python, e permite gravar o binário comprimido ou processar um
 * ficheiro de entrada.
 *
 * @section main_como_usar Como usar (help)
 * @verbatim
   Compilar (exemplo):
     gcc -std=c11 -O2 -Wall -Wextra -o prog main.c arithmetic.c utils.c demo.c

   Execução:
     ./prog
       → Sem argumentos: corre demonstrações integradas e chama o verify Python (requer
         python3 e scripts/external_arithmetic_decode.py na raiz do projeto).

     ./prog caminho/para/ficheiro.bin
       → Lê até MAX_BUFFER bytes, comprime, verifica com Python, imprime o conteúdo original.

     ./prog --dump-compressed entrada.bin saida.comp
       → Grava só o pacote comprimido (cabeçalho + bitstream) em saida.comp.
@endverbatim
 *
 * @section main_io Entrada e saída
 * - Entrada: argumentos da linha de comandos; dados lidos de ficheiros como sequência de octetos.
 * - Saída: texto em @c stdout/@c stderr; ficheiro binário no modo @c --dump-compressed.
 *
 * @section main_plataforma Plataforma alvo
 * Unix/macOS (POSIX). O verify Python usa @c mkstemp em @c utils.c.
 *
 * @section main_direitos Copyright e permissões
 * Uso educacional no âmbito de trabalho de disciplina. Redistribuição com menção de autores.
 *
 * @author Paulo Vinícius, Pedro Lucas
 * @date 2026
 * @note Trabalho académico — compressão aritmética (T1).
 */

#include <stdio.h>
#include <string.h>
#include "arithmetic.h"
#include "demo.h"
#include "utils.h"

/**
 * @brief Buffer estático para o caso de demonstração com ~MAX_BUFFER bytes.
 * @note Usado apenas por @ref run_builtin_demo.
 */
static uint8_t s_demo_large_buffer[MAX_BUFFER];

/**
 * @brief Executa a sequência de demonstrações pré-definidas (vários tamanhos e tipos de dados).
 * @details Chama @ref run_roundtrip_demo para cada caso; o último preenche um buffer grande com
 *          texto repetido via @ref fill_demo_buffer_8k.
 * @note Afeta: @c s_demo_large_buffer (escrita). Variáveis globais em @c utils.c indiretamente
 *       durante a verificação Python.
 */
static void run_builtin_demo(void) {
    printf("=========================================\n");
    printf("SISTEMA DE COMPRESSAO ARITMETICA - T1\n");
    printf("Equipe: Paulo Vinicius & Pedro Lucas\n");
    printf("=========================================\n");

    printf("\n--- DEMONSTRACAO (reversibilidade via decoder Python) ---\n");
    printf("Cada caso comprime em C e confirma com scripts/external_arithmetic_decode.py.\n");
    printf("(Execute sem argumentos a partir da raiz do repositorio.)\n");

    run_roundtrip_demo("Dados binarios (4 bytes)", (const uint8_t *)"\x01\x02\x03\x01", 4);
    run_roundtrip_demo("String curta: BANANA", (const uint8_t *)"BANANA", 6);
    run_roundtrip_demo("String longa",
                       (const uint8_t *)"Compressao Aritmetica eh Top",
                       (int)strlen("Compressao Aritmetica eh Top"));

    int big_len = 0;
    fill_demo_buffer_8k(s_demo_large_buffer, &big_len);
    run_roundtrip_demo("Buffer grande (~MAX_BUFFER bytes, texto repetido)", s_demo_large_buffer,
                       big_len);

    printf("\n=========================================\n");
    printf("Encoder em C; decoder de referencia em Python.\n");
    printf("=========================================\n");
}

/**
 * @brief Função principal: despacha modos demo, ficheiro único ou export comprimido.
 * @param argc Número de argumentos (incluindo o nome do programa).
 * @param argv argv[0] nome do executável; ver secção "Como usar" para os restantes.
 * @return 0 em sucesso; 1 em erro de processamento ou compressão/verify.
 * @note Não mantém estado global próprio além de buffers estáticos deste ficheiro em @ref run_builtin_demo.
 */
int main(int argc, char **argv) {
    if (argc == 4 && strcmp(argv[1], "--dump-compressed") == 0) {
        int rc = export_compressed_binary(argv[2], argv[3]);
        if (rc != 0) {
            return 1;
        }
        printf("Comprimido gravado em: %s\n", argv[3]);
        return 0;
    }

    if (argc >= 2) {
        int rc = run_from_file(argv[1]);
        return rc != 0 ? 1 : 0;
    }

    printf("Uso: %s [caminho_do_ficheiro]\n", argv[0]);
    printf("      %s --dump-compressed entrada.bin saida.comp\n", argv[0]);
    printf("Sem argumentos: demos de reversibilidade (Python) no diretorio atual.\n");
    printf("Com ficheiro: le ate %d bytes, comprime, verifica com Python, imprime original.\n\n",
           MAX_BUFFER);

    run_builtin_demo();
    return 0;
}
