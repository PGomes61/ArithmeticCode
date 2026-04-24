#ifndef DEMO_H
#define DEMO_H

#include <stdint.h>

/**
 * Demonstração na consola: mostra o conteúdo (texto ou hex), chama codec_roundtrip
 * e imprime tamanho do pacote e status. Mesmo pipeline para qualquer tamanho de dados.
 */
void run_roundtrip_demo(const char *label, const uint8_t *data, int length);

#endif
