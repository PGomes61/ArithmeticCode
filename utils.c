#include "utils.h"
#include "arithmetic.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

static int is_printable_ascii(const uint8_t *data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (data[i] < 32 || data[i] > 126) {
            return 0;
        }
    }
    return 1;
}

static void print_bytes_description(const char *prefix, const uint8_t *data, size_t length) {
    printf("%s (%zu bytes): ", prefix, length);
    if (length == 0) {
        printf("(vazio)\n");
        return;
    }
    if (is_printable_ascii(data, length) && length <= 512) {
        putchar('"');
        for (size_t i = 0; i < length; i++) {
            putchar((int)data[i]);
        }
        putchar('"');
        putchar('\n');
    } else {
        size_t max_show = length < 48 ? length : 48;
        for (size_t i = 0; i < max_show; i++) {
            printf("%02X ", data[i]);
        }
        if (length > max_show) {
            printf("... (+%zu bytes)", length - max_show);
        }
        putchar('\n');
    }
}

void compute_frequencies(const uint8_t *input, size_t length, double *freq) {
    if (length == 0U) {
        double u = 1.0 / (double)SYMBOL_COUNT;
        for (int i = 0; i < SYMBOL_COUNT; i++) {
            freq[i] = u;
        }
        return;
    }

    unsigned int count[SYMBOL_COUNT];
    for (int i = 0; i < SYMBOL_COUNT; i++) {
        count[i] = 0U;
    }
    for (size_t i = 0; i < length; i++) {
        count[input[i]]++;
    }

    for (int i = 0; i < SYMBOL_COUNT; i++) {
        freq[i] = (double)count[i] / (double)length;
    }

    double sum = 0.0;
    for (int i = 0; i < SYMBOL_COUNT; i++) {
        sum += freq[i];
    }
    if (sum > 0.0) {
        for (int i = 0; i < SYMBOL_COUNT; i++) {
            freq[i] /= sum;
        }
    }
}

/**
 * spec == 0: um único bloco com a mensagem inteira.
 * spec > 0: no máximo spec bytes por bloco (blocos contíguos).
 */
static int roundtrip(const uint8_t *input,
                     uint8_t *decoded,
                     size_t length,
                     size_t spec,
                     int print_tags) {
    memset(decoded, 0, MAX_BUFFER_SIZE);
    if (length == 0U) {
        return 1;
    }

    size_t max_chunk = spec == 0U ? length : spec;

    double freq[SYMBOL_COUNT];
    size_t block_index = 0U;

    for (size_t off = 0; off < length;) {
        size_t chunk_len = length - off;
        if (chunk_len > max_chunk) {
            chunk_len = max_chunk;
        }

        compute_frequencies(input + off, chunk_len, freq);
        build_cumulative_table(freq);
        long double tag = encode(input + off, chunk_len);
        if (print_tags) {
            printf("  Bloco %zu — tag: %.21Lf\n", block_index + 1U, tag);
        }
        decode(tag, decoded + off, chunk_len);

        off += chunk_len;
        block_index++;
    }

    return memcmp(input, decoded, length) == 0;
}

void run_test_bytes(const char *label, const uint8_t *data, size_t length) {
    if (length > MAX_BUFFER_SIZE) {
        fprintf(stderr,
                "[ERRO] \"%s\": comprimento %zu > MAX_BUFFER_SIZE (%d).\n",
                label,
                length,
                MAX_BUFFER_SIZE);
        return;
    }

    uint8_t input[MAX_BUFFER_SIZE];
    uint8_t decoded[MAX_BUFFER_SIZE];

    memcpy(input, data, length);

    printf("\n--- TESTE: %s ---\n", label);
    print_bytes_description("Original", input, length);

    if (length == 0U) {
        printf("Tag: (vazio)\n");
        print_bytes_description("Decodificado", decoded, length);
        printf("Status: [ OK ] - Integridade verificada.\n");
        return;
    }

    static const size_t fallback_specs[] = {0U, 24U, 20U, 16U, 14U, 12U, 10U, 8U, 6U, 5U, 4U};
    size_t winner_spec = (size_t)-1;

    for (size_t i = 0; i < sizeof(fallback_specs) / sizeof(fallback_specs[0]); i++) {
        if (roundtrip(input, decoded, length, fallback_specs[i], 0)) {
            winner_spec = fallback_specs[i];
            break;
        }
    }

    if (winner_spec == (size_t)-1) {
        printf(
            "Não foi possível recuperar a mensagem com os tamanhos de bloco testados.\n");
        memset(decoded, 0, sizeof(decoded));
        print_bytes_description("Decodificado", decoded, length);
        printf(
            "Status: [ ERRO ] - Veja README-DOCUMENTACAO.md (precisão finita em ponto "
            "flutuante).\n");
        return;
    }

    if (winner_spec == 0U) {
        printf("Modo: uma passagem na mensagem inteira (%zu bytes).\n", length);
    } else {
        printf("Modo: blocos contíguos de até %zu bytes (precisão finita).\n", winner_spec);
    }

    roundtrip(input, decoded, length, winner_spec, 1);

    print_bytes_description("Decodificado", decoded, length);

    if (memcmp(input, decoded, length) == 0) {
        printf("Status: [ OK ] - Integridade verificada.\n");
    } else {
        printf("Status: [ ERRO ] - Estado inconsistente após escolha do modo.\n");
    }
}

void run_test(const char *label, const char *test_string) {
    if (test_string == NULL) {
        fprintf(stderr, "[ERRO] \"%s\": string nula.\n", label);
        return;
    }
    run_test_bytes(label, (const uint8_t *)test_string, strlen(test_string));
}
