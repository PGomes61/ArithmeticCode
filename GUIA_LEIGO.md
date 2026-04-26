# Guia do projeto — explicação para quem está a começar

Este documento explica **o que o programa faz**, **palavra por palavra** quando aparecem termos como “símbolo”, e **como o código se organiza**. Não se assume que dominas C; quando um conceito de programação aparece, há uma nota curta.

Há também um [README.md](README.md) com compilação e limites técnicos.

---

## Índice

1. [O que este programa faz (uma frase)](#1-o-que-este-programa-faz-uma-frase)
2. [Conceitos básicos antes do código](#2-conceitos-básicos-antes-do-código)
3. [O que é “símbolo” (muito importante)](#3-o-que-é-símbolo-muito-importante)
4. [Lista de ficheiros e quem faz o quê](#4-lista-de-ficheiros-e-quem-faz-o-quê)
5. [O caminho completo dos dados (do zero ao fim)](#5-o-caminho-completo-dos-dados-do-zero-ao-fim)
6. [O ficheiro `main.c` (entrada do programa)](#6-o-ficheiro-mainc-entrada-do-programa)
7. [O ficheiro `utils.c` (cola e ficheiros)](#7-o-ficheiro-utilsc-cola-e-ficheiros)
8. [O ficheiro `arithmetic.c` (o “motor” matemático)](#8-o-ficheiro-arithmeticc-o-motor-matemático)
9. [O ficheiro `demo.c` (só para mostrar na consola)](#9-o-ficheiro-democ-só-para-mostrar-na-consola)
10. [Prova de reversibilidade (decoder em Python)](#10-prova-de-reversibilidade-decoder-em-python)
11. [Resumo para apresentação](#11-resumo-para-apresentação)

---

## 1. O que este programa faz (uma frase)

**Lê uma sequência de bytes (por exemplo texto), calcula estatísticas, transforma num formato comprimido (bits em memória), e consegue voltar a obter exatamente os mesmos bytes.**  
Isso chama-se **compressão sem perdas** (lossless): o original e o recuperado são **iguais byte a byte**.

---

## 2. Conceitos básicos antes do código

### Byte

Um **byte** é um número inteiro de **0 a 255**. É a unidade usual de memória.  
Uma letra em ASCII (muitas vezes) ocupa **1 byte** — por exemplo `A` é o número **65**.

### `uint8_t`

Em C, `uint8_t` significa: “inteiro **sem** sinal de **8 bits**”, ou seja, **0 a 255**.  
É o mesmo que “um byte”.

### Array (vetor)

Um **array** é uma fila de casas numeradas: `buffer[0]`, `buffer[1]`, …  
No projeto, `input[i]` é o **i-ésimo byte** da mensagem (o primeiro é `input[0]`).

### Ponteiro (`const uint8_t *data`)

Um **ponteiro** é “o endereço onde começa um bloco de memória”.  
`const uint8_t *data` significa: “aponta para bytes que **não vamos alterar** nesta função”.

### `int length`

**Quantos bytes** vamos usar a partir do início do array (o “comprimento da mensagem”).

### `static` (variável global no `.c`)

Uma variável `static` fora de qualquer função vive **durante todo o programa** e **só existe uma cópia**.  
Aqui serve para ter **buffers muito grandes** sem os meter na **pilha** de uma função (o que poderia “partir” o programa).

### `malloc` / `free`

Este projeto **não usa** alocação dinâmica (não há `malloc`). Tudo é memória **fixa** (arrays com tamanho definido em compile-time ou buffers `static`).

---

## 3. O que é “símbolo” (muito importante)

Na **codificação aritmética**, a palavra **“símbolo”** quase sempre significa:

> **Uma possível leitura de “um passo” do algoritmo — aqui, um byte (0–255).**

Ou seja:

- O **alfabeto** tem **256 símbolos**: os bytes `0`, `1`, …, `255`.
- A **mensagem** é uma sequência de símbolos: por exemplo `B` (66), `A` (65), `N` (78) para `"BAN"`.
- Quando o código diz **“codifica um símbolo”**, quer dizer: **“processa o próximo byte da mensagem”** (`input[i]`).

**Não** é um conceito místico: é só **“qual é o próximo valor de 0 a 255 que estamos a tratar neste momento”**.

---

## 4. Lista de ficheiros e quem faz o quê

| Ficheiro | Papel em linguagem simples |
|----------|----------------------------|
| `main.c` | Começa o programa. Escolhe: **ficheiro** ou **demonstração**. |
| `utils.c` | Junta tudo: **modelo → comprimir → verificar com Python**. Lê ficheiros. |
| `arithmetic.c` | **Matemática**: contagens, modelo, WNC, **compressão** (só encoder em C). |
| `arithmetic.h` | Números máximos e nomes das funções do motor. |
| `demo.c` | **Imprime** na consola (pré-visualização) e chama o mesmo pipeline. |
| `scripts/external_arithmetic_decode.py` | **Decoder de referência** (prova de reversibilidade). |

---

## 5. O caminho completo dos dados (do zero ao fim)

Imagina que tens uma mensagem de bytes na memória.

```
Passo A   Lês / tens os bytes (texto, ficheiro, demo)
    ↓
Passo B   Contas quantas vezes aparece cada valor 0..255
    ↓
Passo C   Se a mensagem for gigante, "esmagas" as contagens para um limite (WNC)
    ↓
Passo D   Constróis a tabela cumulativa (modelo de probabilidades)
    ↓
Passo E   Comprimes: percorres a mensagem byte a byte, estreitas um intervalo inteiro,
          escreves bits quando precisas (renormalização WNC)
    ↓
Passo F   Ficas com um "pacote": cabeçalho (n + contagens) + corpo (bits)
    ↓
Passo G   Descomprimes: lês o cabeçalho, reconstróis o mesmo modelo,
          lês bits, recuperas os n bytes
    ↓
Passo H   O **decoder em Python** lê o mesmo pacote e recupera os n bytes; compara com o original
```

Isto está concentrado na função **`verify_reversible_with_python`** em `utils.c` e no script **`scripts/external_arithmetic_decode.py`**.

---

## 6. O ficheiro `main.c` (entrada do programa)

### O que é `main`?

Todo o programa em C **começa** na função `main`.

### Argumentos `int argc, char **argv`

Quando corres no terminal:

- `./prog` → `argc` é 1 (só o nome do programa).
- `./prog entrada.txt` → `argc` é 2, e `argv[1]` é a string `"entrada.txt"` (o caminho do ficheiro).

### O que o `main` faz neste projeto?

1. **Se** passaste um nome de ficheiro (`argc >= 2`): chama `run_from_file(argv[1])` — lê o ficheiro, comprime, verifica com o decoder Python, imprime o original.
2. **Senão**: imprime uma mensagem de “como usar” e chama `run_builtin_demo()`:
   - corre várias demos (`run_roundtrip_demo` com exemplos curtos e um buffer grande), cada uma chamando o Python para confirmar reversibilidade.

### `static uint8_t s_demo_large_buffer[MAX_BUFFER]`

É um array **global** (uma única cópia) com tamanho `MAX_BUFFER` bytes.  
Guarda o texto grande da demonstração sem usar a pilha da função.

---

## 7. O ficheiro `utils.c` (cola e ficheiros)

### `prepare_counts`

Chama funções do `arithmetic.c` para obter as **contagens já tratadas** (brutas + escala). Serve se outra parte do código precisar só das contagens.

### `verify_reversible_with_python` — compressão em C e prova externa

Ordem interna:

1. **Valida** ponteiros e tamanhos (se algo for inválido, devolve código de erro negativo).
2. **`arithmetic_build_model_from_data`**: a partir dos bytes, constrói `ArithmeticModel` (cumulativos).
3. **`arithmetic_compress`**: escreve o pacote comprimido num buffer grande **`s_codec_compressed`** (variável `static` no topo do ficheiro — existe uma só vez no programa).
4. Grava temporários em `/tmp`, executa **`python3 scripts/external_arithmetic_decode.py … --verify`** com o original; se o script sair com código 0, a reversibilidade está confirmada **fora** do código C.

**Porque `s_codec_compressed` é `static`?**  
Porque o pacote comprimido pode ser **grande** (ordem de megabytes no pior caso). Meter isso na pilha de uma função pequena poderia **ultrapassar o limite da stack**.

### `run_from_file`

1. `fopen` abre o ficheiro em modo binário (`"rb"`).
2. `fread` copia bytes para **`s_file_input`** (no máximo `MAX_BUFFER`).
3. Se ainda havia bytes no ficheiro depois disso, imprime um **aviso** (só leu uma parte).
4. Chama `verify_reversible_with_python` e depois `fwrite` do **conteúdo original** (igual ao que o Python recupera).

### `fill_demo_buffer_8k`

Copia uma frase repetida até encher o buffer até `MAX_BUFFER` bytes — só para ter um exemplo “muito grande” na demo.

---

## 8. O ficheiro `arithmetic.c` (o “motor” matemático)

Este é o ficheiro mais longo. Divide-se em **blocos**.

### Bloco A — Escrever números em bytes (`write_u32_le`, `write_u16_le`, …)

O cabeçalho do pacote guarda números como sequências de bytes (**little-endian**): o byte menos significativo primeiro.  
Isto permite que o mesmo ficheiro binário seja lido da mesma forma em muitas máquinas.

### Bloco B — `BitWriter` (escrever bits num array de bytes)

Computadores gostam de bytes (8 bits). O algoritmo precisa de escrever **um bit de cada vez**.

- `bw_flush_bit`: mete o bit num acumulador `cur`; quando chega a 8 bits, escreve um byte em `buf[pos]` e avança `pos`.

### Bloco C — `encode_one_symbol`

**Entrada:** o modelo (`ArithmeticModel`), o símbolo atual (um **byte** 0–255), e o estado `low`/`high` do intervalo WNC.

**O que faz:**

1. Calcula a largura do intervalo: `range = high - low + 1`.
2. Usa `cum[symbol]` e `cum[symbol+1]` para saber **que fatia** do intervalo pertence a este símbolo.
3. Atualiza `low` e `high` para essa fatia (divisões inteiras; usa `uint64_t` no meio para não transbordar).
4. Entra num ciclo **infinito** até não precisar de mais renormalização **neste momento**:
   - **E1**: se o intervalo está **toda** na metade inferior → emite bit **0** (com `bit_plus_follow`), depois desloca/amplia.
   - **E2**: se está **toda** na metade superior → emite **1**, ajusta, desloca.
   - **E3**: se está “no meio” (underflow) → **não** escolhe 0/1 logo; incrementa `bits_to_follow` e aplica a transformação do meio.
   - Se nenhuma se aplica, **sai** do ciclo.

**`bit_plus_follow`:** depois de escrever um bit “principal”, escreve bits complementares pendentes (ligado ao problema de *carry* do WNC).

A **descompressão** (espelho deste bloco, com leitura de bits) está no ficheiro **Python** `scripts/external_arithmetic_decode.py`, não em `arithmetic.c`.

### Bloco D — Contagens

- **`arithmetic_count_raw`**: para cada posição `i` da mensagem, faz `raw_counts[data[i]]++`. No fim, `raw_counts[k]` diz “quantas vezes o byte `k` apareceu”.

- **`arithmetic_scale_counts_for_wnc`**: se a mensagem for **maior** que o limite WNC (`16383`), **reduz** as contagens proporcionalmente para a soma ser `16383`, sem pôr a zero um símbolo que apareceu (mínimo 1). Se a mensagem for **curta**, copia as contagens sem alterar.

- **`arithmetic_count_symbols`**: chama as duas acima (é o atalho “conta + escala”).

### Bloco E — `arithmetic_model_from_counts`

Constrói `cum[0..256]`:

- `cum[0] = 0`
- `cum[i+1] = cum[i] + counts[i]`

O `total` do modelo é `cum[256]` (soma das contagens **depois** de escaladas).

### Bloco F — `arithmetic_compress`

1. Escreve o cabeçalho: `n` (comprimento real da mensagem) + 256 contagens em `uint16_t`.
2. Se `n == 0`, termina (só cabeçalho).
3. Inicializa `low`, `high`, `BitWriter`.
4. **Para cada** `i` de `0` a `n-1`**: `encode_one_symbol` com `input[i]` — aqui cada **`input[i]`** é um **símbolo** (um byte).
5. Terminação (`done_encoding`): mais um pouco de lógica WNC para fechar o bitstream.
6. Alinha a bits num limite de byte; garante pelo menos **2 bytes** de corpo para o decoder (Python) inicializar `value`.

---

## 9. O ficheiro `demo.c` (só para mostrar na consola)

Não faz compressão “nova”. Só:

- imprime um título e uma **pré-visualização** (texto ou hex);
- chama `verify_reversible_with_python`;
- imprime o tamanho do pacote e se deu certo.

Serve para **apresentações humanas**. O algoritmo em si está em `arithmetic.c`.

---

## 10. Prova de reversibilidade (decoder em Python)

Não há framework Unity: a **prova** é **independente** do C.

- O programa C **comprime** e, nas demos / modo ficheiro, chama o script **`scripts/external_arithmetic_decode.py`** com `--verify` sobre ficheiros temporários.
- Podes repetir à mão: `./prog --dump-compressed original.txt out.comp` e `python3 scripts/external_arithmetic_decode.py out.comp --verify original.txt`.

Requer **`python3`** no PATH e correr a partir da **raiz do repositório** (ou definir `ARITH_DECODE_SCRIPT`).

---

## 11. Resumo para apresentação

- **Símbolo** = **um byte possível (0–255)**; processar a mensagem = processar **uma sequência de símbolos**.
- **Renormalizar** = **escrever bits** quando já tens a certeza deles e **reabrir margem** no intervalo inteiro, para não ficares sem precisão.
- **Escalonamento** = quando a mensagem é **enorme**, o modelo não pode somar frequências infinitas; **reduz-se** a tabela mantendo o decoder compatível.
- **`verify_reversible_with_python` / script Python** = prova que o **formato** é reversível (decoder fora do C).

---

## Glossário rápido

| Termo | Significado aqui |
|--------|------------------|
| Símbolo | Um byte (0–255); cada passo do laço principal trata **um** símbolo da mensagem. |
| Modelo | Tabela de frequências / cumulativos usada pelo encoder e pelo decoder. |
| WNC | Witten–Neal–Cleary: regras para emitir bits e evitar perda de precisão. |
| Round-trip | Comprimir e descomprimir e verificar que o resultado é **idêntico** ao original. |
| Cabeçalho | Primeiros bytes do pacote com `n` e contagens. |
| Corpo | Bits da codificação aritmética depois do cabeçalho. |

---

*Documento gerado para apoio ao estudo e à apresentação do projeto ArithmeticCode.*
