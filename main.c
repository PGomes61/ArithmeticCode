#include <stdio.h>
#include <string.h>
#include "arithmetic.h"
#include "demo.h"
#include "tests.h"
#include "utils.h"

static uint8_t s_demo_large_buffer[MAX_BUFFER];

static void run_builtin_demo(void) {
    printf("=========================================\n");
    printf("SISTEMA DE COMPRESSAO ARITMETICA - T1\n");
    printf("Equipe: Paulo Vinicius & Pedro Lucas\n");
    printf("=========================================\n");

    run_unity_tests();

    printf("\n--- DEMONSTRACAO (Round-Trip) ---\n");
    printf("Todos os casos usam run_roundtrip_demo -> codec_roundtrip (igual ao modo ficheiro).\n");

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
    printf("Ponto fixo 16 bits + WNC (bitstream).\n");
    printf("=========================================\n");
}

int main(int argc, char **argv) {
    if (argc >= 2) {
        int rc = run_from_file(argv[1]);
        return rc != 0 ? 1 : 0;
    }

    printf("Uso: %s [caminho_do_ficheiro]\n", argv[0]);
    printf("Sem argumentos: executa testes Unity e demos embutidas.\n");
    printf("Com ficheiro: le ate %d bytes em modo binario, comprime, descomprime, imprime.\n\n",
           MAX_BUFFER);

    run_builtin_demo();
    return 0;
}
