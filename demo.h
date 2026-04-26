#ifndef DEMO_H
#define DEMO_H

#include <stdint.h>

/**
 * Demonstração na consola: pré-visualização, compressão e verificação de reversibilidade via
 * scripts/external_arithmetic_decode.py (executar a partir da raiz do repositório).
 */
void run_roundtrip_demo(const char *label, const uint8_t *data, int length);

#endif
