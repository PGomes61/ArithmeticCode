#include "demo.h"
#include "utils.h"
#include "arithmetic.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

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
