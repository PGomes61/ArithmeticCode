/**
 * @file utils.c
 * @brief Implementação das rotinas em @ref utils.h (compressão, verify via Python, ficheiros).
 *
 * @details
 * Buffers estáticos evitam alocação dinâmica no caminho quente de compressão e verificação.
 * As principais responsabilidades deste módulo são: comprimir dados em memória, verificar
 * reversibilidade chamando o decoder Python em processo separado, ler ficheiros de entrada e
 * exportar o binário comprimido.
 *
 * @section utils_copyright Copyright e permissões
 * Uso educacional no âmbito de trabalho de disciplina — Compressão Aritmética (T1).
 * Redistribuição permitida com menção dos autores originais.
 *
 * @section utils_plataforma Plataforma alvo
 * Unix/macOS (POSIX), compilado com GCC/Clang, padrão C99 ou superior.
 * Requer @c python3 e o script @c scripts/external_arithmetic_decode.py acessíveis no PATH/CWD.
 *
 * @author Paulo Vinícius, Pedro Lucas
 * @date 2026
 * @note Trabalho académico — disciplina de Sistemas Embarcados / Compressão de Dados.
 */

#include "utils.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/wait.h>

/**
 * @brief Buffer estático para o pacote comprimido antes de gravar temporários ou ficheiro final.
 * @note Escrito por @ref verify_reversible_with_python e @ref export_compressed_binary.
 */
static uint8_t s_codec_compressed[MAX_COMPRESSED_BYTES];

/**
 * @brief Buffer estático para leitura de ficheiros até @ref MAX_BUFFER bytes.
 * @note Escrito por @ref run_from_file e @ref export_compressed_binary.
 */
static uint8_t s_file_input[MAX_BUFFER];

/**
 * @brief Calcula as frequências escalonadas WNC de @p input e armazena em @p counts.
 * @details Atalho conveniente que chama @ref arithmetic_count_symbols directamente.
 *          Útil quando se pretende inspecionar as contagens antes de construir o modelo.
 * @param[in]  input  Buffer de dados de entrada.
 * @param[in]  length Número de bytes a analisar.
 * @param[out] counts Vetor de 256 frequências escalonadas resultantes.
 * @note Não afeta variáveis globais deste ficheiro.
 */
void prepare_counts(const uint8_t *input, int length, uint16_t counts[SYMBOL_BYTE_COUNT]) {
    arithmetic_count_symbols(input, length, counts);
}

/**
 * @brief Comprime @p input em memória e verifica a reversibilidade com o decoder Python externo.
 * @details Constrói o modelo, comprime com @ref arithmetic_compress, grava dois ficheiros
 *          temporários em @c /tmp (comprimido e original), e invoca o script Python com
 *          @c --verify. Os temporários são removidos após a execução independentemente do resultado.
 *          O executável Python e o caminho do script podem ser sobrescritos pelas variáveis de
 *          ambiente @c PYTHON e @c ARITH_DECODE_SCRIPT.
 * @param[in]  input           Buffer de dados originais a comprimir e verificar.
 * @param[in]  length          Número de bytes em @p input; deve ser ≥ 0 e ≤ @c MAX_BUFFER.
 * @param[out] packed_bytes_out Se não-NULL, recebe o tamanho do pacote comprimido em bytes.
 * @return  0 em sucesso (decoder Python confirmou reversibilidade).
 * @return -1 se a compressão falhou (length > 0 mas packed == 0).
 * @return -4 se @p length inválido (negativo ou maior que @c MAX_BUFFER).
 * @return -5 se @p input é NULL.
 * @return -6 em erro de I/O (mkstemp, write, close).
 * @return -7 se o processo Python falhou ou terminou com código != 0.
 * @note Afeta: @ref s_codec_compressed (escrito com o pacote comprimido).
 */
