/**
 * @file arithmetic.c
 * @brief Implementação da codificação aritmética WNC e construção do modelo (ver @ref arithmetic.h).
 *
 * @details
 * Funções internas estáticas tratam de escrita little-endian, buffer de bits e um passo de
 * codificação com renormalização. O workspace @c g_cumulative_workspace evita stack grande nas
 * cópias intermediárias para cumulativas.
 *
 * @section arith_copyright Copyright e permissões
 * Uso educacional no âmbito de trabalho de disciplina — Compressão Aritmética (T1).
 * Redistribuição permitida com menção dos autores originais.
 *
 * @section arith_plataforma Plataforma alvo
 * Unix/macOS (POSIX), compilado com GCC/Clang, padrão C99 ou superior.
 *
 * @author Paulo Vinícius, Pedro Lucas
 * @date 2026
 * @note Trabalho académico — disciplina de Sistemas Embarcados.
 */

#include "arithmetic.h"
#include <string.h>

/** Número de palavras de 32 bits para cópia temporária das contagens (256 símbolos). */
#define CUM_WORK_WORDS 2048

/**
 * @brief Workspace estático para cópia das contagens antes de formar @c cum[] no modelo.
 * @note Afetado por: @ref copy_counts_to_workspace, lido em @ref arithmetic_model_from_counts.
 */
static uint32_t g_cumulative_workspace[CUM_WORK_WORDS];

/**
 * @brief Copia contagens uint16 para workspace interno (promovidas para uint32).
 * @param[in] counts Frequências por símbolo.
 * @note Afeta: @ref g_cumulative_workspace.
 */
static void copy_counts_to_workspace(const uint16_t counts[SYMBOL_BYTE_COUNT]) {
    for (int i = 0; i < SYMBOL_BYTE_COUNT; i++) {
        g_cumulative_workspace[i] = counts[i];
    }
}

/**
 * @brief Escreve uint32 em little-endian nos quatro primeiros bytes de @p p.
 * @param[out] p Ponteiro ao buffer de destino; deve ter pelo menos 4 bytes disponíveis.
 * @param[in]  v Valor de 32 bits a escrever.
 * @note Não afeta variáveis globais deste ficheiro.
 */
