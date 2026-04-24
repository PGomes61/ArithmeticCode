#include "arithmetic.h"
#include <string.h>

#define CUM_WORK_WORDS 2048
static uint32_t g_cumulative_workspace[CUM_WORK_WORDS];

static void copy_counts_to_workspace(const uint16_t counts[SYMBOL_BYTE_COUNT]) {
    for (int i = 0; i < SYMBOL_BYTE_COUNT; i++) {
        g_cumulative_workspace[i] = counts[i];
    }
}

static void write_u32_le(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static uint32_t read_u32_le(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void write_u16_le(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static uint16_t read_u16_le(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

typedef struct {
    uint8_t *buf;
    size_t cap;
    size_t pos;
    unsigned nbits;
    unsigned cur;
} BitWriter;

static void bw_init(BitWriter *w, uint8_t *buf, size_t cap) {
    w->buf = buf;
    w->cap = cap;
    w->pos = 0;
    w->nbits = 0;
    w->cur = 0;
}

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

static int bw_pad_to_byte(BitWriter *w) {
    while (w->nbits > 0u) {
        if (bw_flush_bit(w, 0u) != 0) {
            return -1;
        }
    }
    return 0;
}

typedef struct {
    const uint8_t *buf;
    size_t len;
    size_t pos;
    unsigned nbits;
    unsigned cur;
} BitReader;

static void br_init(BitReader *r, const uint8_t *buf, size_t len, size_t start_pos) {
    r->buf = buf;
    r->len = len;
    r->pos = start_pos;
    r->nbits = 0;
    r->cur = 0;
}

static int br_read_bit(BitReader *r) {
    if (r->nbits == 0u) {
        if (r->pos >= r->len) {
            return 0;
        }
        r->cur = r->buf[r->pos++];
        r->nbits = 8u;
    }
    int bit = (int)((r->cur >> 7u) & 1u);
    r->cur = (unsigned)(r->cur << 1u);
    r->nbits--;
    return bit;
}

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

static int decode_one_symbol(BitReader *br, uint32_t *low, uint32_t *high, uint32_t *value,
                             const ArithmeticModel *model, int *out_symbol) {
    const uint32_t MAX_VALUE = (1u << 16) - 1u;
    const uint32_t HALF = 1u << 15u;
    const uint32_t FIRST_QTR = HALF >> 1u;
    const uint32_t THIRD_QTR = FIRST_QTR * 3u;
    const uint32_t total = model->total;
    const uint64_t range = (uint64_t)(*high - *low) + 1u;
    const uint64_t cum_x = (((uint64_t)(*value - *low + 1u) * (uint64_t)total - 1u) / range);

    int symbol = SYMBOL_BYTE_COUNT - 1;
    for (int s = 0; s < SYMBOL_BYTE_COUNT; s++) {
        if (model->cum[s + 1] > cum_x) {
            symbol = s;
            break;
        }
    }

    const uint32_t cum_low = model->cum[symbol];
    const uint32_t cum_high = model->cum[symbol + 1];
    *high = (uint32_t)(*low + (uint32_t)((range * (uint64_t)cum_high) / (uint64_t)total) - 1u);
    *low = (uint32_t)(*low + (uint32_t)((range * (uint64_t)cum_low) / (uint64_t)total));

    for (;;) {
        if (*high < HALF) {
        } else if (*low >= HALF) {
            *value -= HALF;
            *low -= HALF;
            *high -= HALF;
        } else if (*low >= FIRST_QTR && *high < THIRD_QTR) {
            *value -= FIRST_QTR;
            *low -= FIRST_QTR;
            *high -= FIRST_QTR;
        } else {
            break;
        }
        *low = (*low << 1u) & MAX_VALUE;
        *high = ((*high << 1u) | 1u) & MAX_VALUE;
        *value = ((*value << 1u) | (uint32_t)(unsigned)br_read_bit(br)) & MAX_VALUE;
    }

    *out_symbol = symbol;
    return 0;
}

void arithmetic_count_raw(const uint8_t *data, int length, uint32_t raw_counts[SYMBOL_BYTE_COUNT]) {
    for (int i = 0; i < SYMBOL_BYTE_COUNT; i++) {
        raw_counts[i] = 0u;
    }
    for (int i = 0; i < length; i++) {
        raw_counts[data[i]]++;
    }
}

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

void arithmetic_count_symbols(const uint8_t *data, int length, uint16_t counts[SYMBOL_BYTE_COUNT]) {
    uint32_t raw[SYMBOL_BYTE_COUNT];
    arithmetic_count_raw(data, length, raw);
    arithmetic_scale_counts_for_wnc(raw, length, counts);
}

void arithmetic_build_model_from_data(const uint8_t *data, int length, ArithmeticModel *model) {
    uint16_t scaled[SYMBOL_BYTE_COUNT];
    arithmetic_count_symbols(data, length, scaled);
    arithmetic_model_from_counts(scaled, model);
}

void arithmetic_model_from_counts(const uint16_t counts[SYMBOL_BYTE_COUNT], ArithmeticModel *model) {
    copy_counts_to_workspace(counts);
    model->cum[0] = 0u;
    for (int i = 0; i < SYMBOL_BYTE_COUNT; i++) {
        model->cum[i + 1] = model->cum[i] + g_cumulative_workspace[i];
    }
    model->total = model->cum[SYMBOL_BYTE_COUNT];
}

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

int arithmetic_decompress(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_cap,
                          uint32_t *out_length) {
    if (in_len < ARITH_HEADER_BYTES || out_length == NULL) {
        return -1;
    }

    const uint32_t n = read_u32_le(in);
    if (n > MAX_BUFFER || n > out_cap) {
        return -1;
    }

    uint16_t counts[SYMBOL_BYTE_COUNT];
    uint32_t sum = 0;
    for (int i = 0; i < SYMBOL_BYTE_COUNT; i++) {
        counts[i] = read_u16_le(in + sizeof(uint32_t) + (size_t)i * sizeof(uint16_t));
        sum += counts[i];
    }
    if (n > 0u && (sum == 0u || sum > WNC_FREQUENCY_TOTAL_MAX)) {
        return -1;
    }

    ArithmeticModel model;
    arithmetic_model_from_counts(counts, &model);
    *out_length = n;

    if (n == 0) {
        return 0;
    }

    if (in_len < ARITH_HEADER_BYTES + 2u) {
        return -1;
    }

    BitReader br;
    br_init(&br, in, in_len, ARITH_HEADER_BYTES);

    const uint32_t MAX_VALUE = (1u << 16) - 1u;
    uint32_t value = 0;
    for (int b = 0; b < 16; b++) {
        value = (value << 1u) | (uint32_t)(unsigned)br_read_bit(&br);
    }

    uint32_t low = 0;
    uint32_t high = MAX_VALUE;

    for (uint32_t i = 0; i < n; i++) {
        int sym = 0;
        decode_one_symbol(&br, &low, &high, &value, &model, &sym);
        out[i] = (uint8_t)sym;
    }

    return 0;
}