int verify_reversible_with_python(const uint8_t *input, int length, size_t *packed_bytes_out) {
    if (input == NULL) {
        return -5;
    }
    if (length < 0 || (size_t)length > MAX_BUFFER) {
        return -4;
    }

    ArithmeticModel model;
    arithmetic_build_model_from_data(input, length, &model);

    size_t packed =
        arithmetic_compress(input, length, &model, s_codec_compressed, sizeof(s_codec_compressed));
    if (packed_bytes_out != NULL) {
        *packed_bytes_out = packed;
    }
    if (length > 0 && packed == 0u) {
        return -1;
    }

    char comp_tpl[] = "/tmp/arith_verify_comp_XXXXXX";
    char orig_tpl[] = "/tmp/arith_verify_orig_XXXXXX";
    int fc = mkstemp(comp_tpl);
    if (fc < 0) {
        perror("mkstemp");
        return -6;
    }
    if (write(fc, s_codec_compressed, packed) != (ssize_t)packed) {
        close(fc);
        unlink(comp_tpl);
        return -6;
    }
    if (close(fc) != 0) {
        unlink(comp_tpl);
        return -6;
    }

    int fo = mkstemp(orig_tpl);
    if (fo < 0) {
        unlink(comp_tpl);
        perror("mkstemp");
        return -6;
    }
    if (length > 0 && write(fo, input, (size_t)length) != (ssize_t)length) {
        close(fo);
        unlink(comp_tpl);
        unlink(orig_tpl);
        return -6;
    }
    if (close(fo) != 0) {
        unlink(comp_tpl);
        unlink(orig_tpl);
        return -6;
    }

    const char *py = getenv("PYTHON");
    if (py == NULL || py[0] == '\0') {
        py = "python3";
    }
    const char *script = getenv("ARITH_DECODE_SCRIPT");
    if (script == NULL || script[0] == '\0') {
        script = "scripts/external_arithmetic_decode.py";
    }

    char cmd[1024];
    int nc = snprintf(cmd, sizeof cmd,
                      "%s \"%s\" \"%s\" --verify \"%s\" > /dev/null 2>&1", py, script, comp_tpl,
                      orig_tpl);
    if (nc < 0 || (size_t)nc >= sizeof cmd) {
        unlink(comp_tpl);
        unlink(orig_tpl);
        return -6;
    }

    int sys_rc = system(cmd);
    unlink(comp_tpl);
    unlink(orig_tpl);

    if (sys_rc == -1) {
        perror("system");
        return -7;
    }
    if (!WIFEXITED(sys_rc) || WEXITSTATUS(sys_rc) != 0) {
        return -7;
    }
    return 0;
}

/**
 * @brief Preenche @p buffer com uma frase repetida até atingir exactamente @c MAX_BUFFER bytes.
 * @details Usado para gerar um caso de teste grande com distribuição de símbolos não uniforme
 *          e altamente comprimível. A frase usada é uma string literal interna à função.
 * @param[out] buffer Buffer de destino com capacidade mínima de @c MAX_BUFFER bytes.
 * @param[out] length Recebe o valor @c MAX_BUFFER ao retornar.
 * @note Não afeta variáveis globais deste ficheiro. @p buffer deve ter sido alocado pelo chamador.
 */
void fill_demo_buffer_8k(uint8_t *buffer, int *length) {
    const char *phrase = "Compressao Aritmetica com renormalizacao (WNC). ";
    const int phrase_len = (int)strlen(phrase);
    int pos = 0;
    while (pos < (int)MAX_BUFFER) {
        int chunk = phrase_len;
        if (pos + chunk > (int)MAX_BUFFER) {
            chunk = (int)MAX_BUFFER - pos;
        }
        memcpy(buffer + pos, phrase, (size_t)chunk);
        pos += chunk;
    }
    *length = (int)MAX_BUFFER;
}

/**
 * @brief Lê um ficheiro binário, comprime e verifica reversibilidade com o decoder Python.
 * @details Lê até @c MAX_BUFFER bytes de @p path. Se o ficheiro for maior, emite aviso em
 *          @c stderr e processa apenas os primeiros @c MAX_BUFFER bytes. Após compressão e
 *          verificação bem-sucedida, imprime o conteúdo original em @c stdout.
 * @param[in] path Caminho para o ficheiro de entrada (modo binário).
 * @return  0 em sucesso.
 * @return -1 se o ficheiro não puder ser aberto.
 * @return -2 se @ref verify_reversible_with_python falhar.
 * @note Afeta: @ref s_file_input (sobrescrito com o conteúdo lido) e
 *       @ref s_codec_compressed (via @ref verify_reversible_with_python).
 */
