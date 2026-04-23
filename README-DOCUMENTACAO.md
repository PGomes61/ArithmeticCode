# Codificação aritmética — documentação do projeto

## Objetivo

Este programa ilustra a **codificação aritmética**: a mensagem é representada por um número (uma *tag*) no intervalo `[0, 1)`, obtido por sucessivos recortes proporcionais às probabilidades dos símbolos. O projeto serve para **Sistemas embarcados / algoritmos**: o foco é a lógica do intervalo e a tabela cumulativa, não um compressor de produção completo.

## Fluxo de dados

1. **`compute_frequencies`** — Conta cada byte (`0–255`) e divide pelo comprimento. A soma das frequências vale `1` (após renormaisação numérica).
2. **`build_cumulative_table`** — Monta `cum_prob[0…256]`: `cum_prob[k]` é o limite esquerdo da fatia do símbolo `k` no modelo em `[0, 1)`.
3. **`encode`** — Mantém um intervalo `[low, high)`. Para cada byte `s`, substitui esse intervalo pela fatia correspondente a `[cum_prob[s], cum_prob[s+1])` escalada dentro do intervalo atual. Devolve um ponto dentro do intervalo final (aqui: o próprio `low`).
4. **`decode`** — Com a **mesma** `cum_prob`, percorre `length` símbolos: em cada passo descobre em qual fatia o valor atual cai, escreve o byte e **renormaliza** o valor para o interior de `[0, 1)` relativamente à fatia escolhida.

## Como usar na prática

| Função | Papel |
|--------|--------|
| `compute_frequencies(input, length, freq)` | Preenche `freq[256]` com probabilidades empíricas. |
| `build_cumulative_table(freq)` | Prepara a tabela cumulativa global usada por `encode` / `decode`. |
| `encode(input, length)` | Devolve a tag (`long double`). |
| `decode(tag, output, length)` | Reconstrói `length` bytes em `output`. |

Para **strings C** terminadas em `\0`, use `run_test`. Para **qualquer sequência de bytes** (incluindo `'\0'` no meio), use `run_test_bytes(label, dados, comprimento)` ou chame diretamente as funções acima com `length` explícito.

## Limites e “qualquer string”

- **Comprimento máximo** do buffer: `MAX_BUFFER_SIZE` (`4096` bytes), definido em `arithmetic.h`.
- **Precisão em ponto flutuante**: quanto mais longa e **variada** a mensagem, mais estreito fica o intervalo final; `long double` eventualmente **não distingue** todas as sequências possíveis. Por isso o código de teste (`utils.c`) tenta, em ordem: uma passagem na mensagem inteira e, se a verificação falhar, **blocos contíguos** menores (24, 20, 16, … bytes). Nem todos os textos longos ficam reversíveis só com float — isso é esperado numa demo baseada em reais.
- Para **compressão real** e transmissão em canal binário, usa-se habitualmente **aritmética inteira**, **renormalização** do intervalo e emissão **bit a bit** (como em codecs profissionais). Uma única tag `long double` **não** substitui um fluxo de bits nem mede bem “economia de dados” frente ao texto original.

## Memória

Os buffers em `run_test_bytes` usam no máximo `MAX_BUFFER_SIZE` bytes na pilha para cópia e decodificação — adequado ao trabalho académico. Para um produto embutido, poder-se-ia ler **à medida** (streaming) ou limitar o pico conforme a RAM disponível.

## Relação com transmissão e “tamanho dos dados”

- O modelo (`freq` / cumulativa) tem de ser **idêntico** em codificador e descodificador. Se for deduzido da própria mensagem, em cenário real teria de ser **enviado** ou **convencionado** à partida (modelo fixo ou adaptativo).
- O valor impresso (“tag”) é útil para **aprender e validar** o algoritmo; não é, por si só, um formato de ficheiro compacto.

## Extensões sugeridas

- Codificação inteira fixa (`uint32_t`/`uint64_t`) com produtos em 128 bits para intervalos estáveis.
- Modelo adaptativo durante a leitura da mensagem.
- Saída binária + símbolo de fim ou comprimento em cabeçalho.

## Referência

[Wikipedia — Arithmetic coding](https://en.wikipedia.org/wiki/Arithmetic_coding)