static void write_u32_le(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

/**
 * @brief Escreve uint16 em little-endian em @p p.
 * @param[out] p Ponteiro ao buffer de destino; deve ter pelo menos 2 bytes disponíveis.
 * @param[in]  v Valor de 16 bits a escrever.
 * @note Não afeta variáveis globais deste ficheiro.
 */
static void write_u16_le(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

/**
 * @brief Estado do escritor de bits (bit packing MSB-first dentro de cada byte de saída).
 */
typedef struct {
    uint8_t *buf;   /**< Início do buffer de saída. */
    size_t cap;     /**< Capacidade máxima do buffer. */
    size_t pos;     /**< Próximo índice de byte a escrever após bytes completos. */
    unsigned nbits; /**< Bits já acumulados no byte corrente (0–8). */
    unsigned cur;   /**< Valor parcial do byte corrente. */
} BitWriter;

/**
 * @brief Inicializa o escritor de bits.
 * @param w Escritor.
 * @param buf Buffer destino.
 * @param cap Capacidade de @p buf.
 */
static void bw_init(BitWriter *w, uint8_t *buf, size_t cap) {
    w->buf = buf;
    w->cap = cap;
    w->pos = 0;
    w->nbits = 0;
    w->cur = 0;
}

/**
 * @brief Adiciona um bit ao stream; emite um byte quando acumula 8 bits.
 * @param[in,out] w   Escritor de bits cujo estado é atualizado.
 * @param[in]     bit Bit a inserir (apenas o bit menos significativo é usado).
 * @return 0 em sucesso, -1 se o buffer exceder @c cap.
 * @note Não afeta variáveis globais deste ficheiro.
 */
static int bw_flush_bit(BitWriter *w, unsigned bit) {
    w->cur = (unsigned)((w->cur << 1) | (bit & 1u));
    w->nbits++;
    if (w->nbits == 8u) {
        if (w->pos >= w->cap) {
            return -1;
        }
        w->buf[w->pos++] = (uint8_t)w->cur;
        w->nbits = 0;
        w->cur = 0;
    }
    return 0;
}

/**
 * @brief Emite um bit seguido de @p *bits_to_follow bits opostos (seguimento WNC).
 * @param[in,out] w              Escritor de bits cujo estado é atualizado.
 * @param[in]     bit            Bit principal a emitir (0 ou 1).
 * @param[in,out] bits_to_follow Contador de bits pendentes; decrementado até zero durante o seguimento.
 * @return 0 em sucesso, -1 se overflow de buffer.
 * @note Não afeta variáveis globais deste ficheiro.
 */
static int bw_bit_plus_follow(BitWriter *w, unsigned bit, unsigned *bits_to_follow) {
    if (bw_flush_bit(w, bit) != 0) {
        return -1;
    }
    while (*bits_to_follow > 0u) {
        if (bw_flush_bit(w, bit ^ 1u) != 0) {
            return -1;
        }
        (*bits_to_follow)--;
    }
    return 0;
}

/**
 * @brief Completa o byte corrente com zeros até alinhar à fronteira de octeto.
 * @param[in,out] w Escritor de bits cujo byte parcial será preenchido com zeros.
 * @return 0 em sucesso, -1 se overflow de buffer durante o preenchimento.
 * @note Não afeta variáveis globais deste ficheiro.
 */
static int bw_pad_to_byte(BitWriter *w) {
    while (w->nbits > 0u) {
        if (bw_flush_bit(w, 0u) != 0) {
            return -1;
        }
    }
    return 0;
}

/**
 * @brief Um passo de codificação aritmética para um símbolo, com renormalização e bits emitidos.
 * @param bw Escritor de bits.
 * @param low,high Ponteiros ao intervalo atual [low, high] (16 bits efetivos).
 * @param bits_to_follow Acumulador de seguimento WNC.
 * @param model Modelo cumulativo.
 * @param symbol Símbolo a codificar (0–255).
 * @return 0 em sucesso, -1 se falha de buffer no escritor.
 * @note Não altera variáveis globais do ficheiro.
 */
static int encode_one_symbol(BitWriter *bw, uint32_t *low, uint32_t *high, unsigned *bits_to_follow,
                             const ArithmeticModel *model, int symbol) {
    const uint32_t MAX_VALUE = (1u << 16) - 1u;
    const uint32_t HALF = 1u << 15u;
    const uint32_t FIRST_QTR = HALF >> 1u;
    const uint32_t THIRD_QTR = FIRST_QTR * 3u;
    const uint32_t total = model->total;
    const uint64_t range = (uint64_t)(*high - *low) + 1u;
    const uint32_t cum_low = model->cum[symbol];
    const uint32_t cum_high = model->cum[symbol + 1];

    *high = (uint32_t)(*low + (uint32_t)((range * (uint64_t)cum_high) / (uint64_t)total) - 1u);
    *low = (uint32_t)(*low + (uint32_t)((range * (uint64_t)cum_low) / (uint64_t)total));

    for (;;) {
        if (*high < HALF) {
            if (bw_bit_plus_follow(bw, 0u, bits_to_follow) != 0) {
                return -1;
            }
            *low = (*low << 1u) & MAX_VALUE;
            *high = ((*high << 1u) | 1u) & MAX_VALUE;
        } else if (*low >= HALF) {
            if (bw_bit_plus_follow(bw, 1u, bits_to_follow) != 0) {
                return -1;
            }
            *low = ((*low - HALF) << 1u) & MAX_VALUE;
            *high = (((*high - HALF) << 1u) | 1u) & MAX_VALUE;
        } else if (*low >= FIRST_QTR && *high < THIRD_QTR) {
            (*bits_to_follow)++;
            *low = ((*low - FIRST_QTR) << 1u) & MAX_VALUE;
            *high = (((*high - FIRST_QTR) << 1u) | 1u) & MAX_VALUE;
        } else {
            break;
        }
    }
    return 0;
}

/**
 * @brief Conta as ocorrências brutas de cada símbolo (byte) em @p data.
 * @param[in]  data       Buffer de entrada com os dados a analisar.
 * @param[in]  length     Número de bytes a processar.
 * @param[out] raw_counts Vetor de 256 posições preenchido com a frequência absoluta de cada símbolo.
 * @note Não afeta variáveis globais deste ficheiro.
 */
void arithmetic_count_raw(const uint8_t *data, int length, uint32_t raw_counts[SYMBOL_BYTE_COUNT]) {
    for (int i = 0; i < SYMBOL_BYTE_COUNT; i++) {
        raw_counts[i] = 0u;
    }
    for (int i = 0; i < length; i++) {
        raw_counts[data[i]]++;
    }
}

/**
 * @brief Escala as contagens brutas para que a soma não exceda @c WNC_FREQUENCY_TOTAL_MAX.
 * @details Se a mensagem couber dentro do limite WNC, as contagens são copiadas directamente.
 *          Caso contrário, cada contagem é proporcionalmente reduzida; símbolos com peso > 0
 *          recebem no mínimo 1 para preservar a distinguibilidade, e ajustes incrementais
 *          corrigem erros de arredondamento para garantir que a soma seja exactamente @c WNC_FREQUENCY_TOTAL_MAX.
 * @param[in]  raw_counts     Frequências absolutas (256 símbolos), obtidas por @ref arithmetic_count_raw.
 * @param[in]  message_length Comprimento original da mensagem em bytes; usado como divisor.
 * @param[out] scaled_counts  Frequências escalonadas prontas para @ref arithmetic_model_from_counts.
 * @note Não afeta variáveis globais deste ficheiro.
 */
void arithmetic_scale_counts_for_wnc(const uint32_t *raw_counts, int message_length,
                                     uint16_t scaled_counts[SYMBOL_BYTE_COUNT]) {
    for (int i = 0; i < SYMBOL_BYTE_COUNT; i++) {
        scaled_counts[i] = 0;
    }
    if (message_length <= 0) {
        return;
    }

    const uint32_t L = WNC_FREQUENCY_TOTAL_MAX;
    const uint32_t n = (uint32_t)message_length;

    if (n <= L) {
        for (int i = 0; i < SYMBOL_BYTE_COUNT; i++) {
            scaled_counts[i] = (uint16_t)raw_counts[i];
        }
        return;
    }

    uint32_t scaled[SYMBOL_BYTE_COUNT];
    uint32_t sum = 0;
    for (int i = 0; i < SYMBOL_BYTE_COUNT; i++) {
        if (raw_counts[i] == 0u) {
            scaled[i] = 0u;
        } else {
            uint32_t v = (uint32_t)(((uint64_t)raw_counts[i] * (uint64_t)L) / (uint64_t)n);
            if (v == 0u) {
                v = 1u;
            }
            scaled[i] = v;
        }
        sum += scaled[i];
    }

    while (sum > L) {
        int best = -1;
        uint32_t best_val = 0u;
        for (int i = 0; i < SYMBOL_BYTE_COUNT; i++) {
            if (scaled[i] > 1u && (best < 0 || scaled[i] > best_val)) {
                best = i;
                best_val = scaled[i];
            }
        }
        if (best < 0) {
            break;
        }
        scaled[best]--;
        sum--;
    }

    while (sum < L) {
        int best = -1;
        uint32_t best_margin = 0u;
        for (int i = 0; i < SYMBOL_BYTE_COUNT; i++) {
            if (raw_counts[i] > scaled[i]) {
                const uint32_t margin = raw_counts[i] - scaled[i];
                if (best < 0 || margin > best_margin) {
                    best = i;
                    best_margin = margin;
                }
            }
        }
        if (best >= 0) {
            scaled[best]++;
            sum++;
        } else {
            for (int i = 0; i < SYMBOL_BYTE_COUNT; i++) {
                if (raw_counts[i] > 0u) {
                    scaled[i]++;
                    sum++;
                    break;
                }
            }
        }
    }

    for (int i = 0; i < SYMBOL_BYTE_COUNT; i++) {
        scaled_counts[i] = (uint16_t)scaled[i];
    }
}

/**
 * @brief Combina contagem bruta e escalonamento WNC numa única chamada.
 * @param[in]  data    Buffer de entrada.
 * @param[in]  length  Número de bytes a processar.
 * @param[out] counts  Frequências escalonadas (prontas para o modelo), 256 entradas.
 * @note Internamente chama @ref arithmetic_count_raw e @ref arithmetic_scale_counts_for_wnc.
 *       Não afeta variáveis globais deste ficheiro.
 */
void arithmetic_count_symbols(const uint8_t *data, int length, uint16_t counts[SYMBOL_BYTE_COUNT]) {
    uint32_t raw[SYMBOL_BYTE_COUNT];
    arithmetic_count_raw(data, length, raw);
    arithmetic_scale_counts_for_wnc(raw, length, counts);
}

/**
 * @brief Constrói o modelo aritmético directamente a partir dos dados de entrada.
 * @details Conveniência que encadeia @ref arithmetic_count_symbols e @ref arithmetic_model_from_counts.
 * @param[in]  data   Buffer de entrada.
 * @param[in]  length Número de bytes a processar.
 * @param[out] model  Modelo cumulativo preenchido e pronto para @ref arithmetic_compress.
 * @note Afeta: @ref g_cumulative_workspace (via @ref arithmetic_model_from_counts).
 */
void arithmetic_build_model_from_data(const uint8_t *data, int length, ArithmeticModel *model) {
    uint16_t scaled[SYMBOL_BYTE_COUNT];
    arithmetic_count_symbols(data, length, scaled);
    arithmetic_model_from_counts(scaled, model);
}

/**
 * @brief Constrói o modelo cumulativo a partir de frequências escalonadas já conhecidas.
 * @details Preenche @p model->cum[0..SYMBOL_BYTE_COUNT] por soma prefixada das contagens e
 *          define @p model->total como o total acumulado. Usa @ref g_cumulative_workspace
 *          como buffer interno para a promoção uint16 → uint32.
 * @param[in]  counts Frequências escalonadas para cada símbolo (256 entradas).
 * @param[out] model  Modelo cumulativo preenchido.
 * @note Afeta: @ref g_cumulative_workspace.
 */
void arithmetic_model_from_counts(const uint16_t counts[SYMBOL_BYTE_COUNT], ArithmeticModel *model) {
    copy_counts_to_workspace(counts);
    model->cum[0] = 0u;
    for (int i = 0; i < SYMBOL_BYTE_COUNT; i++) {
        model->cum[i + 1] = model->cum[i] + g_cumulative_workspace[i];
    }
    model->total = model->cum[SYMBOL_BYTE_COUNT];
}

/**
 * @brief Comprime @p input com o modelo fornecido, gravando cabeçalho + bitstream em @p out.
 * @details O cabeçalho ocupa @c ARITH_HEADER_BYTES (4 bytes de comprimento LE + 512 bytes de
 *          contagens LE). O corpo é o bitstream WNC com padding de zeros no último byte.
 *          Garante pelo menos 2 bytes de corpo para que o decoder possa inicializar o buffer.
 * @param[in]  input   Buffer de dados originais a comprimir.
 * @param[in]  length  Número de bytes a comprimir; deve ser ≤ @c MAX_BUFFER.
 * @param[in]  model   Modelo cumulativo construído sobre @p input.
 * @param[out] out     Buffer de saída onde o pacote comprimido será escrito.
 * @param[in]  out_cap Capacidade máxima de @p out em bytes.
 * @return Número de bytes escritos em @p out; 0 em caso de erro (buffer insuficiente,
 *         modelo inválido, @p length fora do intervalo, ou overflow do BitWriter).
 * @note Afeta: estado interno do @c BitWriter (local); lê @ref g_cumulative_workspace
 *       indiretamente através de @p model (já construído antes desta chamada).
 */
size_t arithmetic_compress(const uint8_t *input, int length, const ArithmeticModel *model,
                           uint8_t *out, size_t out_cap) {
    if (length < 0 || (size_t)length > MAX_BUFFER) {
        return 0;
    }
    if (out_cap < ARITH_HEADER_BYTES) {
        return 0;
    }

    write_u32_le(out, (uint32_t)length);
    for (int i = 0; i < SYMBOL_BYTE_COUNT; i++) {
        uint16_t c = 0;
        if (length > 0) {
            c = (uint16_t)(model->cum[i + 1] - model->cum[i]);
        }
        write_u16_le(out + sizeof(uint32_t) + (size_t)i * sizeof(uint16_t), c);
    }

    if (length == 0 || model->total == 0u) {
        return ARITH_HEADER_BYTES;
    }

    if (model->total > WNC_FREQUENCY_TOTAL_MAX) {
        return 0;
    }

    BitWriter bw;
    const size_t body_cap = out_cap - ARITH_HEADER_BYTES;
    bw_init(&bw, out + ARITH_HEADER_BYTES, body_cap);

    const uint32_t MAX_VALUE = (1u << 16) - 1u;
    const uint32_t FIRST_QTR = (1u << 14);
    uint32_t low = 0;
    uint32_t high = MAX_VALUE;
    unsigned bits_to_follow = 0;

    for (int i = 0; i < length; i++) {
        if (encode_one_symbol(&bw, &low, &high, &bits_to_follow, model, (int)input[i]) != 0) {
            return 0;
        }
    }

    bits_to_follow++;
    if (low < FIRST_QTR) {
        if (bw_bit_plus_follow(&bw, 0u, &bits_to_follow) != 0) {
            return 0;
        }
    } else {
        if (bw_bit_plus_follow(&bw, 1u, &bits_to_follow) != 0) {
            return 0;
        }
    }

    if (bw_pad_to_byte(&bw) != 0) {
        return 0;
    }

    size_t body_bytes = bw.pos;
    while (body_bytes < 2u) {
        if (ARITH_HEADER_BYTES + body_bytes >= out_cap) {
            return 0;
        }
        out[ARITH_HEADER_BYTES + body_bytes] = 0;
        body_bytes++;
    }

    return ARITH_HEADER_BYTES + body_bytes;
}