int run_from_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        perror(path);
        return -1;
    }

    size_t n = fread(s_file_input, 1, sizeof(s_file_input), f);
    uint8_t probe[1];
    size_t more = fread(probe, 1, 1, f);
    fclose(f);

    if (more != 0u) {
        fprintf(stderr,
                "Aviso: ficheiro maior que MAX_BUFFER (%u bytes); apenas os primeiros %zu bytes "
                "foram lidos.\n",
                (unsigned)MAX_BUFFER, n);
    }

    printf("\n--- MODO FICHEIRO: %s ---\n", path);
    printf("Bytes lidos: %zu\n", n);

    size_t packed = 0;
    int rc = verify_reversible_with_python(s_file_input, (int)n, &packed);
    if (rc != 0) {
        fprintf(stderr, "Erro: verify_reversible_with_python falhou (codigo %d).\n", rc);
        fprintf(stderr,
                "  (Execute a partir da raiz do projeto; precisa de python3 e "
                "scripts/external_arithmetic_decode.py.)\n");
        return -2;
    }

    printf("Tamanho do pacote comprimido: %zu bytes (cabecalho %u bytes).\n", packed,
           (unsigned)ARITH_HEADER_BYTES);
    printf("Reversibilidade: OK (decoder externo em Python).\n");
    printf("--- Conteudo original (igual ao que o Python recupera) ---\n");
    fwrite(s_file_input, 1, n, stdout);
    printf("\n--- Fim da saida ---\n");

    return 0;
}

/**
 * @brief Lê @p input_path, comprime e grava o pacote binário comprimido em @p output_path.
 * @details Lê até @c MAX_BUFFER bytes de @p input_path (com aviso se truncado), constrói o
 *          modelo, comprime com @ref arithmetic_compress e grava o resultado em @p output_path.
 *          Não realiza verificação Python — use @ref verify_reversible_with_python separadamente.
 * @param[in] input_path  Caminho do ficheiro de entrada (modo binário).
 * @param[in] output_path Caminho do ficheiro de saída onde o pacote comprimido será gravado.
 * @return  0 em sucesso.
 * @return -1 se @p input_path não puder ser aberto.
 * @return -2 se a compressão falhar (entrada não-vazia mas packed == 0).
 * @return -3 se @p output_path não puder ser aberto, a escrita for incompleta, ou o fclose falhar.
 * @note Afeta: @ref s_file_input (sobrescrito com dados lidos) e
 *       @ref s_codec_compressed (sobrescrito com o pacote comprimido).
 */
int export_compressed_binary(const char *input_path, const char *output_path) {
    FILE *fin = fopen(input_path, "rb");
    if (fin == NULL) {
        perror(input_path);
        return -1;
    }

    size_t n = fread(s_file_input, 1, sizeof(s_file_input), fin);
    uint8_t probe[1];
    size_t more = fread(probe, 1, 1, fin);
    fclose(fin);

    if (more != 0u) {
        fprintf(stderr,
                "Aviso: ficheiro maior que MAX_BUFFER (%u bytes); apenas os primeiros %zu bytes "
                "foram comprimidos.\n",
                (unsigned)MAX_BUFFER, n);
    }

    ArithmeticModel model;
    arithmetic_build_model_from_data(s_file_input, (int)n, &model);

    size_t packed = arithmetic_compress(s_file_input, (int)n, &model, s_codec_compressed,
                                        sizeof(s_codec_compressed));
    if (n > 0u && packed == 0u) {
        fprintf(stderr, "export_compressed_binary: compressao falhou.\n");
        return -2;
    }

    FILE *fout = fopen(output_path, "wb");
    if (fout == NULL) {
        perror(output_path);
        return -3;
    }
    if (fwrite(s_codec_compressed, 1, packed, fout) != packed) {
        fprintf(stderr, "%s: escrita incompleta.\n", output_path);
        fclose(fout);
        return -3;
    }
    if (fclose(fout) != 0) {
        perror(output_path);
        return -3;
    }
    return 0;
}
