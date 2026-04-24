#include "utils.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

static uint8_t s_codec_compressed[MAX_COMPRESSED_BYTES];
static uint8_t s_file_input[MAX_BUFFER];
static uint8_t s_file_decoded[MAX_BUFFER];

void prepare_counts(const uint8_t *input, int length, uint16_t counts[SYMBOL_BYTE_COUNT]) {
    arithmetic_count_symbols(input, length, counts);
}

int codec_roundtrip(const uint8_t *input, int length, uint8_t *decoded_out, size_t decoded_cap,
                    size_t *packed_bytes_out) {
    if (input == NULL || decoded_out == NULL) {
        return -5;
    }
    if (length < 0 || (size_t)length > MAX_BUFFER || decoded_cap < (size_t)length) {
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

    memset(decoded_out, 0, decoded_cap);
    uint32_t out_n = 0;
    int dec = arithmetic_decompress(s_codec_compressed, packed, decoded_out, decoded_cap, &out_n);
    if (dec != 0 || out_n != (uint32_t)length) {
        return -2;
    }
    if (length > 0 && memcmp(input, decoded_out, (size_t)length) != 0) {
        return -3;
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
    int rc = codec_roundtrip(s_file_input, (int)n, s_file_decoded, sizeof(s_file_decoded), &packed);
    if (rc != 0) {
        fprintf(stderr, "Erro: codec_roundtrip falhou (codigo %d).\n", rc);
        return -2;
    }

    printf("Tamanho do pacote comprimido: %zu bytes (cabecalho %u bytes).\n", packed,
           (unsigned)ARITH_HEADER_BYTES);
    printf("Round-trip: OK.\n");
    printf("--- Saida decodificada (texto; bytes nao imprimiveis podem nao aparecer bem) ---\n");
    fwrite(s_file_decoded, 1, n, stdout);
    printf("\n--- Fim da saida ---\n");

    return 0;
}
