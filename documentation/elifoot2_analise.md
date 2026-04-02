# Elifoot II — Análise Comportamental Detalhada
### Referência para Port para Sega Genesis (SGDK 1.70)

> Documento gerado por engenharia reversa do binário `elifoot.exe` (MS-DOS, MZ executable, ~141 KB),
> análise dos arquivos de dados `EQUIPAS.EF2` e `TREINAD.EF2`, e conhecimento acumulado do jogo.
> **v2 — revisado com análise binária aprofundada e correções factuais.**
> Destina-se a guiar um **vibe port** fiel para Sega Genesis com SGDK 1.70.

---

## Índice

1. [Visão Geral do Jogo](#1-visão-geral-do-jogo)
2. [Estrutura de Dados — Arquivos de Entrada](#2-estrutura-de-dados--arquivos-de-entrada)
3. [Fluxo de Jogo — Estados e Transições](#3-fluxo-de-jogo--estados-e-transições)
4. [Sistema de Ligas e Competições](#4-sistema-de-ligas-e-competições)
5. [Equipes, Jogadores e Posições](#5-equipes-jogadores-e-posições)
6. [Mecânica de Força e Atributos](#6-mecânica-de-força-e-atributos)
7. [Sistema Tático — Formações](#7-sistema-tático--formações)
8. [Simulação de Partidas](#8-simulação-de-partidas)
9. [Sistema Econômico](#9-sistema-econômico)
10. [Transferências e Mercado](#10-transferências-e-mercado)
11. [Treinadores](#11-treinadores)
12. [Estádios](#12-estádios)
13. [Interface e Controles](#13-interface-e-controles)
14. [Sistema de Saves](#14-sistema-de-saves)
15. [Palmares e Histórico](#15-palmares-e-histórico)
16. [Anomalias e Dados Sujos nos Arquivos Fonte](#16-anomalias-e-dados-sujos-nos-arquivos-fonte)
17. [Recomendações para o Port Genesis](#17-recomendações-para-o-port-genesis)

---

## 1. Visão Geral do Jogo

Elifoot II é um jogo de gerenciamento de futebol para MS-DOS, desenvolvido por Filipe Luz (Portugal), amplamente popular nos países lusófonos durante os anos 1990. O jogador assume o papel de técnico/diretor de uma equipe de futebol e toma decisões sobre táticas, escalações, finanças e transferências ao longo de uma ou mais temporadas.

O jogo roda inteiramente em modo texto/ASCII no DOS, usando a BIOS de vídeo em modo 80×25 colunas com cores ANSI. Não há sprites nem gráficos de mapa de bits — toda a apresentação é feita com caracteres de caixa-alta (IBM CP437/CP850) e cores de 4 bits (16 cores de texto CGA).

**Características centrais do design:**
- Gerenciamento assíncrono: o jogador toma decisões antes de cada rodada e observa os resultados simulados.
- Sem controle em tempo real das partidas — a simulação é determinística com componente aleatória.
- Múltiplas divisões e copa paralela.
- Economia interna baseada em salários, bilheteria e prêmios.
- Transferências por negociação direta ou leilão de salário.

**Compilador/runtime:** O executável é Turbo Pascal (identificado pelo padrão de string length-prefixed, calls `far` com `9A xx xx xx xx`, e a sentinela `[EOF]` no TREINAD.EF2). Isso é relevante para entender o modelo de dados: strings são Pascal strings (byte de comprimento + chars), inteiros são 16 bits por default, e o RNG é provavelmente o LCG interno do TP.

---

## 2. Estrutura de Dados — Arquivos de Entrada

### 2.1 `EQUIPAS.EF2` — Arquivo de Equipes

Arquivo de texto ASCII (CP850/Latin-1) com terminadores `\r\n` (DOS). Contém **29 equipes** na versão brasileira/portuguesa analisada (**correção: a v1 deste documento dizia erroneamente 28**), cada uma com um bloco estruturado da seguinte forma:

```
 NOME_DA_EQUIPE   NAC\r\n
N1\r\n
N2\r\n
 Nome_Jogador1    pos NAC\r\n
 Nome_Jogador2    pos NAC\r\n
...
\r\n          ← linha em branco separa equipes
```

**Campos do cabeçalho de equipe:**
- `NOME_DA_EQUIPE`: string de até ~14 chars, padded com espaços à esquerda, em maiúsculas. Começa obrigatoriamente com um espaço.
- `NAC`: código de 3 letras da nacionalidade/liga da equipe (neste arquivo: sempre `POR`).
- `N1`: inteiro lido como INTEGER do Turbo Pascal (16 bits). Valores presentes: `{0, 1, 2, 4, 7, 14, 15}`.
- `N2`: inteiro lido como INTEGER do Turbo Pascal (16 bits). Valores presentes: `{0, 1, 2, 4, 7, 14, 15}`.

**Semântica de N1 e N2 — análise definitiva:**

Os valores `{0, 1, 2, 4, 7, 14, 15}` não cobrem o range completo 0–15. Isso reflete os valores deste arquivo de demonstração específico; o jogo aceita qualquer inteiro. A análise do código de leitura (offset `0x1a988`) revela multiplicação por índice de equipe para cálculo de orçamento, levando à seguinte hipótese mais fundamentada:

- **N1** = posição no chaveamento da copa / "pote" de sorteio. Equipes com o mesmo N1 são agrupadas no mesmo pote. Com 29 equipes e copa de 16 slots, N1 seleciona quais equipes participam e em que metade do chaveamento.
- **N2** = fator de orçamento/força inicial do clube. Os valores sugerem uma escala invertida (0 = clube mais rico/forte, 15 = mais pobre/fraco) ou uma escala absoluta usada como multiplicador. Evidência: CORINTHIANS, BOTAFOGO, SANTOS, BRAGANTINO, GUARANI, VITORIA, NACIONAL têm N2=0 (clubes relativamente fortes ou médios); VASCO, ATLETICO, PALMEIRAS, PAYSSANDU têm N2=15.

**Nota importante para o port:** A distribuição de N2 não cria uma hierarquia limpa de "forte vs fraco" por si só — o cálculo de força dos jogadores individuais em runtime é o fator dominante. N2 provavelmente afeta apenas o orçamento inicial e possivelmente a capacidade inicial do estádio.

**Campos de cada jogador:**
- `Nome`: string de até ~14 chars, padded com espaços à esquerda e/ou à direita. **Ver Seção 16 para anomalias de parsing.**
- `pos`: posição em 2 letras — `gr`, `df`, `md`, `av`. Separada do nome por ao menos um espaço (mas ver anomalias).
- `NAC`: código de 3 letras de nacionalidade do jogador.

**Distribuição padrão de jogadores por equipe:**
- 2 guarda-redes, 5 defensores, 5 médios, 4 avançados = **16 jogadores por equipe** (padrão universal no arquivo analisado).

**Lista completa das 29 equipes com seus parâmetros N1/N2:**

| # | Equipe | N1 | N2 |
|---|---|---|---|
| 1 | SAO PAULO | 0 | 7 |
| 2 | CORINTHIANS | 15 | 0 |
| 3 | FLUMINENSE | 2 | 4 |
| 4 | PALMEIRAS | 2 | 15 |
| 5 | BOTAFOGO | 15 | 0 |
| 6 | VASCO | 0 | 15 |
| 7 | Jardim Europa | 14 | 1 |
| 8 | CRUZEIRO | 7 | 1 |
| 9 | ATLETICO | 0 | 15 |
| 10 | GREMIO | 0 | 1 |
| 11 | PARANA | 1 | 4 |
| 12 | PORTUGUESA | 4 | 2 |
| 13 | BRAGANTINO | 7 | 0 |
| 14 | GUARANI | 2 | 0 |
| 15 | JUVENTUDE | 15 | 2 |
| 16 | BAHIA | 4 | 1 |
| 17 | VITORIA | 4 | 0 |
| 18 | CRICIUMA | 0 | 14 |
| 19 | PAYSSANDU | 1 | 15 |
| 20 | GOIAS | 7 | 2 |
| 21 | SANTOS | 15 | 0 |
| 22 | INDEPENDENTE | 4 | 7 |
| 23 | FLAMENGO | 0 | 4 |
| 24 | SANTA CRUZ | 14 | 4 |
| 25 | NACIONAL | 7 | 0 |
| 26 | O POVO | 15 | 1 |
| 27 | REMO F.C. | 7 | 1 |
| 28 | AMERICANO | 15 | 4 |
| 29 | AMERICA F.C. | 15 | 4 |

**Resumo estatístico do arquivo analisado:**
- 29 equipes, 464 jogadores (16 por equipe × 29).
- Nacionalidades no plantel: POR (dominante, ~95%), BRA, SER, CRO, JUG, ARG, ING e outras raras.
- Única equipe com jogador não-POR e não-BRA de posição de campo confirmada: JUVENTUDE (Vurcevic, av SER; Edinho, av BRA), GOIAS (Lewis, av ING).

### 2.2 `TREINAD.EF2` — Arquivo de Treinadores

Arquivo de texto com uma lista de nomes de treinadores, um por linha (`\r\n`), terminada pela sentinela `[EOF]` seguida de `\x1a` (EOF do DOS/CP/M). Contém exatamente **50 treinadores** (**correção: a v1 dizia "aproximadamente 50–60"**).

Os nomes têm acentuação em CP850. Lista completa:

```
 0: W. Luxemburgo          26: Manuel Fernandes
 1: Joel Santana           27: Luis Miguel
 2: Tele Santana           28: Rui Mancio
 3: W. Rodrigues           29: Diamantino Vieira
 4: Luis Felipe            30: Edmundo Duarte
 5: Raul Águas             31: Norton de Matos
 6: Quinito                32: Henrique Nunes
 7: Paco Fortes            33: Ricardo Formosinho
 8: Acécio Casimiro        34: Amandio Barreiras
 9: Vitor Oliveira         35: José Piruta
10: Mário Reis             36: Prof. Neca
11: Ernesto Paulo          37: Filipovic
12: José Romão             38: Alberto Costa
13: Rodolfo Reis           39: Augusto Inácio
14: Manuel Cajuda          40: Jesualdo Ferreira
15: Eurico Gomes           41: Toni
16: Vitor Manuel           42: João Alves
17: António Jesus          43: Daniel Silva
18: José Rachão            44: Joaquim Parolo
19: Francisco Vital        45: Lee Hoe-Taik
20: Carlos Manuel          46: José Passado
21: Abel Braga             47: Quim Tolo
22: Vieira Nunes           48: Estebes
23: Jorge Jesus            49: Ti Manel Coibes
24: Adelino Teixeira        [EOF]
25: Joaquim Teixeira
```

**Notas de encoding para o port:** Os nomes contêm acentos CP850: `á` (0x87), `é` (0x82), `ã` (0xA3), `ô` (0x93), `ç` (0x87), `ó` (0xA2), `ú` (0xA3), etc. Para a ROM do Genesis, esses precisam ser convertidos para ASCII-safe ou mapeados para glifos da fonte customizada.

---

## 3. Fluxo de Jogo — Estados e Transições

O jogo funciona como uma máquina de estados com as seguintes fases principais:

```
[INÍCIO] → Carregar EQUIPAS.EF2 + TREINAD.EF2
         → Escolher: Novo Jogo | Continuar Jogo
              ↓
[SELEÇÃO DE EQUIPE] → Jogador insere nome + escolhe equipe
              ↓
[LOOP PRINCIPAL DA TEMPORADA]
    ├─ [PRÉ-RODADA]
    │     ├─ Gestão de plantel (substituições, formação)
    │     ├─ Negociações (salários, transferências)
    │     ├─ Gestão financeira (ver receitas, prêmios)
    │     └─ Consulta de informações (classificação, calendário, palmarés)
    │
    ├─ [RODADA DE CAMPEONATO]
    │     ├─ Simular cada partida da jornada
    │     ├─ Exibir resultados
    │     └─ Atualizar classificação
    │
    ├─ [RODADA DE COPA (intercalada)]
    │     ├─ Sorteio (início da copa)
    │     ├─ 1ª Eliminatória (1ª mão)
    │     ├─ 1ª Eliminatória (2ª mão)
    │     ├─ 2ª Eliminatória (1ª e 2ª mão)
    │     ├─ Quartos de Final
    │     ├─ Meias-Finais
    │     └─ Final da Taça
    │
    └─ [FIM DE TEMPORADA]
          ├─ Calcular e exibir prêmios
          ├─ Promoção/Rebaixamento
          ├─ Atualizar palmares
          └─ Iniciar nova temporada (loop)
```

### 3.1 Início de Jogo

Ao iniciar, o jogo verifica a presença de `EQUIPAS.EF2` e `TREINAD.EF2` no diretório de trabalho. Se não encontrar, exibe (strings literais do executável):

```
Nao encontrei EQUIPAS.EF2
ERRO — VERIFIQUE O FICHEIRO — EQUIPAS.EF2

Nao encontrei TREINAD.EF2
```

O jogo pergunta `Quer continuar um jogo? ` (com espaço trailing — exato do binário). Se sim, solicita nome do save e diretório.

### 3.2 Seleção de Equipe

O jogador insere:
- **Nome**: string de texto livre (identidade do técnico). Tela exibe `NOME` e `EQUIPA` como labels.
- **Equipe**: selecionada do arquivo carregado.

A equipe adversária é controlada pela IA para todas as outras 28 equipes.

---

## 4. Sistema de Ligas e Competições

### 4.1 Divisões

O jogo simula **quatro divisões** (`1ª DIVISAO`, `2ª DIVISAO`, `3ª DIVISAO`, `4ª DIVISAO`). As 29 equipes do arquivo são distribuídas entre as divisões com base em parâmetros iniciais. Há promoção e rebaixamento ao fim de cada temporada.

A classificação de cada divisão é acompanhada por rodada e exibida com a estrutura de tabela de duas linhas de cabeçalho (confirmada pelo binário, offset `0x181d4`):

```
╔═════════════════════════════════════╦════════════╦════════════╦═════════════════════════╗
║                                     │    CASA    │    FORA    │          TOTAL          ║
║                                     │ J  V E D G │ J  V E D G │  J   V  E  D    G     P ║
```

Onde: J=jogos, V=vitórias, E=empates, D=derrotas, G=saldo de gols, P=pontos (TOTAL apenas).

**Nota para o port:** A tabela usa bordas duplas (`╔═╗║╚╝`) e simples (`│`) do CP437. O layout real ocupa a largura completa de 80 colunas no DOS — na adaptação para 40 colunas do Genesis, será necessário mostrar apenas TOTAL com J/V/E/D/G/P, omitindo as colunas CASA e FORA, ou implementar scroll horizontal.

### 4.2 Campeonato

O campeonato é disputado em sistema de pontos corridos (todos contra todos em cada divisão), com rodadas chamadas de `JORNADA`. O menu `Shift+F1` exibe o calendário completo, `Shift+F2` os resultados anteriores.

A tela de classificação tem o título exato: `CLASSIFICACAO - ` (seguido do número da jornada), confirmado em `0x18310`.

### 4.3 Copa (Taça)

A copa é disputada em eliminatórias com **duas mãos** (ida e volta). Fases identificadas nos strings (com strings literais do executável):

| Fase | String Literal no EXE | Offset |
|---|---|---|
| Sorteio | `SORTEIO DOS JOGOS DA TACA` | `0x051b0` |
| 1ª Eliminatória | `1ª ELIMINATORIA (` + `ª MAO)` | `0x0586a` |
| 2ª Eliminatória | `2ª ELIMINATORIA (` | `0x05883` |
| Quartos de Final | `QUARTOS DE FINAL  (` | `0x05895` |
| Meias-Finais | `MEIAS FINAIS (` | `0x058a9` |
| Final | `FINAL` | `0x058b8` |
| Final da Taça | `FINAL DA TACA` | `0x08794` |
| Equipes apuradas | `EQUIPAS APURADAS` | `0x07ee1` |
| Equipes eliminadas | `EQUIPAS ELIMINADAS` | `0x087ae` |
| Vencedor | `VENCEDOR DA TACA` | `0x07ef2` |

A exibição das fases usa string `1ª ELIMINATORIA   `, `2ª ELIMINATORIA   `, `QUARTOS DE FINAL   `, `MEIAS FINAIS   ` em versão estendida com espaços para a tabela de acompanhamento (offsets `0x13913`, `0x13926`, `0x13939`, `0x1394d`).

O indicador de mão: ` (1ª MAO)` e ` (2ª MAO)` (strings em `0x13983` e `0x1398d`).

### 4.4 Tempo Extra e Pênaltis

Se o resultado agregado for empate ao fim dos 90 minutos, entra `PROLONGAMENTO`. Se persistir, série de pênaltis:

```
º PENALTY (CASA)
EXECUTADO           
º PENALTY (FORA)
EXECUTADO          
```

O caractere `º` inicial (offset `0x6ec9`, byte `0xBA` = `║` em CP437) confirma que as linhas de penalty são exibidas **dentro de uma caixa de borda dupla**, não como texto solto. O campo `EXECUTADO` tem 20 espaços de largura — padding para alinhamento.

### 4.5 Prêmios ao Fim da Temporada

Tela `PREMIOS DA TEMPORADA` (offset `0x0be5b`), exibe:

| Prêmio | String Literal | Destinatário |
|---|---|---|
| Campeão | `VENCEDOR DO CAMPEONATO` | 1º colocado da divisão |
| Copa | `VENCEDOR DA TAÇA` | Campeão da copa |
| Melhor Ataque | `MELHOR ATAQUE (X GOLOS)` | Equipe com mais gols |
| Melhor Defesa | `MELHOR DEFESA (X GOLOS)` | Equipe com menos gols sofridos |
| Melhor Marcador | `MELHOR MARCADOR (X GOLOS)` | Jogador artilheiro |
| 2º Classificado | ` 2º CLASSIFICADO            ` | 2º colocado |
| 3º Classificado | ` 3º CLASSIFICADO            ` | 3º colocado |

Cada prêmio entrega um valor monetário creditado no orçamento da equipe.

---

## 5. Equipes, Jogadores e Posições

### 5.1 Posições

O jogo usa quatro posições (strings literais encontrados em `0x18ffa`–`0x19025`):

| Código no arquivo | String de display | Nome PT-BR | Papel tático |
|---|---|---|---|
| `gr` | `Guarda-redes` | Goleiro | Defende o gol; posição especial |
| `df` | `Defesa` | Defensor | Linha defensiva |
| `md` | `Medio` | Meia | Meio-campo |
| `av` | `Avancado` | Atacante | Linha ofensiva |

**Nota:** Os strings de display no executável são `Guarda-redes`, `Defesa`, `Medio`, `Avancado` — sem acento em `Médio` e `Avançado`. Isso é o texto que aparece nas telas de plantel e transferência, **não** o código de duas letras do arquivo.

### 5.2 Escalação Padrão

Uma equipe escalada para uma partida contém **11 titulares** selecionados do plantel de 16. O sistema de formação define quantos jogadores de cada posição entram em campo (exceto o `gr`, que é sempre 1).

O banco de reservas é exibido como `JOGADORES NO BANCO` (offset `0x1c7d9`) e `JOGADORES EM CAMPO` (offset `0x1c7c6`). Durante a visualização do plantel, é possível fazer substituições via `F1` (`F1  Substituir`, `Esc Fim` — offsets `0x1c7ec` e `0x1c7fb`).

A tela de suplentes usa as labels `SUPLENTES` (offset `0x1dcc0`).

### 5.3 Restrições Críticas de Plantel

Identificadas nos strings literais do executável:

**Mínimo de 14 jogadores** (offset `0x123e7`):
```
Como só possui 14 jogadores na equipa
```
Se o plantel tiver exatamente 14 jogadores, o clube é **obrigado** a aceitar qualquer pedido de aumento de salário, pois não pode dispensar o jogador.

**Mínimo de 1 guarda-redes** (offset `0x1239d`):
```
Como só possui um guarda-redes na equipa, este
```
Se só há 1 goleiro no plantel, pedidos de aumento dele são aceitos compulsoriamente.

Strings de aceitação compulsória (offset `0x12cc5` e `0x12d05`):
```
 Como só tem 14 jogadores
 no plantel é obrigado
 a aceitar.

 Como só tem um guarda-redes
```

### 5.4 Autoescalar

O menu `Alt+F3` (`Seleccionar melhores`) faz a IA escolher automaticamente os 11 melhores disponíveis para a formação atual.

### 5.5 Nacionalidades

A nacionalidade do jogador (`NAC`) afeta o cálculo de força. Jogadores estrangeiros identificados no arquivo: SER (sérvios), CRO (croatas), JUG (iugoslavos), ARG (argentinos), BRA (brasileiros em contexto de liga portuguesa), ING (ingleses).

---

## 6. Mecânica de Força e Atributos

### 6.1 Força do Jogador

O arquivo `EQUIPAS.EF2` **não contém valores numéricos de força por jogador** — apenas nome, posição e nacionalidade. A força é **calculada em runtime**.

A tela de venda confirma a existência do atributo como coluna visível (string literal `FORÇA`, offset `0xd9a9` — em CP850 aparece como `FOR\x87A` com cedilha acentuada):

```
VENDA PELA MELHOR OFERTA DE ORDENADO
 JOGADOR   |  POSIÇÃO  |  FORÇA    |  EQUIPA   |  PREÇO    
ORDENADO MÍNIMO: [valor]
```

O string de pedido de aumento (offset `0x12360`) é:
```
[NOME], [POS], [FORÇA] de força ,pede que lhe seja aumentado o ordenado para [VALOR].
```

Isso confirma que a **força é um inteiro** (precedido de sua representação textual) e que está diretamente correlacionada com o salário mínimo exigido.

### 6.2 Faixa Provável de Força

Baseado no comportamento típico de jogos Turbo Pascal de gerenciamento da era: **1–99** (1 byte, armazenado como INTEGER mas valores práticos em 1–99). A fórmula inferida:
- Força base por posição: GR e DF tendem a ter faixas mais altas (25–70), MD e AV faixas médias (20–65).
- Modificador por ordem no elenco: primeiro jogador listado por posição = titular com força ligeiramente maior.
- Componente aleatória na geração inicial (LCG seed do Turbo Pascal).
- Crescimento/decaimento por temporada (inferido — jogadores envelhecem).

### 6.3 Chicotadas Psicológicas

A tela `CHICOTADAS PSICOLOGICAS` (offset `0x1645f`) é acionada ao demitir/trocar treinador. Strings literais:

```
[NOME] foi despedido do [CLUBE]
Para o seu lugar foi escolhido [NOVO TREINADOR]
Troca com [OUTRO TÉCNICO]
```

Efeito mecânico: boost temporário de moral/motivação, afetando desempenho nas próximas partidas.

### 6.4 Moral (Inferido)

Atributo não exposto diretamente nas strings, mas implícito pelo sistema de treinadores e chicotadas. Afeta o modificador de performance na simulação de partidas. Sobe com vitórias e prêmios; cai com derrotas consecutivas.

---

## 7. Sistema Tático — Formações

### 7.1 Dez Formações Disponíveis

Identificadas nos strings do executável (menu `TACTICAS`, offset `0x11578`):

| Tecla | Formação | Estrutura (GR–DF–MD–AV) | String Literal |
|---|---|---|---|
| F1 | 3-4-3 | 1–3–4–3 | ` F1 3-4-3` |
| F2 | 4-3-3 | 1–4–3–3 | ` F2 4-3-3` |
| F3 | 4-4-2 | 1–4–4–2 | ` F3 4-4-2` |
| F4 | 4-5-1 | 1–4–5–1 | ` F4 4-5-1` |
| F5 | 5-2-3 | 1–5–2–3 | ` F5 5-2-3` |
| F6 | 5-3-2 | 1–5–3–2 | ` F6 5-3-2` |
| F7 | 5-4-1 | 1–5–4–1 | ` F7 5-4-1` |
| F8 | 5-5-0 | 1–5–5–0 | ` F8 5-5-0` |
| F9 | 6-3-1 | 1–6–3–1 | ` F9 6-3-1` |
| F10 | 6-4-0 | 1–6–4–0 | `F10 6-4-0` |

As formações ultradefensivas `5-5-0` e `6-4-0` (sem atacantes) são usadas para proteger vantagem. A `3-4-3` é a mais ofensiva. A `6-4-0` com 6 defensores é a mais extrema.

**Nota:** A formação define exatamente quantos jogadores de cada posição são escalados. Se o plantel não tem jogadores suficientes em uma posição, o jogo provavelmente proíbe a formação ou preenche com jogadores fora de posição (comportamento a verificar no port).

---

## 8. Simulação de Partidas

### 8.1 Modelo de Simulação

A partida é simulada sem controle em tempo real. O algoritmo (inferido):

1. Calcula **força agregada** de cada equipe com base na formação e jogadores escalados.
2. Aplica modificador de **casa/fora** (home advantage). A tabela de classificação mostra estatísticas separadas para casa e fora, confirmando que o jogo distingue.
3. Aplica modificador de **moral** e qualidade do **treinador**.
4. Usa RNG (provavelmente LCG 16-bit do Turbo Pascal) para determinar número de gols.
5. Atribui gols a jogadores individuais para efeito de artilharia.

### 8.2 Strings de Partida — Exatos

Identificados no binário (área `0x6800`–`0x7200`):

```
INTERVALO          ← exibido no meio da simulação
PROLONGAMENTO      ← tempo extra
º PENALTY (CASA)   ← borda dupla + texto (penalti da equipe mandante)
EXECUTADO           ← resultado do penalti (20 chars com padding)
º PENALTY (FORA)   ← penalti da equipe visitante
EXECUTADO          ← resultado (19 chars com padding)
```

**Detalhe técnico:** O byte `0xBA` (character `║` do CP437) precede `PENALTY` — isso significa que a exibição dos pênaltis está dentro de um box de borda dupla. O port deve reproduzir este layout.

### 8.3 Bilheteria e Espectadores

A tela `ULTIMAS RECEITAS` (offset `0x11310`) exibe:

```
ULTIMAS RECEITAS
ADVERSARIO        JOGO   BILHETES     ESPECTADORES    RECEITA
```

Campos: adversário, resultado do jogo, bilhetes vendidos, espectadores (pode diferir de bilhetes — ingressos grátis?), receita total.

O preço do bilhete é configurável: `Preço dos bilhetes: [valor]` aparece na tela de finanças/estádio (offset `0x138c4`).

---

## 9. Sistema Econômico

### 9.1 Orçamento e Dinheiro

A tela financeira exibe (strings literais, offsets `0x138ad`–`0x138d9`):

```
Dinheiro: [valor]
Ordenados: [valor_total_mensal]
Preço dos bilhetes: [valor]
CASA / FORA
```

**Entradas de receita:**
- Bilheteria das partidas em casa.
- Prêmios de campeonato (1º, 2º, 3º, melhor ataque/defesa/marcador).
- Prêmios de copa.
- Venda de jogadores.

**Saídas de despesa:**
- Salários mensais pagos a todos os jogadores.
- Construção/ampliação de bancadas no estádio.

**Atenção para o port:** Os valores monetários podem ultrapassar 32.767 (limite de `int` de 16 bits no m68k). Usar `long` (32 bits) para todos os campos monetários.

### 9.2 Salários (Ordenados)

Sistema central de gestão. Strings literais completos:

**Pedido de aumento pelo jogador** (offset `0x12366`):
```
[NOME], [FORÇA] de força ,pede que lhe seja aumentado o ordenado para [VALOR].
Como só possui um guarda-redes na equipa, este aumento terá de ser aceite
```

**Confirmação de aceite** (offset `0x12474`):
```
Aceita o aumento (S/N) ? 
```

**Recusa e saída voluntária** (offset `0x1242d`):
```
Caso não aceite o jogador pode decidir pôr-se em leilão pelo preço de [VALOR]
```

**Despedida** (offset `0x12d52`):
```
 Então adeus.
```

**Alteração proativa pelo gestor** (offset `0x12c6c`–`0x12cb0`):
```
 ALTERAR O ORDENADO
Novo ordenado: 
 Nem pensar!
 Exijo um minimo de [VALOR]
```

O string `Nem pensar!` é exibido quando o gestor tenta reduzir o salário abaixo do mínimo aceitável pelo jogador. `Exijo um minimo de` confirma que cada jogador tem um salário mínimo fixo.

### 9.3 Estatísticas de Jogo Financeiro

A tela do estádio (offset `0x1388b`–`0x138ad`) exibe:

```
[N]º lugar  
[X] lugares
[Y] sócios
Dinheiro: [valor]
Ordenados: [valor]
Preço dos bilhetes: [valor]
```

O `º lugar` provavelmente indica a posição atual na classificação, exibida contextualmente na tela de finanças.

---

## 10. Transferências e Mercado

### 10.1 Venda Direta

Menu `Ctrl+F1` (`Vender`). Tela exata (strings em `0xd96a`–`0xda01`):

```
VENDA PELA MELHOR OFERTA DE ORDENADO
 JOGADOR   |  POSIÇÃO  |  FORÇA    |  EQUIPA   |  PREÇO    
ORDENADO MÍNIMO: [valor]
TRANSFERIDO PARA O [CLUBE DESTINO]
NOVO ORDENADO : [valor]
NÃO HOUVE OFERTAS
```

**Notas de layout:** Cada coluna tem 10 chars de largura (confirmado pelos espaços no string bruto). As colunas `POSIÇÃO`, `FORÇA`, `EQUIPA`, `PREÇO` têm padding de 2 espaços antes e após.

### 10.2 Leilão de Salário

Quando jogador decide sair após recusa de aumento, entra em leilão por salário. O mecanismo é único: equipes da IA competem oferecendo o **maior salário** que podem pagar, não um preço de transferência. Permite que clubes mais ricos roubem jogadores pagando mais.

### 10.3 Registro de Últimas Transferências

Menu `Ctrl+F4`. Cabeçalho exato (offset `0xf64c`):

```
ÚLTIMAS TRANSFERÊNCIAS REALIZADAS
JOGADOR           PS FÇ NAC     DE              PARA               ORD   
```

Colunas: JOGADOR (17 chars), PS=posição (2), FÇ=força (2), NAC=nacionalidade (3), DE=equipe origem (14), PARA=equipe destino (14), ORD=ordenado (5).

---

## 11. Treinadores

### 11.1 Funções e Gestão

Cada equipe tem um treinador do pool de `TREINAD.EF2`. O menu `Alt+F4` (`Treinadores`) exibe `TREINADORES EM JOGO` (offset `0x1e443`), com opções (strings literais):

```
F1  Anular treinador
F2  Novo treinador
Esc Fim
```

Se o arquivo `TREINAD.EF2` não for encontrado na tela de gestão: `Não encontrei TREINAD.EF2` (offset `0x1e4b3`).

A linha de prompt de novo treinador: `Nº treinador: ` (offset `0x1e4dc`).

### 11.2 Proposta para Treinadores Adversários

String (offset `0x1612d`):
```
 : quer ir treinar o 
```
Seguido de ` (s/n) ?` — o treinador de uma equipe IA recebe proposta de outra equipe IA, e o jogador pode ser consultado.

### 11.3 Títulos de Treinadores

Menu `Shift+F9` → `TITULOS DOS TREINADORES NAS ULTIMAS 20 EPOCAS` (offset `0x1eeb8`):

```
CAMPEONATOS
[Treinador] — [X] campeonatos
```

Limitado a 20 temporadas para controle de memória.

---

## 12. Estádios

### 12.1 Situação do Estádio

Menu `Alt+F2`. Tela `SITUAÇÃO DO ESTADIO` (offset `0x13466`). Strings de decisão de construção (offset `0x1347a`):

```
Deseja construir uma bancada (preço: [X]) ?
[N] lugares
[M] sócios
```

O estádio tem capacidade medida em **lugares** (torcedores) e **sócios** — dois tipos de público. A construção de bancadas aumenta capacidade e, consequentemente, receita por partida.

### 12.2 Tela Financeira Contextual do Estádio

Exibida na mesma tela (offsets `0x1388b`–`0x138d9`):

```
[N]º lugar  
[X] lugares / pontos
[Y] sócios
Dinheiro: [valor]
Ordenados: [valor]
Preço dos bilhetes: [valor]
CASA
FORA
```

A separação `CASA` / `FORA` indica que existem valores diferentes para jogos em casa e fora (possivelmente preços de bilhete distintos, ou estatísticas separadas).

---

## 13. Interface e Controles

### 13.1 Mapa Completo de Teclas

Extraído diretamente do executável (área de menus `0x11578`–`0x1178d`):

**TÁTICAS (Formações):**
| Tecla | Ação | String Literal |
|---|---|---|
| F1 | Formação 3-4-3 | ` F1 3-4-3` |
| F2 | Formação 4-3-3 | ` F2 4-3-3` |
| F3 | Formação 4-4-2 | ` F3 4-4-2` |
| F4 | Formação 4-5-1 | ` F4 4-5-1` |
| F5 | Formação 5-2-3 | ` F5 5-2-3` |
| F6 | Formação 5-3-2 | ` F6 5-3-2` |
| F7 | Formação 5-4-1 | ` F7 5-4-1` |
| F8 | Formação 5-5-0 | ` F8 5-5-0` |
| F9 | Formação 6-3-1 | ` F9 6-3-1` |
| F10 | Formação 6-4-0 | `F10 6-4-0` |

**CAMPEONATO:**
| Tecla | Ação | String Literal |
|---|---|---|
| Shift+F1 | Calendário da temporada | ` Shift+F1 Calendário` |
| Shift+F2 | Resultados anteriores | ` Shift+F2 Resultados` |
| Shift+F3 | Palmarés (histórico de títulos) | ` Shift+F3 Palmarés` |
| Shift+F4 | Melhores marcadores | ` Shift+F4 Marcadores` |
| Shift+F5 | Últimos vencedores de temporadas | ` Shift+F5 Ult. Vencedores` |
| Shift+F6 | Classificação da divisão | ` Shift+F6 Classificação` |
| Shift+F7 | Próximas jornadas | ` Shift+F7 Próximas jornadas` |
| Shift+F8 | Títulos das equipes | ` Shift+F8 Títulos equipas` |
| Shift+F9 | Títulos dos treinadores | ` Shift+F9 Títulos treinadores` |

**FINANÇAS:**
| Tecla | Ação | String Literal |
|---|---|---|
| Ctrl+F1 | Vender jogador | ` Ctrl+F1 Vender` |
| Ctrl+F2 | Alterar ordenados | ` Ctrl+F2 Ordenados` |
| Ctrl+F3 | Prêmios da temporada | ` Ctrl+F3 Prémios` |
| Ctrl+F4 | Últimas transferências | ` Ctrl+F4 Transferências` |
| Ctrl+F5 | Últimas receitas | ` Ctrl+F5 Receitas` |

**DIVERSOS:**
| Tecla | Ação | String Literal |
|---|---|---|
| Alt+F1 | Gravar jogo | ` Alt+F1 Gravar` |
| Alt+F2 | Estádio | ` Alt+F2 Estádio` |
| Alt+F3 | Selecionar melhores (auto-escalar) | ` Alt+F3 Seleccionar melhores` |
| Alt+F4 | Treinadores | ` Alt+F4 Treinadores` |
| Alt+F10 | Sair para DOS | ` Alt+F10 Saír para DOS` |

### 13.2 Interface Textual

Toda a interface é 80×25 caracteres com:
- Bordas em box-drawing characters CP437 (simples `┌─┐│└┘` e duplas `╔═╗║╚╝`).
- Cores de foreground/background de 4 bits cada (16 cores CGA).
- Menus contextuais sobrepostos (popup sobre o estado atual).
- Confirmações binárias `(S/N)` para ações destrutivas.
- Textos de feedback inline: `Nem pensar!`, `Exijo um minimo de [X]`, `Então adeus.`

### 13.3 Saída do Jogo

String literal (offset `0x1375e`):
```
 SAÍR PARA O DOS ?
     ( S / N )
```

---

## 14. Sistema de Saves

### 14.1 Gravação

Menu `Alt+F1`. Strings literais (offsets `0x194c3`–`0x194fd`):

```
save               ← label interno Pascal (minúsculas)
GRAVAR O JOGO
Directoria: 
Nome da gravacao: 
AGUARDE UM MOMENTO
```

Erro de disco protegido (offset `0x17c89`–`0x17caf`):
```
E R R O
O disco está protegido contra escrita
Verifique a disquete
Tecle <ESC> para seguir
```

### 14.2 Carregamento

Ao iniciar: `Quer continuar um jogo? ` (com espaço trailing). Strings de load:

```
load               ← label interno Pascal
CONTINUAR UM JOGO
Directoria: 
Não há gravações nesta directoria
Deseja tentar de novo (S/N)? 
ERRO A LER O FICHEIRO [nome]
Quer tentar outra vez ? 
```

### 14.3 Formato do Save

O save é um dump binário do estado completo do jogo em memória. Conteúdo mínimo estimado:
- Classificação de 4 divisões (pontos, vitórias, empates, derrotas, gols por equipe).
- Plantel atual de cada equipe (16 jogadores × 29 equipes = 464 entradas com força + salário + gols marcados).
- Saldo bancário de cada equipe (29 × `long`).
- Capacidade do estádio de cada equipe (29 × `int`).
- Jornada atual e fase da copa.
- Palmares acumulado (até 20 épocas × prêmios).
- Treinador atual de cada equipe.

Tamanho estimado: 4–10 KB (excede 512 bytes da SRAM padrão Genesis — ver Seção 17).

---

## 15. Palmares e Histórico

### 15.1 Palmares por Equipe

Menu `Shift+F3` → `PALMARES` (offset `0x0eb00`). Strings de formato (offsets `0x0eb09`–`0x0ebb0`):

```
[EQUIPE] — [Nª DIVISAO]:
  [X] época(s)
  1 campeonato da 1ª divisão
  [N] campeonatos da 1ª divisão
  [N] presença(s) na final da taça
  [N] taça(s)
  [N] na 2ª eliminatória
  [N] nos quartos-de-final
  [N] nas meias-finais
[Nª DIVISÃO]: [M] presenças
```

### 15.2 Títulos dos Treinadores

Menu `Shift+F9` → `TÍTULOS DOS TREINADORES NAS ÚLTIMAS 20 ÉPOCAS` (offset `0x1eeb8`):

```
CAMPEONATOS
[Treinador] — [X] campeonatos

TAÇAS
[Treinador] — [X] taças
```

As seções `CAMPEONATOS` e `TAÇAS` (offsets `0x1eb3d` e `0x1eb49`) são separadas.

### 15.3 Títulos das Equipes

Menu `Shift+F8` → `TITULOS DAS EQUIPAS` (offset `0x1eb29`).

### 15.4 Últimos Vencedores

Menu `Shift+F5` → `ULTIMOS VENCEDORES` (offset `0x18bd8`).

---

## 16. Anomalias e Dados Sujos nos Arquivos Fonte

Esta seção é **crítica para o port** — o parser deve ser robusto o suficiente para lidar com estes casos reais.

### 16.1 Jogadores sem Espaço antes da Posição

Quatro jogadores no arquivo têm o campo `pos` colado ao final do nome (ausência do espaço separador):

| Equipe | Linha no arquivo | Problema |
|---|---|---|
| JUVENTUDE | ` Paulo Alexandredf POR` | `Alexandre` seguido diretamente de `df` |
| JUVENTUDE | ` Jaime Cerqueiramd POR` | `Cerqueira` seguido diretamente de `md` |
| VITORIA | ` Alberto Machadomd POR` | `Machado` seguido diretamente de `md` |
| GOIAS | ` Ricardo Martinsdf POR` | `Martins` seguido diretamente de `df` |
| INDEPENDENTE | ` Artur Alexandremd POR` | `Alexandre` seguido diretamente de `md` |

**Estratégia de parsing para o port:** Ao ler a linha de um jogador, fazer o split pelo **fim** da linha (`rsplit(None, 2)` em Python, ou ler os últimos 6 chars como `pos NAC` e o restante como nome). Nunca assumir que o separador entre nome e posição é sempre um espaço.

### 16.2 Caracteres CP850 Acentuados em Nomes

Vários jogadores e treinadores têm acentos CP850 que não são ASCII. Exemplos:
- `JoÒo Luís` (FLUMINENSE) — `ò` = 0x93 em CP850
- `Ac cio` (GOIAS) — o arquivo tem um espaço no meio do nome (Acácio com encoding corrompido)
- `Titô` (INDEPENDENTE) — `ô` com acento
- `Jójó` (INDEPENDENTE) — duplo `ó`
- `Tomé` (INDEPENDENTE)

Para a ROM Genesis: converter CP850 → ASCII transliterado (remover acentos) OU criar glifos extras na fonte. Recomenda-se a transliteração para simplificar (João → Joao, José → Jose, etc.).

### 16.3 Código de Nacionalidade Errado

- PALMEIRAS, linha `Antonio Carlos df POK` — `POK` ao invés de `POR`. Provavelmente erro de digitação original. O parser deve aceitar qualquer string de 3 chars sem validar contra lista de países.

---

## 17. Recomendações para o Port Genesis

### 17.1 O que Preservar do Original

- **Mecânica core**: a lógica de simulação de partidas, o sistema econômico de salários/leilão, e as dez formações táticas são os pilares do jogo e devem ser portados fielmente.
- **29 equipes e 464 jogadores**: carregar do binário embutido na ROM (substituindo `EQUIPAS.EF2`).
- **50 treinadores**: idem, embutir de `TREINAD.EF2`.
- **Loop de temporada**: campeonato em 4 divisões + copa em fases eliminatórias com 2 mãos.
- **Restrições de plantel** (mínimo 14 / 1 goleiro): estas regras dão personalidade ao sistema econômico.
- **Strings de feedback**: `Nem pensar!`, `Então adeus.`, `Não houve ofertas` — são parte da personalidade do jogo.

### 17.2 Adaptações Necessárias para Genesis

**Input:**
- Substituir F1–F10 / Shift+Fx / Ctrl+Fx / Alt+Fx por pad de 6 botões do Mega Drive.
- Mapeamento sugerido:
  - `A` = confirmar / selecionar (equivalente a Enter/S)
  - `B` = cancelar / voltar (equivalente a Esc/N)
  - `C` = ação secundária (ex: auto-escalar, equivalente a Alt+F3)
  - `X/Y/Z` = atalhos de menu (formações, informações)
  - `Start` = menu principal / pausa
  - D-pad = navegar menus e listas

**Exibição e cores:**
- Tela de 320×224 pixels, 40×28 tiles de 8×8 (modo VDP padrão do Genesis).
- O original usa 80×25 — adaptar layouts para caber em 40 colunas.
- Substituir box-drawing CP437 por tiles de borda desenhados no VDP.
- Paletes: usar as 4 paletes de 16 cores do Genesis para diferenciar equipes, menus, e texto de destaque, reproduzindo as cores CGA originais o mais fielmente possível.
- Usar BG_A para o conteúdo de jogo e WINDOW plane para menu de status fixo (rodada, dinheiro, divisão).

**Layout de tela recomendado para 40×28:**
```
┌────────────────────────────────────────┐  ← WINDOW (linhas 0–1)
│ ELIFOOT II   Jornada:12  Div1  $74500  │  Status bar fixa
├────────────────────────────────────────┤
│                                        │
│         CONTEÚDO PRINCIPAL             │  ← BG_A (linhas 2–25)
│         (menus, listas, resultados)    │  23 linhas utilizáveis
│                                        │
├────────────────────────────────────────┤
│  [A] Confirmar  [B] Cancelar  [C] Auto │  ← WINDOW (linhas 26–27)
└────────────────────────────────────────┘  Help bar fixa
```

**Armazenamento:**
- O estado completo do jogo excede os 512 bytes de SRAM padrão. Opções:
  1. **SRAM de 8 KB** (cartucho com bateria maior — viável para homebrew): comporta 3 slots completos.
  2. **Save compactado de estado mínimo** (apenas a equipe do jogador + classificação resumida) dentro de 512 bytes.
  3. **Password system** (não requer SRAM): codifica o estado em ~20 caracteres selecionáveis com D-pad.
- Usar `SRAM_enable()` / `SRAM_writeByte()` / `SRAM_disable()` conforme SGDK 1.70.

**Gerador de Números Aleatórios:**
- O RNG do Elifoot original é o LCG interno do Turbo Pascal.
- No Genesis, usar `random()` do SGDK ou implementar LCG próprio com semente baseada em V-counter + frame count para variabilidade natural.

### 17.3 Caveats SGDK 1.70 Críticos

- `int` = 16 bits no m68k: salários e valores financeiros devem ser `long` (32 bits).
- `JOY_init()` deve ser chamado antes de qualquer leitura de controle — sem isso o jogo parecerá travado.
- `SYS_doVBlankProcess()` só deve ser chamado no loop principal de vsync, nunca dentro de loops de cálculo (simulação de 29 partidas por rodada, ordenação de classificação).
- `VDP_clearPlane(WINDOW, TRUE)` deve ser chamado ao trocar de tela para evitar resíduos de texto.
- `VDP_drawText()` escreve apenas no WINDOW com paleta fixa — para texto colorido em BG_A, usar `VDP_setTileMapXY()` diretamente com tiles da fonte customizada e tile attribute word com índice de paleta.

### 17.4 Dados a Embutir na ROM

Os arquivos devem ser convertidos para binários compactos embutidos via rescomp:

```
// res/resources.res
BIN teams_data   "data/teams.bin"    2 2 0 FAST
BIN coaches_data "data/coaches.bin"  2 2 0 FAST
```

Pré-processamento necessário em Python antes da conversão:
1. Strip de `\r\n` → `\n`.
2. Transliteração CP850 → ASCII (remover acentos).
3. Normalização dos nomes de jogador (tratar anomalias de parsing da Seção 16).
4. Compactação em estrutura binária de tamanho fixo por registro.

### 17.5 Funcionalidades a Simplificar ou Omitir

| Feature Original | Recomendação para o Port | Razão |
|---|---|---|
| Quatro divisões completas | Reduzir para 2 divisões (16+13 equipes) para caber em RAM | RAM 64 KB do Genesis |
| Nome do técnico digitado livremente | Selecionar de lista pré-definida | Sem teclado no Genesis |
| Diretória de save configurável | Save em SRAM fixo | Sem sistema de arquivos |
| Múltiplos saves com nome | Até 3 slots de save identificados por número | Limitação de SRAM |
| Histórico de 20 épocas no palmares | Reduzir para 5 épocas | Restrição de SRAM |
| Tabela de classificação 80 colunas (CASA+FORA+TOTAL) | Exibir apenas TOTAL em 40 colunas | Largura da tela |
| Impressão em LPT1 | Omitir | Hardware inexistente no Genesis |

### 17.6 Paleta de Cores CGA → Genesis

O jogo DOS usa o modo de texto CGA com atributos de 4 bits de foreground e 4 bits de background. As cores CGA usadas precisam ser mapeadas para os 9 bits por componente (RGB333) do VDP do Genesis:

| Cor CGA | RGB CGA (típico monitor) | Valor Genesis (RGB333 aprox.) |
|---|---|---|
| 0 Preto | `#000000` | `0x0000` |
| 1 Azul escuro | `#0000AA` | `0x0006` |
| 2 Verde escuro | `#00AA00` | `0x0060` |
| 3 Ciano escuro | `#00AAAA` | `0x0066` |
| 4 Vermelho escuro | `#AA0000` | `0x0600` |
| 5 Magenta escuro | `#AA00AA` | `0x0606` |
| 6 Marrom/laranja | `#AA5500` | `0x0640` |
| 7 Cinza claro | `#AAAAAA` | `0x0666` |
| 8 Cinza escuro | `#555555` | `0x0444` |
| 9 Azul brilhante | `#5555FF` | `0x000E` |
| 10 Verde brilhante | `#55FF55` | `0x00E0` |
| 11 Ciano brilhante | `#55FFFF` | `0x00EE` |
| 12 Vermelho brilhante | `#FF5555` | `0x0E00` |
| 13 Magenta brilhante | `#FF55FF` | `0x0E0E` |
| 14 Amarelo | `#FFFF55` | `0x0EE0` |
| 15 Branco | `#FFFFFF` | `0x0EEE` |

A paleta principal do jogo usa texto branco (15) ou amarelo (14) sobre fundo azul escuro (1) ou preto (0) para a maior parte da interface. Cores de destaque usam ciano (11) ou verde brilhante (10) para cabeçalhos.

---

*Documento gerado com base em engenharia reversa aprofundada de `elifoot.exe` (143.648 bytes, MS-DOS MZ, Turbo Pascal), análise binária/textual de `EQUIPAS.EF2` (14.812 bytes, 29 equipes, 464 jogadores) e `TREINAD.EF2` (778 bytes, 50 treinadores), e conhecimento acumulado do jogo Elifoot II.*

*v2 — Correções: contagem de equipes (28→29), contagem de treinadores (~50-60→50), semântica de N1/N2 refinada, strings literais adicionados com offsets, anomalias de parsing documentadas, tabela de cores CGA adicionada, análise da tela de classificação aprofundada.*


---

## 20. Segunda Analise Aprofundada -- Varredura Completa do Binario

> Analise sistematica de TODOS os strings do executavel (8057 strings extraidas),
> com identificacao de funcionalidades ainda nao documentadas.

### 20.1 MELHORES MARCADORES -- Ecra Dedicado (Shift+F4)

**Evidencia:** offset `0x10c7b`: `"MELHORES MARCADORES"` (titulo de ecra, nao apenas label de menu).

O ecra de artilheiros e um ecra completo separado, nao apenas uma linha na classificacao.
Mostra todos os jogadores com golos marcados, em ordem decrescente.
O port actual exibe apenas o artilheiro no rodape da classificacao -- falta o ecra completo.

### 20.2 SUBSTITUICOES no Plantel (F1 Substituir)

**Evidencia:** offset `0x1c7ed`: `"F1  Substituir"` / `"Esc Fim"` imediatamente apos `"JOGADORES EM CAMPO"` / `"JOGADORES NO BANCO"`.

O original tem dois modos de visualizacao do plantel:
- **JOGADORES EM CAMPO** -- titulares
- **JOGADORES NO BANCO** -- suplentes

Com tecla F1 para fazer substituicoes entre os dois grupos. A logica e diferente
do simples toggle que implementamos: e um mecanismo de TROCA entre titular e suplente
especifico, provavelmente com confirmacao.

**String `"SUPLENTES"`** (offset `0x1dcc1`) confirma uma vista separada de suplentes.

### 20.3 PROPOSTA DE TREINADOR ENTRE EQUIPAS IA (Ctrl+F2 area)

**Evidencia:** offset `0x1612e`: `" : quer ir treinar o "` seguido de `" (s/n) ?"`.

Mecanismo de proposta de treinador entre equipas IA: um treinador de uma equipa IA
pode querer ir treinar outra equipa IA, e o jogador e consultado (S/N).
Esta e uma funcionalidade de gestao do mundo do jogo, nao apenas da equipa do jogador.

**String `"Troca com"` (offset `0x164ab`)** e parte do mesmo sistema -- troca de treinadores
entre clubes (chicotada com troca mutua vs contratacao unilateral).

### 20.4 TREINADOR DEIXA DE JOGAR

**Evidencia:** offset `0x1e4dd`: `"deixa de jogar? "` na area de gestao de treinadores,
imediatamente apos `"Nº treinador: "`.

Evento onde um treinador pode anunciar que quer deixar o futebol. Diferente de ser despedido
-- e uma saida voluntaria do treinador. O jogador provavelmente escolhe o substituto.

### 20.5 ENTRADA DE NOME DO TECNICO (texto livre)

**Evidencia:** offset `0x1b8c5`: `"Entra:"` na area de seleccao inicial de equipa (proximo
de `"Quer continuar um jogo?"`, `"NOME"`, `"EQUIPA"`).

Confirma que o jogo aceita texto livre para o nome do tecnico. O label `"Entra:"` e o prompt
de entrada de teclado (equivalente a "Digite:"). O port nao implementa entrada de texto.

### 20.6 NOME DO NOVO TREINADOR (input na gestao)

**Evidencia:** offset `0x1e50e`: `"Nome: "` no contexto da area de treinadores.

Ao contratar um novo treinador, o original pode mostrar o nome actual e pedir confirmacao.

### 20.7 SUPLENTES -- Vista Separada

**Evidencia:** offset `0x1dcc1`: `"SUPLENTESUëÕ©"` (string de titulo de ecra).

Existe um ecra separado apenas para suplentes, alem do ecra de campo. O port actual mostra
todos os jogadores numa lista unica com a coluna T/S.

### 20.8 BOX-DRAWING BORDERS -- UI Original

**Evidencia:** offset `0x171be`: sequencia longa de caracteres de borda CP437 
(`┌─┐`, `└─┘`, `║`, `═`, `╔`, `╗`, `╚`, `╝`, etc.).

**Offset `0x180e7`**: `"N╔═════...═╗N"` -- borda dupla da classificacao.

O original usa bordas graficas extensas. O port usa linhas de traco simples (`-`).
Nao e uma funcionalidade em falta, mas e uma diferenca visual significativa.

### 20.9 IMPRESSORA (LPT1) -- Ignoravel

**Evidencia:** offset `0x16e91`: `"LPT1"`. Suporte a impressora DOS.
Irrelevante para o port Genesis.

---

## 21. Tabela Final Completa de Gaps (Revisao Final)

| # | Prioridade | Funcionalidade | Estado |
|---|---|---|---|
| 1 | ALTO | Alterar ordenados proactivamente (Ctrl+F2) | Ausente |
| 2 | MEDIO | Venda directa com preco (Ctrl+F1) | So leilao |
| 3 | MEDIO | Ecra MELHORES MARCADORES completo (Shift+F4) | Parcial (rodape) |
| 4 | MEDIO | Equipas Apuradas/Eliminadas copa | Parcial |
| 5 | MEDIO | Substituicoes F1 no plantel (troca titular<>sub) | Parcial (toggle) |
| 6 | COSMETICO | Nome do Tecnico (entrada texto livre) | Ausente |
| 7 | COSMETICO | INTERVALO / PROLONGAMENTO na simulacao | Parcial |
| 8 | COSMETICO | Bordas graficas CP437 nos ecras | Ausente (usamos tracado) |
| 9 | BAIXO | Proposta de treinador entre equipas IA | Ausente |
| 10 | BAIXO | Evento "treinador deixa de jogar" | Ausente |
| 11 | BAIXO | Vista separada SUPLENTES | Parcial (lista unificada) |
| 12 | BAIXO | Ultimas Receitas (Ctrl+F5) | Ausente |
| 13 | BAIXO | Ultimas Transferencias (Ctrl+F4) | Ausente |
| 14 | BAIXO | Proximas Jornadas (Shift+F7) | Ausente |
| 15 | BAIXO | Calendario completo (Shift+F1) | Ausente |
| 16 | BAIXO | Resultados anteriores (Shift+F2) | Ausente |
| 17 | BAIXO | Titulos das Equipas (Shift+F8) | Ausente |
| 18 | BAIXO | Titulos dos Treinadores (Shift+F9) | Ausente |
| 19 | BAIXO | Ultimos Vencedores (Shift+F5) | Ausente |
| 20 | BAIXO | Premios consultaveis durante temporada | Parcial (so no fim) |
