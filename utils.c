#include "utils.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/wait.h>

static uint8_t s_codec_compressed[MAX_COMPRESSED_BYTES];
static uint8_t s_file_input[MAX_BUFFER];

void prepare_counts(const uint8_t *input, int length, uint16_t counts[SYMBOL_BYTE_COUNT]) {
    arithmetic_count_symbols(input, length, counts);
}

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
