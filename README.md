# Codificação aritmética (WNC) — T1

**Autores:** Paulo Vinícius & Pedro Lucas  
**Data:** 2026  
**Contexto:** Trabalho de disciplina — Compressão de Dados / Sistemas Embarcados  
**Plataforma alvo:** Unix/macOS (POSIX), GCC/Clang, padrão C99, sem alocação dinâmica  
**Copyright:** Uso educacional. Redistribuição permitida com menção dos autores.

---

## Visão geral

Projeto em **C99** sem **recursão** e sem **alocação dinâmica** (`malloc`/`free`). O núcleo implementa codificação aritmética em **ponto fixo de 16 bits** com **renormalização** no estilo **Witten–Neal–Cleary (WNC)**: a saída é um **bitstream** (bytes) com cabeçalho, não um único `double` no intervalo `[0,1)` (que colapsa após ~16 símbolos por precisão finita).

**Guia passo a passo para principiantes:** [GUIA_LEIGO.md](GUIA_LEIGO.md)

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
gcc -std=c99 -Wall -Wextra -Werror -o prog main.c arithmetic.c utils.c demo.c
```

Ou, na raiz do projeto: `make`

- **Modo demonstração** (exemplos embutidos; cada um comprime em C e verifica **reversibilidade** com `scripts/external_arithmetic_decode.py` — executar na raiz do repositório com `python3` no PATH):

  ```bash
  ./prog
  ```

- **Modo ficheiro** (lê bytes, comprime, confirma com o decoder Python, imprime o conteúdo original no stdout):

  ```bash
  ./prog entrada.txt
  ```

- **Exportar só o binário comprimido** (para testar o Python manualmente):

  ```bash
  ./prog --dump-compressed entrada.txt saida.comp
  python3 scripts/external_arithmetic_decode.py saida.comp --verify entrada.txt
  ```

### Limites de tamanho

| Conceito | Valor por omissão | Notas |
|----------|-------------------|--------|
| **`MAX_BUFFER`** | **1 MiB** (`1u << 20`) | Tamanho máximo de uma mensagem por execução (leitura de ficheiro, buffers estáticos em `utils` / `demo` / `main`). |
| **Aumentar** | Até **RAM** e **compilador** aguentarem | Em `arithmetic.h` podes alterar `#define MAX_BUFFER`; `MAX_COMPRESSED_BYTES` cresce com `MAX_BUFFER` (~3× o tamanho da mensagem em BSS só em `utils.c`). Valores da ordem de **vários MiB** são típicos em PC; acima de **dezenas de MiB** o executável fica pesado e os testes lentos. |
| **Cabeçalho `n`** | `uint32_t` | Teoricamente até **2³²−1** bytes se `MAX_BUFFER` e RAM o permitirem. |
| **WNC** | `WNC_FREQUENCY_TOTAL_MAX` = **16383** | A **soma das frequências no modelo** não pode exceder isto. Mensagens **maiores** são suportadas: as contagens são **escalonadas** (`arithmetic_scale_counts_for_wnc`) para somar 16383 sem perder símbolos com peso zero. |

Se o ficheiro for **maior que `MAX_BUFFER`**, só os primeiros `MAX_BUFFER` bytes são lidos e aparece um aviso no *stderr*.

Compilar com outro limite (exemplo 512 KiB), sem editar o ficheiro:

```bash
gcc -std=c99 -Wall -Wextra -Werror -DMAX_BUFFER=524288 -o prog main.c arithmetic.c utils.c demo.c
```

(`arithmetic.h` usa `#ifndef MAX_BUFFER` para respeitar `-D`.)

---

## Estrutura de ficheiros

| Ficheiro | Papel |
|----------|--------|
| `arithmetic.h` / `arithmetic.c` | Modelo cumulativo, **compressão** WNC e bitstream (sem decoder em C). |
| `utils.h` / `utils.c` | `verify_reversible_with_python`, `export_compressed_binary`, `fill_demo_buffer_8k`, `run_from_file`. |
| `demo.h` / `demo.c` | `run_roundtrip_demo` (pré-visualização + verificação via Python). |
| `scripts/external_arithmetic_decode.py` | **Decoder de referência** (reversibilidade / prova externa). |
| `main.c` | CLI: ficheiro, `--dump-compressed`, ou demos sem argumentos. |
| `entrada.txt` | Exemplo de entrada para `./prog entrada.txt`. |

---

## Guia por módulos (resumo)

### `arithmetic.h` / `arithmetic.c`

- Cabeçalho LE (`write_u32_le` / `write_u16_le`), `BitWriter`, `encode_one_symbol`, histograma e escalonamento WNC, `arithmetic_compress`.
- **Não** contém decoder: a descompressão está em `scripts/external_arithmetic_decode.py`.

### `utils.c`

- **`verify_reversible_with_python`**: comprime em memória, grava temporários, executa o script Python com `--verify`.
- **`export_compressed_binary`**, **`run_from_file`**, **`fill_demo_buffer_8k`**.

### `demo.c` / `main.c`

- Demos chamam **`verify_reversible_with_python`** (precisam de `python3` e do caminho do script a partir do diretório atual).

Variáveis de ambiente opcionais: **`PYTHON`** (default `python3`), **`ARITH_DECODE_SCRIPT`** (default `scripts/external_arithmetic_decode.py`).

---

## Restrições do enunciado (checklist)

- Pelo menos **dois laços aninhados**: por exemplo laço sobre símbolos da mensagem × laço/procura na tabela cumulativa (no encoder ou no decoder Python).
- **Sem recursão** no compressor (C).
- **Sem alocação dinâmica** no teu código do algoritmo em C (só arrays estáticos e pilha com tamanhos fixos). A prova de reversibilidade usa **Python** à parte.

---

## Referência rápida WNC

- **E1**: intervalo inteiro na metade inferior → emite `0` (com bits pendentes) e escala.
- **E2**: intervalo inteiro na metade superior → emite `1`, subtrai metade, escala.
- **E3**: intervalo “à volta” do meio → incrementa contador de bits a seguir sem emitir ainda; depois escala; resolve ambiguidade quando metades coincidem em transmissão.

Para uma explicação didática mais ampla (vídeo), podes referenciar materiais sobre codificação aritmética aplicada à compressão de ficheiros grandes (o conceito é o mesmo: **fluxo de bits**, não um único real).
