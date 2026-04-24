# Codificação aritmética (WNC) — T1

Projeto em **C99** sem **recursão** e sem **alocação dinâmica** (`malloc`/`free`). O núcleo implementa codificação aritmética em **ponto fixo de 16 bits** com **renormalização** no estilo **Witten–Neal–Cleary (WNC)**: a saída é um **bitstream** (bytes) com cabeçalho, não um único `double` no intervalo `[0,1)` (que colapsa após ~16 símbolos por precisão finita).

---

## Ponto principal do código

| Ideia | O que significa |
|--------|-------------------|
| **Problema do `double`** | Encolher `[low, high]` em ponto flutuante sem emitir bits faz o intervalo ficar menor que o ULP da mantissa; o decoder deixa de distinguir símbolos. |
| **Solução** | Manter `low` e `high` como inteiros em `[0, 65535]` e, sempre que o intervalo cai na metade inferior, na metade superior ou no “meio” (E3), **renormalizar**: emitir bits para o ficheiro e **esticar** o intervalo (multiplicar por 2, etc.), como em codecs reais. |
| **Formato do pacote** | **516 bytes de cabeçalho**: 4 bytes = comprimento `n` (LE) + 512 bytes = 256 contagens `uint16_t` (LE). Depois vem o **corpo AC** em bits (MSB primeiro em cada byte). |
| **Modelo** | **Estático**: contagens calculadas sobre a mensagem inteira antes de codificar; o decoder reconstrói a mesma tabela a partir do cabeçalho. |

---

## Como compilar e executar

```bash
gcc -std=c99 -Wall -Wextra -Werror -o prog main.c arithmetic.c utils.c demo.c tests.c unity.c
```

Ou, na raiz do projeto: `make`

- **Modo demonstração** (testes Unity + exemplos embutidos):

  ```bash
  ./prog
  ```

- **Modo ficheiro** (lê bytes do ficheiro, comprime, descomprime, imprime o conteúdo recuperado no stdout):

  ```bash
  ./prog entrada.txt
  ```

### Limites de tamanho

| Conceito | Valor por omissão | Notas |
|----------|-------------------|--------|
| **`MAX_BUFFER`** | **1 MiB** (`1u << 20`) | Tamanho máximo de uma mensagem por execução (leitura de ficheiro, buffers estáticos em `utils` / `demo` / `tests` / `main`). |
| **Aumentar** | Até **RAM** e **compilador** aguentarem | Em `arithmetic.h` podes alterar `#define MAX_BUFFER`; `MAX_COMPRESSED_BYTES` cresce com `MAX_BUFFER` (~3× o tamanho da mensagem em BSS só em `utils.c`). Valores da ordem de **vários MiB** são típicos em PC; acima de **dezenas de MiB** o executável fica pesado e os testes lentos. |
| **Cabeçalho `n`** | `uint32_t` | Teoricamente até **2³²−1** bytes se `MAX_BUFFER` e RAM o permitirem. |
| **WNC** | `WNC_FREQUENCY_TOTAL_MAX` = **16383** | A **soma das frequências no modelo** não pode exceder isto. Mensagens **maiores** são suportadas: as contagens são **escalonadas** (`arithmetic_scale_counts_for_wnc`) para somar 16383 sem perder símbolos com peso zero. |

Se o ficheiro for **maior que `MAX_BUFFER`**, só os primeiros `MAX_BUFFER` bytes são lidos e aparece um aviso no *stderr*.

Compilar com outro limite (exemplo 512 KiB), sem editar o ficheiro:

```bash
gcc -std=c99 -Wall -Wextra -Werror -DMAX_BUFFER=524288 -o prog main.c arithmetic.c utils.c demo.c tests.c unity.c
```

(`arithmetic.h` usa `#ifndef MAX_BUFFER` para respeitar `-D`.)

---

## Estrutura de ficheiros

| Ficheiro | Papel |
|----------|--------|
| `arithmetic.h` / `arithmetic.c` | Modelo cumulativo, compressão e descompressão WNC, bitstream. |
| `utils.h` / `utils.c` | `codec_roundtrip` (único pipeline), contagens, buffer demo `fill_demo_buffer_8k`, `run_from_file`. |
| `demo.h` / `demo.c` | Demonstrações na consola: `run_roundtrip_demo` (pré-visualização + `codec_roundtrip`). |
| `tests.h` / `tests.c` | Testes **Unity** (round-trip via `codec_roundtrip`). |
| `main.c` | Entrada do programa: ficheiro **ou** sequência de `run_roundtrip_demo` (todos os exemplos iguais). |
| `unity.c` / `unity.h` / `unity_internals.h` | Framework de testes (externo ao algoritmo). |
| `entrada.txt` | Exemplo de entrada para `./prog entrada.txt`. |

---

## Guia “linha a linha” (por blocos)

### `arithmetic.h`

