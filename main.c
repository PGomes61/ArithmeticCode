#include <stdio.h>
#include <string.h>
#include "arithmetic.h"
#include "demo.h"
#include "utils.h"

static uint8_t s_demo_large_buffer[MAX_BUFFER];

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
