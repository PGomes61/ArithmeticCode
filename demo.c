/**
 * @file demo.c
 * @brief Implementação das demonstrações declaradas em @ref demo.h.
 *
 * @details
 * Fornece funções auxiliares de pré-visualização (texto ou hexadecimal) e a função principal
 * de demonstração @ref run_roundtrip_demo, que comprime uma mensagem em C e confirma a
 * reversibilidade com o decoder externo em Python.
 *
 * @section demo_copyright Copyright e permissões
 * Uso educacional no âmbito de trabalho de disciplina — Compressão Aritmética (T1).
 * Redistribuição permitida com menção dos autores originais.
 *
 * @section demo_plataforma Plataforma alvo
 * Unix/macOS (POSIX), compilado com GCC/Clang, padrão C99 ou superior.
 *
 * @author Paulo Vinícius, Pedro Lucas
 * @date 2026
 * @note Trabalho acadêmico — disciplina de Sistemas Embarcados.
 */

#include "demo.h"
#include "utils.h"
#include "arithmetic.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Verifica se os primeiros @p max_scan bytes são majoritariamente texto imprimível.
 * @param data Buffer de entrada.
 * @param length Comprimento total disponível.
 * @param max_scan Número máximo de bytes a inspecionar.
 * @return 1 se parecer texto; 0 caso contrário.
 */
static int prefix_is_printable_text(const uint8_t *data, int length, int max_scan) {
    int n = length < max_scan ? length : max_scan;
    for (int i = 0; i < n; i++) {
        unsigned char c = data[i];
        if (c == '\n' || c == '\r' || c == '\t') {
            continue;
        }
        if (!isprint((int)c)) {
            return 0;
        }
    }
    return 1;
}

/**
 * @brief Imprime pré-visualização do conteúdo: texto legível ou amostra hexadecimal.
 * @details Se o conteúdo parecer texto imprimível (verificado via @ref prefix_is_printable_text),
 *          exibe até 200 caracteres como string; caso contrário, exibe até 32 bytes em hex.
 * @param[in] data   Buffer com os dados a pré-visualizar.
 * @param[in] length Número de bytes em @p data.
 * @note Não afeta variáveis globais deste arquivo.
 */
static void print_preview(const uint8_t *data, int length) {
    if (length <= 0) {
        printf("(vazio)\n");
        return;
    }
    if (prefix_is_printable_text(data, length, length < 512 ? length : 512)) {
        int show = length < 200 ? length : 200;
        printf("Como texto (%d bytes): \"%.*s\"\n", length, show, (const char *)data);
        if (length > show) {
            printf("... (%d bytes adicionais nao mostrados na linha acima)\n", length - show);
        }
    } else {
        int nhex = length < 32 ? length : 32;
        printf("Amostra hex (%d de %d bytes): ", nhex, length);
        for (int i = 0; i < nhex; i++) {
            printf("%02X ", (unsigned)data[i]);
        }
        printf("\n");
    }
}

/**
 * @brief Executa uma demonstração completa de compressão e verificação de reversibilidade.
 * @details Imprime um separador com @p label, exibe uma pré-visualização dos dados via
 *          @ref print_preview (texto ou hex conforme o conteúdo), comprime em C e invoca
 *          o decoder Python com @ref verify_reversible_with_python, reportando o resultado
 *          (tamanho do pacote e status OK/ERRO) em @c stdout.
 * @param[in] label  Rótulo identificador do caso de teste, exibido no cabeçalho da saída.
 * @param[in] data   Buffer com os dados a comprimir e verificar.
 * @param[in] length Número de bytes em @p data.
 * @note Não afeta variáveis globais deste arquivo. Variáveis globais em @c utils.c
 *       (@c s_codec_compressed, @c s_file_input) são afetadas indiretamente via
 *       @ref verify_reversible_with_python.
 */
void run_roundtrip_demo(const char *label, const uint8_t *data, int length) {
    size_t packed = 0;

    printf("\n--- %s ---\n", label);
    print_preview(data, length);

    int rc = verify_reversible_with_python(data, length, &packed);
    printf("Pacote comprimido: %zu bytes (cabecalho %u bytes).\n", packed,
           (unsigned)ARITH_HEADER_BYTES);

    if (rc != 0) {
        printf("Status: [ ERRO ] - verify_reversible_with_python codigo %d.\n", rc);
        return;
    }

    printf("Status: [ OK ] - Reversivel (decoder externo Python).\n");
}