- **Include guard** (`#ifndef` … `#define` … `#endif`): evita inclusão dupla.
- **`MAX_BUFFER`**: tamanho máximo da mensagem em bytes (buffers estáticos na pilha).
- **`SYMBOL_BYTE_COUNT`**: 256 símbolos (um byte).
- **`ARITH_HEADER_BYTES`**: tamanho fixo do cabeçalho em bytes (`4 + 256*2`).
- **`MAX_COMPRESSED_BYTES`**: teto seguro para o buffer comprimido (cabeçalho + corpo + margem).
- **`ArithmeticModel`**: `cum[257]` — `cum[i]` é a soma das contagens dos símbolos `0` a `i-1`; `total = cum[256]` (= soma de todas as contagens = `n` no modelo empírico).
- **Protótipos** das funções públicas expostas ao resto do projeto.

### `arithmetic.c` — mapa de linhas (ficheiro atual)

| Linhas | Conteúdo |
|--------|-----------|
| 1–2 | `#include` do cabeçalho e `string.h`. |
| 4–9 | `CUM_WORK_WORDS`, array estático `g_cumulative_workspace` (~8 KB). |
| 11–15 | `copy_counts_to_workspace`. |
| 17–35 | `write_u32_le`, `read_u32_le`, `write_u16_le`, `read_u16_le` (cabeçalho LE). |
| 37–43 | Struct `BitWriter` e campos. |
| 45–51 | `bw_init`. |
| 53–65 | `bw_flush_bit`. |
| 67–78 | `bw_bit_plus_follow`. |
| 80–87 | `bw_pad_to_byte`. |
| 89–95 | Struct `BitReader`. |
| 97–103 | `br_init`. |
| 105–117 | `br_read_bit`. |
| 119–155 | `encode_one_symbol` (subintervalo + E1/E2/E3 + emissão de bits). |
| 157–201 | `decode_one_symbol` (resolve símbolo + renormalização espelhada). |
| 203–210 | `arithmetic_count_symbols`. |
| 212–219 | `arithmetic_model_from_counts`. |
| 221–288 | `arithmetic_compress` (cabeçalho, corpo AC, flush, padding mínimo 2 bytes). |
| 290–337 | `arithmetic_decompress` (validação, cabeçalho, 16 bits iniciais, laço de símbolos). |

### `utils.c`

- **`prepare_counts`**: obtém contagens **já escaladas** para o WNC (`arithmetic_count_symbols` = raw + escala).
- **`arithmetic_count_raw` / `arithmetic_scale_counts_for_wnc` / `arithmetic_build_model_from_data`**: em `arithmetic.c` — contagens em `uint32_t`, escalonamento quando `n > 16383`, construção do modelo.
- **`codec_roundtrip`**: **único caminho** de codificação/decodificação usado por Unity, `demo.c`, e `run_from_file`.
- **`fill_demo_buffer_8k`**: só preenche bytes de exemplo até `MAX_BUFFER` (entrada para a demo; não comprime por si).
- **`run_from_file`**: lê o ficheiro e chama `codec_roundtrip`; imprime o resultado com `fwrite` (adequado a dados binários).

### `demo.c`

- **`run_roundtrip_demo`**: imprime rótulo e pré-visualização (texto legível ou hex), chama **`codec_roundtrip`**, imprime tamanho do pacote e status. O `main.c` usa **apenas** esta função para todos os exemplos (binário, strings, buffer grande).

### `main.c`

- **`run_builtin_demo`**: cabeçalho, `run_unity_tests`, depois várias chamadas a **`run_roundtrip_demo`** (incluindo o buffer de `MAX_BUFFER` bytes após `fill_demo_buffer_8k`).
- **`main`**: com `argv[1]` chama `run_from_file`; sem argumentos corre `run_builtin_demo`.

### `tests.c`

- **`assert_round_trip`**: chama `codec_roundtrip` (mesmo algoritmo que o resto do programa).
- **Vários `test_*`**: casos vazio, 1 byte, 16 bytes, frase, redundância, padrão binário, 1 KB, buffer máximo.
- **`run_unity_tests`**: regista e executa os testes no Unity.

---

## Restrições do enunciado (checklist)

- Pelo menos **dois laços aninhados**: por exemplo laço sobre símbolos da mensagem × laço/procura na tabela cumulativa no decoder.
- **Sem recursão** no compressor/descompressor.
- **Sem alocação dinâmica** no teu código do algoritmo (só arrays estáticos e pilha com tamanhos fixos). O **Unity** é biblioteca de testes separada.

---

## Referência rápida WNC

- **E1**: intervalo inteiro na metade inferior → emite `0` (com bits pendentes) e escala.
- **E2**: intervalo inteiro na metade superior → emite `1`, subtrai metade, escala.
- **E3**: intervalo “à volta” do meio → incrementa contador de bits a seguir sem emitir ainda; depois escala; resolve ambiguidade quando metades coincidem em transmissão.

Para uma explicação didática mais ampla (vídeo), podes referenciar materiais sobre codificação aritmética aplicada à compressão de ficheiros grandes (o conceito é o mesmo: **fluxo de bits**, não um único real).
