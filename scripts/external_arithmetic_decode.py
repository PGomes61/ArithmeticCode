#!/usr/bin/env python3
"""
Decoder independente (fora do arithmetic.c) para o formato gerado por arithmetic_compress.

Formato do ficheiro:
  - 4 bytes: uint32 little-endian = n (número de bytes da mensagem)
  - 512 bytes: 256 × uint16 little-endian = frequências do modelo (símbolo 0..255)
  - resto: corpo WNC 16 bits (mesma ordem de bits que BitReader no C)

Uso:
  python3 scripts/external_arithmetic_decode.py entrada.comp saida.bin
  python3 scripts/external_arithmetic_decode.py entrada.comp --verify original.bin

Requer gerar entrada.comp com o encoder em C, por exemplo:
  ./prog --dump-compressed original.txt saida.comp
"""

from __future__ import annotations

import argparse
import struct
import sys

SYMBOL_BYTE_COUNT = 256
ARITH_HEADER_BYTES = 4 + SYMBOL_BYTE_COUNT * 2
WNC_FREQUENCY_TOTAL_MAX = (1 << 14) - 1
MAX_BUFFER = 1 << 20


def u32(x: int) -> int:
    return x & 0xFFFFFFFF


def read_u32_le(data: bytes, off: int = 0) -> int:
    return struct.unpack_from("<I", data, off)[0]


def read_u16_le(data: bytes, off: int) -> int:
    return struct.unpack_from("<H", data, off)[0]


class BitReader:
    """Espelha br_init / br_read_bit em arithmetic.c (MSB primeiro dentro de cada byte)."""

    __slots__ = ("buf", "length", "pos", "nbits", "cur")

    def __init__(self, buf: bytes, start_pos: int) -> None:
        self.buf = buf
        self.length = len(buf)
        self.pos = start_pos
        self.nbits = 0
        self.cur = 0

    def read_bit(self) -> int:
        if self.nbits == 0:
            if self.pos >= self.length:
                return 0
            self.cur = self.buf[self.pos]
            self.pos += 1
            self.nbits = 8
        bit = (self.cur >> 7) & 1
        self.cur = (self.cur << 1) & 0xFF
        self.nbits -= 1
        return int(bit)


def model_from_counts(counts: list[int]) -> tuple[list[int], int]:
    """Cumulativas cum[0..256] e total, como arithmetic_model_from_counts."""
    cum = [0] * (SYMBOL_BYTE_COUNT + 1)
    for i in range(SYMBOL_BYTE_COUNT):
        cum[i + 1] = u32(cum[i] + (counts[i] & 0xFFFF))
    total = cum[SYMBOL_BYTE_COUNT]
    return cum, total


def decode_one_symbol(br: BitReader, low: int, high: int, value: int, cum: list[int], total: int) -> tuple[int, int, int, int]:
    """Espelha decode_one_symbol; devolve (low, high, value, symbol)."""
    max_value = (1 << 16) - 1
    half = 1 << 15
    first_qtr = half >> 1
    third_qtr = first_qtr * 3

    range_ = u32(high - low) + 1
    t = u32(value - low) + 1
    cum_x = (t * total - 1) // range_

    symbol = SYMBOL_BYTE_COUNT - 1
    for s in range(SYMBOL_BYTE_COUNT):
        if cum[s + 1] > cum_x:
            symbol = s
            break

    cum_low = cum[symbol]
    cum_high = cum[symbol + 1]
    high = u32(low + (range_ * cum_high) // total - 1)
    low = u32(low + (range_ * cum_low) // total)

    while True:
        if high < half:
            pass
        elif low >= half:
            value = u32(value - half)
            low = u32(low - half)
            high = u32(high - half)
        elif low >= first_qtr and high < third_qtr:
            value = u32(value - first_qtr)
            low = u32(low - first_qtr)
            high = u32(high - first_qtr)
        else:
            break
        low = u32(low << 1)
        high = u32((high << 1) | 1)
        value = u32((value << 1) | br.read_bit())

    return low, high, value, symbol


def decompress(data: bytes) -> bytes:
    if len(data) < ARITH_HEADER_BYTES:
        raise ValueError("dados demasiado curtos para cabecalho")

    n = read_u32_le(data, 0)
    if n > MAX_BUFFER:
        raise ValueError(f"n={n} excede MAX_BUFFER ({MAX_BUFFER})")

    counts: list[int] = []
    ssum = 0
    for i in range(SYMBOL_BYTE_COUNT):
        c = read_u16_le(data, 4 + i * 2)
        counts.append(c)
        ssum += c

    if n > 0 and (ssum == 0 or ssum > WNC_FREQUENCY_TOTAL_MAX):
        raise ValueError(f"modelo invalido: soma das frequencias = {ssum}")

    cum, total = model_from_counts(counts)

    if n == 0:
        return b""

    if len(data) < ARITH_HEADER_BYTES + 2:
        raise ValueError("ficheiro comprimido precisa de pelo menos 2 bytes de corpo quando n>0")

    br = BitReader(data, ARITH_HEADER_BYTES)
    max_value = (1 << 16) - 1
    value = 0
    for _ in range(16):
        value = u32((value << 1) | br.read_bit())

    low = 0
    high = max_value
    out = bytearray()
    for _ in range(n):
        low, high, value, sym = decode_one_symbol(br, low, high, value, cum, total)
        out.append(sym & 0xFF)

    return bytes(out)


def main() -> int:
    p = argparse.ArgumentParser(description="Decoder Python do formato arithmetic_compress (WNC).")
    p.add_argument("compressed", help="ficheiro .comp (cabeçalho + corpo)")
    p.add_argument("output", nargs="?", help="ficheiro de saída (bytes). Omitir com --verify.")
    p.add_argument("--verify", metavar="ORIGINAL", help="compara a saída com este ficheiro (deve ser igual)")
    args = p.parse_args()

    with open(args.compressed, "rb") as f:
        blob = f.read()

    try:
        decoded = decompress(blob)
    except ValueError as e:
        print(str(e), file=sys.stderr)
        return 1

    if args.verify:
        with open(args.verify, "rb") as f:
            orig = f.read()
        if decoded != orig:
            print("VERIFY FALHOU: bytes diferentes do original.", file=sys.stderr)
            print(f"  original: {len(orig)} bytes, decodificado: {len(decoded)} bytes", file=sys.stderr)
            m = min(len(orig), len(decoded))
            for i in range(m):
                if orig[i] != decoded[i]:
                    print(f"  primeiro diff no offset {i}", file=sys.stderr)
                    break
            return 2
        print("VERIFY OK: saida do decoder Python coincide com o original.")
        return 0

    if not args.output:
        p.error("indique ficheiro de saida ou use --verify")

    with open(args.output, "wb") as f:
        f.write(decoded)
    print(f"Escritos {len(decoded)} bytes em {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
