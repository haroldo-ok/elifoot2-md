# Plano de Port: Elifoot II → Sega Genesis
### Guia técnico completo para SGDK 1.70 — v2

---

## Sumário Executivo

Este documento traça o plano completo para portar o Elifoot II (MS-DOS, ~1994, Turbo Pascal)
para o Sega Genesis usando SGDK 1.70. A filosofia central é **fidelidade funcional com
adaptação de apresentação**: toda a mecânica de jogo é preservada; a interface textual em
modo 80×25 é reconstituída usando o sistema de tiles do VDP do Genesis, com as cores CGA
originais mapeadas para as paletas de hardware com a máxima fidelidade possível.

**Escopo do port:**
- 29 equipes e 464 jogadores embutidos em ROM (arquivo `EQUIPAS.EF2` analisado em v2 da análise).
- 50 treinadores embutidos em ROM (`TREINAD.EF2`).
- 4 divisões, sistema de copa com eliminatórias em 2 mãos.
- Economia completa: salários, bilheteria, transferências por leilão.
- 10 formações táticas.
- 3 slots de save em SRAM (cartucho com bateria, 8 KB).
- Interface textual fiel ao original com cores CGA reproduzidas o mais precisamente possível
  no espaço de cor RGB333 (9 bits) do VDP Genesis.

---

## Índice

1. [Mapeamento de Hardware](#1-mapeamento-de-hardware)
2. [Arquitetura de Software](#2-arquitetura-de-software)
3. [Sistema de Renderização Textual](#3-sistema-de-renderização-textual)
4. [Sistema de Cores: CGA → Genesis (seção expandida)](#4-sistema-de-cores-cga--genesis)
5. [Estruturas de Dados em C](#5-estruturas-de-dados-em-c)
6. [Módulos do Jogo](#6-módulos-do-jogo)
7. [Input: Mapeamento de Controles](#7-input-mapeamento-de-controles)
8. [Sistema de SRAM](#8-sistema-de-sram)
9. [Dados em ROM: rescomp](#9-dados-em-rom-rescomp)
10. [Caveats SGDK 1.70 Aplicados](#10-caveats-sgdk-170-aplicados)
11. [Cronograma de Desenvolvimento](#11-cronograma-de-desenvolvimento)
12. [Estrutura de Diretórios do Projeto](#12-estrutura-de-diretórios-do-projeto)

---

## 1. Mapeamento de Hardware

### 1.1 Recursos do Genesis Disponíveis

| Recurso | Especificação | Uso no Port |
|---|---|---|
| CPU | Motorola 68000 @ 7.67 MHz | Lógica do jogo, simulação |
| RAM | 64 KB | Estado do jogo em memória |
| VRAM | 64 KB | Tiles de fonte + tilemap |
| Paletas VDP | 4 × 16 cores (9 bits por cor, RGB333) | 4 grupos de cores do jogo |
| BG Planes | BG_A (scroll), BG_B (scroll), WINDOW (fixo) | Tela de jogo + UI fixa |
| Resolução | 320×224 px = 40×28 tiles de 8×8 | Grade textual do jogo |
| SRAM | 8 KB (cartucho com bateria) | 3 slots de save |

### 1.2 Mapeamento de Planes

O jogo original usa uma grade textual 80×25. O Genesis oferece 40×28 tiles — metade da
largura, mas com tiles de 8×8px que podem ser combinados com fonte proporcional compacta
para compensar. Com 40 colunas e linhas 0–27, o layout é:

| Plane VDP | Uso | Motivo |
|---|---|---|
| `BG_A` | Conteúdo principal da tela (menus, listas, resultados) | Scrollável, prioridade baixa |
| `WINDOW` | Barras de status fixas (topo e rodapé) | Plano fixo, não scrolla |
| `BG_B` | Cor de fundo sólido por trás do BG_A | Preenche cor de background da tela atual |

**Layout de tela (40 colunas × 28 linhas):**

```
Linha  0: ╔══════════════════════════════════════╗  ← WINDOW
Linha  1: ║ ELIFOOT II  Jornada:12  Div1  $74500 ║    Status bar fixa (status bar)
Linha  2: ╠══════════════════════════════════════╣  ← WINDOW / BG_A começa aqui
Linhas 3–24:                                         ← BG_A (22 linhas de conteúdo)
          ║       CONTEÚDO PRINCIPAL              ║    menus, listas, resultados
Linha 25: ╠══════════════════════════════════════╣
Linha 26: ║  [A]Confirmar  [B]Voltar  [C]Auto    ║  ← WINDOW
Linha 27: ╚══════════════════════════════════════╝    Help bar fixa (context help)
```

**Decisão de design crítica — bordas da janela:** O original usa box-drawing CP437 simples.
No port, as bordas da status bar e do help bar são desenhadas no plane WINDOW; o conteúdo
principal em BG_A usa bordas apenas quando a tela exibe uma sub-caixa (ex: popup de
confirmação). O BG_B é preenchido com um tile sólido da cor de fundo da tela atual (preto
ou azul escuro conforme o contexto), garantindo que áreas sem tile no BG_A mostrem a cor
correta em vez de transparência.

### 1.3 Orçamento de VRAM

```
Tile 0:         vazio (tile transparente/preto — não usar diretamente)
Tiles   1– 96:  Fonte ASCII (96 glifos: chars 32–127)
Tiles  97–109:  Box-drawing CP437 (bordas simples e duplas: ─│┌┐└┘═║╔╗╚╝+)
Tiles 110–120:  Símbolos de posição (GR, DF, MD, AV) + ícones de divisão
Tiles 121–128:  Cursor animado (2 frames) + tiles de logo

Total fonte:    ~128 tiles × 32 bytes = 4.096 bytes de VRAM

Tilemap BG_A:   64×32 tiles × 2 bytes = 4.096 bytes
Tilemap BG_B:   64×32 tiles × 2 bytes = 4.096 bytes
Tilemap WINDOW: 64×32 tiles × 2 bytes = 4.096 bytes
Sprite table:   ~512 bytes (cursor animado)

Total VRAM usada: ~17 KB de 64 KB disponíveis → confortável.
```

---

## 2. Arquitetura de Software

### 2.1 Visão Geral dos Módulos

```
src/
├── main.c              ← Entry point, init, loop principal
├── engine/
│   ├── render.c/h      ← Renderização textual no VDP
│   ├── input.c/h       ← Leitura de controle + edge detection
│   ├── rng.c/h         ← LCG 32 bits (seed via V-counter)
│   ├── compat.c/h      ← memmove, strncmp, atoi — sem guards #ifndef
│   └── sram_io.c/h     ← Serialização/deserialização de save + CRC16
├── game/
│   ├── data.c/h        ← Dados estáticos (equipes, jogadores, treinadores)
│   ├── types.h         ← Structs do jogo (Player, Team, MatchResult…)
│   ├── league.c/h      ← Lógica de campeonato e classificação
│   ├── cup.c/h         ← Lógica de copa (eliminatórias 2 mãos)
│   ├── match.c/h       ← Simulação de partidas
│   ├── economy.c/h     ← Finanças, salários, bilheteria
│   ├── transfer.c/h    ← Transferências e leilão de salário
│   └── season.c/h      ← Coordenação da temporada
└── screens/
    ├── title.c/h       ← Tela de título / seleção de time
    ├── main_menu.c/h   ← Menu principal pré-rodada
    ├── squad.c/h       ← Gestão de plantel / formação
    ├── results.c/h     ← Tela de resultados da rodada
    ├── standings.c/h   ← Classificação
    ├── finances.c/h    ← Telas financeiras (salários, receitas)
    ├── transfers.c/h   ← Telas de transferência e leilão
    ├── palmares.c/h    ← Palmares e histórico (últimas 5 temporadas)
    └── save_load.c/h   ← Telas de save/load (3 slots)
```

### 2.2 Loop Principal

```c
// main.c
u16 main(u16 hardReset) {
    // --- INIT (ordem obrigatória) ---
    JOY_init();                    // CRÍTICO: PRIMEIRA chamada sempre
    VDP_init();
    render_init();                 // carrega fonte, paletas, configura planes
    rng_init((u16)GET_VCOUNTER()); // semente: variabilidade por momento de ligar

    data_init();                   // descompacta ROM → RAM (equipes, jogadores)

    // --- TELA DE TÍTULO ---
    screen_title();                // seleção de equipe ou continua save

    // --- LOOP DE TEMPORADA ---
    while (TRUE) {
        season_run();
        screen_season_end();       // prêmios, promoção/rebaixamento
    }

    return 0;
}
```

### 2.3 Máquina de Estados das Telas

```c
typedef enum {
    SCREEN_TITLE,
    SCREEN_MAIN_MENU,
    SCREEN_SQUAD,
    SCREEN_RESULTS,
    SCREEN_STANDINGS,
    SCREEN_CALENDAR,
    SCREEN_FINANCES_SALARIES,
    SCREEN_FINANCES_REVENUES,
    SCREEN_TRANSFERS,
    SCREEN_PALMARES,
    SCREEN_STADIUM,
    SCREEN_COACHES,
    SCREEN_SAVE,
    SCREEN_CUP_DRAW,
    SCREEN_SEASON_END,
} GameScreen;
```

### 2.4 Transição de Telas

```c
void screen_transition(GameScreen new_screen) {
    VDP_clearPlane(BG_A,   TRUE);  // limpa conteúdo principal
    VDP_clearPlane(WINDOW, TRUE);  // CRÍTICO: evita lixo de texto no WINDOW
    // BG_B: raramente precisa limpar — a cor de fundo é reescrita por render_set_bg_color()
    current_screen = new_screen;
    render_status_bar();           // redesenha WINDOW (linhas 0-2 e 25-27)
}
```

---

## 3. Sistema de Renderização Textual

### 3.1 Estratégia Central: Fonte Customizada com `VDP_setTileMapXY()`

O SGDK oferece `VDP_drawText()`, mas ela **só escreve no plane WINDOW** com paleta fixa.
O port usa texto colorido em múltiplos planes, exigindo escrever tiles diretamente via
`VDP_setTileMapXY()` com o **tile attribute word** controlando paleta e prioridade.

O tile attribute word do VDP Genesis (16 bits):

```
Bit 15:      Prioridade (0=baixa, 1=alta)
Bits 14–13:  Índice de paleta (0=PAL0, 1=PAL1, 2=PAL2, 3=PAL3)
Bit 12:      Flip vertical
Bit 11:      Flip horizontal
Bits 10–0:   Índice do tile em VRAM (0–2047)
```

Isso significa que o **mesmo glifo** na VRAM pode ser renderizado em qualquer das 4 paletas
sem duplicar tiles — apenas o atributo de paleta muda no tilemap. É assim que o texto branco
sobre preto e o texto preto sobre ciano (cursor de seleção) usam os mesmos glifos.

### 3.2 API de Renderização (`engine/render.h`)

```c
// Constantes de base
#define FONT_BASE_TILE    1      // tiles 1..96  = ASCII 32..127
#define BOX_BASE_TILE    97      // tiles 97..109 = box-drawing CP437
#define PAL_IDX_SHIFT    13

// Converte char ASCII para índice de tile na VRAM
#define CHAR_TO_TILE(c)  (FONT_BASE_TILE + (u16)((u8)(c) - 32))

// Monta o tile attribute word
#define TILE_ATTR(tile, pal, prio) \
    ( ((u16)(prio) << 15) | ((u16)(pal) << PAL_IDX_SHIFT) | (u16)(tile) )

// ---- API pública ----

// Escreve string com paleta especificada
void render_text(VDPPlane plane, const char *str,
                 u16 x, u16 y, u16 pal_idx);

// Escreve string formatada
void render_textf(VDPPlane plane, u16 x, u16 y, u16 pal_idx,
                  const char *fmt, ...);

// Preenche região retangular com tile de background (limpa área)
void render_clear_rect(VDPPlane plane, u16 x, u16 y, u16 w, u16 h);

// Desenha caixa com bordas usando tiles 97-109
void render_box(VDPPlane plane, u16 x, u16 y, u16 w, u16 h,
                u16 pal_idx);

// Define cor de fundo da tela (preenche BG_B com tile sólido)
void render_set_bg_color(u16 bg_pal_color_idx);
```

### 3.3 Implementação de `render_text()`

```c
// engine/render.c
void render_text(VDPPlane plane, const char *str,
                 u16 x, u16 y, u16 pal_idx) {
    u16 cx = x;
    const char *p = str;
    while (*p) {
        u8 c = (u8)*p++;
        if (c < 32 || c > 127) { cx++; continue; } // skip fora do range ASCII
        u16 tile_idx = CHAR_TO_TILE(c);
        u16 attr = TILE_ATTR(tile_idx, pal_idx, 0);
        VDP_setTileMapXY(plane, attr, cx, y);
        cx++;
        if (cx >= 40) break; // não escrever fora dos 40 tiles de largura
    }
}
```

### 3.4 Fonte: Arquivo PNG e Carregamento

A fonte é um PNG de 8×768px (8×8px por glifo × 96 glifos), indexado em **2 cores**:
- cor índice 0 = transparente (background — cor viria da paleta)
- cor índice 1 = foreground (o pixel do glifo em si)

A cor real em tela é sempre determinada pela **paleta selecionada no tile attribute**, não
pelo PNG em si. Isso é o que permite texto branco, amarelo, ciano, verde etc. com o mesmo
arquivo de fonte.

```
# res/resources.res
TILESET font_tiles "gfx/font_cp850.png" NONE NONE
```

```c
// Carrega fonte no VDP durante init
VDP_loadTileSet(&font_tiles, FONT_BASE_TILE, DMA);
```

**Fonte recomendada:** IBM PC codepage 437/850, 8×8px, export como PNG 1bpp indexado.
Ferramentas: `mkfont` (SGDK), ou qualquer editor bitmap + script Python para gerar os
32 bytes/tile manualmente.

### 3.5 Tiles de Box-Drawing (CP437 → tiles customizados)

O original usa box-drawing CP437 extensivamente para bordas de janelas e separadores.
No Genesis, esses caracteres são mapeados para tiles customizados (97–109) que precisam
ser desenhados pixel a pixel no PNG da fonte:

| Tile | Char CP437 | Byte | Uso |
|---|---|---|---|
| 97  | `─` | 0xC4 | Linha horizontal simples |
| 98  | `│` | 0xB3 | Linha vertical simples |
| 99  | `┌` | 0xDA | Canto superior esq. simples |
| 100 | `┐` | 0xBF | Canto superior dir. simples |
| 101 | `└` | 0xC0 | Canto inferior esq. simples |
| 102 | `┘` | 0xD9 | Canto inferior dir. simples |
| 103 | `═` | 0xCD | Linha horizontal dupla |
| 104 | `║` | 0xBA | Linha vertical dupla |
| 105 | `╔` | 0xC9 | Canto duplo superior esq. |
| 106 | `╗` | 0xBB | Canto duplo superior dir. |
| 107 | `╚` | 0xC8 | Canto duplo inferior esq. |
| 108 | `╝` | 0xBC | Canto duplo inferior dir. |
| 109 | `┼` / `+` | 0xC5 | Cruzamento de linhas |

```c
// Constantes para render_box()
#define BOX_SIMPLE  0  // usa tiles 97-102 (bordas simples)
#define BOX_DOUBLE  1  // usa tiles 103-108 (bordas duplas)

void render_box(VDPPlane plane, u16 x, u16 y, u16 w, u16 h,
                u16 pal_idx) {
    u16 base = (pal_idx == PAL_SELECTED) ? 103 : 97; // dupla para status bar
    // canto superior esq.
    VDP_setTileMapXY(plane, TILE_ATTR(base+2, pal_idx, 0), x, y);
    // linha horizontal superior
    for (u16 cx = x+1; cx < x+w-1; cx++)
        VDP_setTileMapXY(plane, TILE_ATTR(base+0, pal_idx, 0), cx, y);
    // ... (padrão completo implementado em render.c)
}
```

---

## 4. Sistema de Cores: CGA → Genesis

Esta é a seção mais crítica para fidelidade visual. O Elifoot II roda em modo texto CGA
do DOS com atributos de **4 bits de foreground + 4 bits de background = 256 combinações
possíveis**, embora o jogo use apenas ~15 combinações distintas. O VDP Genesis tem 9 bits
por cor (RGB333), e 4 paletas de 16 cores cada — 64 cores no total.

### 4.1 O Espaço de Cor do VDP Genesis

O VDP Genesis usa o formato de word 16 bits:

```
0000 0BBB 0GGG 0RRR
```

Cada canal (R, G, B) tem 3 bits → 8 níveis (0–7). Os bits nas posições 3, 7, 11 são sempre
zero (ignorados pelo hardware). Os valores mapeiam para voltagem analógica de forma
razoavelmente linear, mas os 8 níveis de cada canal não cobrem exatamente o range 0–255
do monitor. Aproximação prática:

| Valor de canal (3 bits) | Código hex Genesis | Intensidade aproximada (0–255) |
|---|---|---|
| 0 | `0` | 0 |
| 1 | `2` | 52 |
| 2 | `4` | 87 |
| 3 | `6` | 116 |
| 4 | `8` | 144 (não usado normalmente) |
| 5 | `A` | 172 |
| 6 | `C` (não existe — canal só vai a 7) | — |
| 7 | `E` | 255 |

Na prática, para o propósito deste port, usa-se apenas os níveis 0, 2 (≈meio-escuro), 6
(≈meio-claro), e E (≈máximo), que correspondem bem ao sistema de cores CGA.

### 4.2 Tabela Canônica: 16 Cores CGA → Valores Genesis (RGB333)

Esta tabela é a referência definitiva. Os valores Genesis são baseados na análise do arquivo
`elifoot2_analise.md` (Seção 17.6), corrigidos para o formato de word exato do VDP:

> **Nota de formato:** O VDP Genesis usa `0x0BGR` onde cada canal ocupa 4 bits com o bit 3
> sempre zero. Ou seja: bits [3:0]=R, bits [7:4]=G, bits [11:8]=B.
> Exemplo: Azul puro máximo = `0x0E00` (B=E, G=0, R=0).

| Idx | Nome CGA | RGB CGA Canônico | R (3b) | G (3b) | B (3b) | Valor Genesis | Nota |
|---|---|---|---|---|---|---|---|
| 0  | Preto | `#000000` | 0 | 0 | 0 | `0x0000` | Background padrão |
| 1  | Azul escuro | `#0000AA` | 0 | 0 | 6 | `0x0600` | B=6, G=0, R=0 |
| 2  | Verde escuro | `#00AA00` | 0 | 6 | 0 | `0x0060` | |
| 3  | Ciano escuro | `#00AAAA` | 0 | 6 | 6 | `0x0660` | |
| 4  | Vermelho escuro | `#AA0000` | 6 | 0 | 0 | `0x0006` | |
| 5  | Magenta escuro | `#AA00AA` | 6 | 0 | 6 | `0x0606` | |
| 6  | Marrom/laranja | `#AA5500` | 6 | 2 | 0 | `0x0026` | G=2 (≈0x55) |
| 7  | Cinza claro | `#AAAAAA` | 6 | 6 | 6 | `0x0666` | |
| 8  | Cinza escuro | `#555555` | 2 | 2 | 2 | `0x0222` | Intensidade média-baixa |
| 9  | Azul brilhante | `#5555FF` | 2 | 2 | E | `0x0E22` | |
| 10 | Verde brilhante | `#55FF55` | 2 | E | 2 | `0x02E2` | |
| 11 | Ciano brilhante | `#55FFFF` | 2 | E | E | `0x0EE2` | Usado em cabeçalhos |
| 12 | Vermelho brilhante | `#FF5555` | E | 2 | 2 | `0x022E` | Usado em erros/derrotas |
| 13 | Magenta brilhante | `#FF55FF` | E | 2 | E | `0x0E2E` | |
| 14 | Amarelo | `#FFFF55` | E | E | 2 | `0x02EE` | Títulos, destaques |
| 15 | Branco | `#FFFFFF` | E | E | E | `0x0EEE` | Texto padrão |

> **Atenção — erro comum:** O format word do VDP Genesis é `0x0BGR` (Blue nos bits altos),
> **não** `0x0RGB`. Azul escuro CGA (`#0000AA`) tem apenas canal B ativo → `0x0600`
> (B=6 nos bits 11:8), não `0x0006`. Conferir sempre antes de escrever valores literais.

### 4.3 Análise de Uso de Cores no Jogo Original

Com base na análise comportamental (`elifoot2_analise.md`, Seção 13 e 17.6), o Elifoot II
usa as seguintes combinações fg/bg de forma consistente:

**Paleta de background dominante:** Azul escuro (CGA 1) ou Preto (CGA 0).
**Texto dominante:** Branco (CGA 15) ou Amarelo (CGA 14).
**Cabeçalhos e subtítulos:** Ciano brilhante (CGA 11).
**Valores positivos (dinheiro, vitórias, gols marcados):** Verde brilhante (CGA 10).
**Valores negativos (dívida, derrotas):** Vermelho brilhante (CGA 12).
**Cursor / item selecionado:** Preto (CGA 0) sobre Ciano escuro (CGA 3) — inversão de vídeo.
**Status bar fixa:** Preto (CGA 0) sobre Ciano escuro (CGA 3).
**Alertas / avisos:** Branco (CGA 15) sobre Vermelho escuro (CGA 4).
**Bordas de janela:** Ciano escuro (CGA 3) sobre background da tela.
**Texto desabilitado / comentário:** Cinza claro (CGA 7) sobre preto.

### 4.4 Distribuição nas 4 Paletas Genesis

O VDP tem 4 paletas de 16 cores cada. A estratégia é organizar as combinações mais usadas
de forma que **cada paleta cubra um conjunto coeso de foreground+background**, sendo
selecionável pelo atributo do tile.

#### PAL0 — Interface Geral (texto sobre fundo azul ou preto)

Esta é a paleta mais usada. Cobre a maioria dos textos sobre fundo azul escuro ou preto.

```c
// gfx/palette_main.png deve ter exatamente estas 16 cores na ordem:
u16 pal0[16] = {
    // Cores de background (usadas como bg via BG_B ou tile sólido)
    0x0000,  // [0]  Preto        — background padrão da maioria das telas
    0x0600,  // [1]  Azul escuro  — background alternativo (tela de título, menu principal)

    // Cores de foreground (texto)
    0x0EEE,  // [2]  Branco       — texto padrão, a cor mais frequente
    0x02EE,  // [3]  Amarelo      — títulos de tela, cabeçalhos de seção
    0x0EE2,  // [4]  Ciano brilh. — subtítulos, nomes de equipe adversária, info
    0x02E2,  // [5]  Verde brilh. — valores positivos ($, vitórias, gols marcados)
    0x022E,  // [6]  Vermelho br. — valores negativos (dívida, derrotas, gols sofridos)
    0x0666,  // [7]  Cinza claro  — texto secundário, desabilitado, comentários
    0x0606,  // [8]  Magenta      — alertas de cup/especiais
    0x0660,  // [9]  Ciano escuro — bordas de caixa, separadores
    0x0222,  // [10] Cinza escuro — texto muito secundário
    0x0E2E,  // [11] Magenta br.  — reserva
    0x0026,  // [12] Marrom       — reserva (raramente usado no original)
    0x0060,  // [13] Verde escuro — reserva
    0x0E00,  // [14] Azul brilh.  — reserva
    0x0EEE,  // [15] Branco (dup) — tile de cursor piscando
};
```

**Observação importante:** O índice [0] de cada paleta é a **cor de transparência** do
tile — pixels transparentes do glifo mostram esta cor. Portanto PAL0[0] = preto (0x0000)
significa que o background de qualquer texto em PAL0 é preto. Para texto sobre azul escuro,
o BG_B precisa estar preenchido com a cor azul, ou os tiles devem usar PAL0[1] como
background via tile sólido colocado em BG_B antes dos tiles de texto em BG_A.

#### PAL1 — Seleção / Cursor / Status Bar (texto sobre ciano)

Usada para o item selecionado em listas e para as barras de status fixas no WINDOW.
O original usa "preto sobre ciano" para indicar seleção — PAL1 implementa exatamente isso.

```c
u16 pal1[16] = {
    // [0] = background da paleta = ciano escuro (a cor dominante aqui é o BG)
    0x0660,  // [0]  Ciano escuro — background do cursor/status bar
    0x0000,  // [1]  Preto        — texto sobre ciano (inversão de vídeo)
    0x0EEE,  // [2]  Branco       — texto secundário sobre ciano (contraste menor)
    0x02EE,  // [3]  Amarelo      — destaque sobre ciano (ex: dinheiro na status bar)
    0x02E2,  // [4]  Verde brilh. — valores positivos sobre ciano
    0x022E,  // [5]  Vermelho br. — valores negativos sobre ciano
    0x0EE2,  // [6]  Ciano brilh. — reserva
    0x0EEE,  // [7]  Branco       — reserva
    // [8-15]: copias para uso futuro
    0x0660, 0x0000, 0x0EEE, 0x02EE,
    0x02E2, 0x022E, 0x0EE2, 0x0EEE,
};
```

**Como usar PAL1 para o cursor de seleção:**

A linha selecionada em uma lista é renderizada com `pal_idx = 1` em vez de `pal_idx = 0`.
O fundo ciano vem de PAL1[0], e o texto preto de PAL1[1]. Não é necessário tiles separados
— os mesmos glifos da VRAM, apenas com o atributo de paleta = 1.

```c
// Em qualquer screen com lista:
for (u8 i = 0; i < visible_count; i++) {
    u16 pal = (i == nav.selected) ? PAL1 : PAL0;
    u16 fg  = (i == nav.selected) ? 1    : 2;   // preto (PAL1[1]) ou branco (PAL0[2])
    // Nota: fg não entra diretamente — a paleta determina as cores automaticamente.
    // Basta usar pal_idx correto em render_text():
    render_text(BG_A, team_name[i], 2, 4 + i, pal);
}
```

#### PAL2 — Equipe do Jogador (cor primária customizável)

Reservada para destacar a equipe do jogador humano. Pode ser preenchida com a cor da
camisa da equipe selecionada (extensão opcional do port). Por padrão, é igual a PAL0
mas com as primeiras duas cores trocadas para dar identidade visual à equipe do jogador.

```c
// Inicialização padrão — pode ser customizada por equipe
u16 pal2[16];
memcpy(pal2, pal0, 32); // copia PAL0 como base
// Customizar pal2[0] e pal2[1] com a cor da equipe selecionada
```

#### PAL3 — Equipe Adversária / Copa

Análoga à PAL2, para o adversário principal. Também usada em telas de copa para
diferenciar os dois times.

### 4.5 Carregamento das Paletas no Init

```c
// engine/render.c — render_init()
void render_init(void) {
    // Paletas definidas como arrays estáticos (ver Seção 4.4)
    PAL_setPalette(PAL0, pal0_data, DMA);
    PAL_setPalette(PAL1, pal1_data, DMA);
    PAL_setPalette(PAL2, pal2_data, DMA);
    PAL_setPalette(PAL3, pal3_data, DMA);

    // Carrega fonte na VRAM
    VDP_loadTileSet(&font_tiles, FONT_BASE_TILE, DMA);

    // Configura BG_B com tile preto (tile 0, PAL0) para background padrão
    VDP_clearPlane(BG_B, FALSE);

    // WINDOW: limpa antes de usar
    VDP_clearPlane(WINDOW, TRUE);
}
```

**Alternativa via rescomp:** As paletas podem ser definidas em PNGs indexados de 16px × 1px
e carregadas via `PALETTE` no `.res`. Isso permite editar as cores visualmente em qualquer
editor de imagem, mas o controle é menos explícito. Recomenda-se arrays C para as paletas
0 e 1 (críticas para fidelidade) e PNG para as paletas 2 e 3 (customizáveis por equipe).

### 4.6 Constantes Semânticas de Cor e Sua Correspondência CGA

Para tornar o código de UI legível e manutenível, definir constantes semânticas que mapeiam
diretamente às cores do jogo original:

```c
// engine/render.h — constantes semânticas de cor
// Formato: RENDER_COLOR_<NOME> = (pal_idx << 4) | fg_entry_in_pal
// (pal_idx nos bits altos, índice na paleta nos bits baixos)
// A função render_text() decodifica isso internamente.

// Sobre fundo preto (PAL0, background = PAL0[0] = preto)
#define COLOR_NORMAL       0x00  // PAL0, fg=PAL0[2]=branco  — texto padrão
#define COLOR_TITLE        0x01  // PAL0, fg=PAL0[3]=amarelo — títulos de tela
#define COLOR_INFO         0x02  // PAL0, fg=PAL0[4]=ciano   — informações, subtítulos
#define COLOR_POSITIVE     0x03  // PAL0, fg=PAL0[5]=verde   — valores positivos
#define COLOR_NEGATIVE     0x04  // PAL0, fg=PAL0[6]=vermelho— valores negativos
#define COLOR_SECONDARY    0x05  // PAL0, fg=PAL0[7]=cinza   — texto desabilitado
#define COLOR_ALERT        0x06  // PAL0, fg=PAL0[8]=magenta — alertas
#define COLOR_BORDER       0x07  // PAL0, fg=PAL0[9]=ciano e.— bordas de caixa

// Sobre fundo ciano (PAL1, background = PAL1[0] = ciano escuro)
#define COLOR_SELECTED     0x10  // PAL1, fg=PAL1[1]=preto   — item selecionado (cursor)
#define COLOR_STATUS_BAR   0x10  // PAL1, idem — status bar no WINDOW
#define COLOR_STATUS_MONEY 0x13  // PAL1, fg=PAL1[3]=amarelo — dinheiro na status bar
#define COLOR_STATUS_POS   0x14  // PAL1, fg=PAL1[4]=verde   — posição na classificação
```

A função `render_text()` expandida aceita a constante semântica e decodifica internamente:

```c
// Versão simplificada: pal_idx vai diretamente para o tile attribute
// A constante semântica apenas documenta a intenção — o caller passa pal_idx (0-3)
// e fg_entry é implícito pela paleta definida.
// Isso simplifica a implementação: render_text(plane, str, x, y, PAL0)
// O foreground é sempre determinado pela paleta carregada no hardware.
```

### 4.7 Mapeamento de Cores por Elemento de UI

Esta tabela é a referência de implementação para cada `render_text()` chamada nas telas.

| Elemento de UI | Fg CGA | Bg CGA | Fg Genesis | Bg Genesis | Paleta | Notas |
|---|---|---|---|---|---|---|
| Texto normal | Branco (15) | Preto (0) | `0x0EEE` | `0x0000` | PAL0 | Todo texto padrão |
| Títulos de tela | Amarelo (14) | Preto (0) | `0x02EE` | `0x0000` | PAL0 | Ex: "CLASSIFICAÇÃO" |
| Subtítulos / info | Ciano br. (11) | Preto (0) | `0x0EE2` | `0x0000` | PAL0 | Ex: "Jornada 12" |
| Valores positivos | Verde br. (10) | Preto (0) | `0x02E2` | `0x0000` | PAL0 | Saldo, vitórias |
| Valores negativos | Vermelho br. (12) | Preto (0) | `0x022E` | `0x0000` | PAL0 | Dívida, derrotas |
| Texto secundário | Cinza claro (7) | Preto (0) | `0x0666` | `0x0000` | PAL0 | Comentários, ajuda |
| Bordas de janela | Ciano esc. (3) | Preto (0) | `0x0660` | `0x0000` | PAL0 | `render_box()` |
| Alerta/popup | Branco (15) | Vermelho esc. (4) | `0x0EEE` | `0x0006` | PAL0+BG_B* | |
| **Item selecionado** | **Preto (0)** | **Ciano esc. (3)** | **`0x0000`** | **`0x0660`** | **PAL1** | Cursor de seleção |
| Status bar (WINDOW) | Preto (0) | Ciano esc. (3) | `0x0000` | `0x0660` | PAL1 | Linhas 0-2 e 25-27 |
| Dinheiro na status | Amarelo (14) | Ciano esc. (3) | `0x02EE` | `0x0660` | PAL1 | Destaque em PAL1 |
| Nome da equipe do player | Amarelo (14) | Azul esc. (1) | `0x02EE` | `0x0600` | PAL2 | Tela de título |
| Nome do adversário | Ciano br. (11) | Azul esc. (1) | `0x0EE2` | `0x0600` | PAL3 | Tela de resultados |

`*` Para "alerta sobre vermelho": o BG_B é temporariamente preenchido com tile sólido
vermelho (usando um tile monocromático com todos pixels = cor 1 em PAL0, onde PAL0[6]
= vermelho brilhante), e o texto é renderizado em PAL0 com fg=branco.

### 4.8 Como Renderizar Texto sobre Fundo Colorido (Azul ou Ciano)

O VDP Genesis não tem um mecanismo direto de "background de tile". A cor de fundo de um
glifo é sempre PAL_N[0] (o primeiro índice da paleta do tile). Para ter texto sobre fundo
colorido:

**Opção A (BG_B):** Preencher o plane BG_B inteiro com a cor desejada. O BG_A fica por
cima com os tiles de texto (os pixels transparentes "vêem" o BG_B). Esta é a abordagem
para fundos de tela inteira (ex: tela de título sobre azul escuro).

```c
// Para tela com background azul escuro:
// 1. Colocar um tile sólido "all-ones" em BG_B — todos pixels = cor 1 de PAL0
//    onde PAL0[1] = 0x0600 (azul escuro)
// 2. Renderizar texto em BG_A com PAL0 (PAL0[0] = transparente, PAL0[2] = branco)
// Resultado: BG_A mostra texto branco, BG_B mostra fundo azul
render_fill_plane(BG_B, TILE_SOLID_PAL0_1, 0, 0, 40, 28);
render_text(BG_A, "ELIFOOT II", 14, 10, PAL0);
```

**Opção B (tile sólido como background local):** Para uma região específica (ex: uma
popup de alerta de 20×5 tiles), desenhar tiles sólidos em BG_A antes do texto. Um tile
sólido é um tile com todos os 8×8 pixels = valor 1 da paleta. Manter tiles sólidos
para cada cor de fundo usada (preto, azul, ciano, vermelho) nos slots 120–127 da VRAM.

```c
// Tile sólido de vermelho para popup de erro:
// 1. VRAM[120] = tile com todos pixels = cor 6 de PAL0 (vermelho brilhante)
// 2. Preencher região do popup com TILE_ATTR(120, PAL0, 1) em BG_A (prioridade alta)
// 3. Escrever texto em BG_A com PAL0 sobre a região (os tiles sólidos ficam "atrás"
//    dos tiles de texto na mesma prioridade? Não — escrever texto SOBRE os sólidos)
// Nota: na verdade escrever os tiles de texto substitui os tiles sólidos no mesmo xy.
// A solução correta é: para popup de erro, usar PAL com [0]=vermelho como bg.
```

> **Simplificação recomendada para o port:** As duas situações de fundo não-preto
> mais frequentes são (1) status bar em ciano (PAL1) e (2) tela de título em azul
> (BG_B). Em todos os outros casos o fundo é preto. Isso cobre >95% dos casos sem
> precisar de lógica de "tile sólido". A popup de alerta (caso raro) pode simplesmente
> usar uma borda vermelha em PAL0[6] em vez de fundo vermelho sólido.

### 4.9 Estratégia para a Status Bar (WINDOW)

A status bar nas linhas 0–2 e 25–27 usa PAL1 (ciano com texto preto/amarelo). O plane
WINDOW nunca scrolla e tem prioridade sobre BG_A e BG_B, então é ideal para isso.

```c
void render_status_bar(void) {
    // Linha 0: borda superior dupla
    render_box_line(WINDOW, 0, BOX_TOP, PAL1);  // ╔═══...═══╗

    // Linha 1: conteúdo da status bar
    render_textf(WINDOW, 1, 1, PAL1,
        "ELIFOOT II  Jorn:%02u  Div%u  $%-6ld",
        g_season.round,
        g_teams[g_season.player_team].division + 1,
        g_teams[g_season.player_team].money);
    // "ELIFOOT II" em branco (PAL1[2]), dinheiro em amarelo (PAL1[3])
    // Para cores diferentes na mesma linha, usar múltiplos render_text():
    render_text(WINDOW, "ELIFOOT II", 1, 1, PAL1);           // branco
    render_textf(WINDOW, 12, 1, PAL1, "Jorn:%02u", round);   // branco
    render_textf(WINDOW, 24, 1, PAL1, "Div%u", div+1);       // branco
    // Dinheiro em amarelo: precisa de outra chamada com fg=amarelo
    // Isso exige uma PAL1 variant ou render_text_fg(plane,str,x,y,pal,fg_idx)

    // Linha 2: borda separadora
    render_box_line(WINDOW, 2, BOX_SEPARATOR, PAL1);  // ╠═══...═══╣

    // Linha 25: borda separadora
    render_box_line(WINDOW, 25, BOX_SEPARATOR, PAL1);

    // Linha 26: help bar
    render_text(WINDOW, "[A]Confirmar  [B]Voltar  [C]Auto", 4, 26, PAL1);

    // Linha 27: borda inferior
    render_box_line(WINDOW, 27, BOX_BOTTOM, PAL1);   // ╚═══...═══╝
}
```

**Problema de fg granular na mesma linha:** Para ter "ELIFOOT II" em branco e "$74500"
em amarelo na mesma linha do WINDOW, o tile attribute de cada tile tem sua própria
paleta. A solução é estender `render_text()` para aceitar um `fg_entry` além de `pal_idx`:

```c
// Versão extendida: pal_idx seleciona a paleta, fg_entry seleciona qual entrada
// DENTRO da paleta é usada como foreground. O background é sempre pal[0].
// Isso não muda o tile na VRAM — apenas o atributo de paleta no tilemap.
// Como todos os tiles usam "cor 1" como foreground pelo bitmap da fonte,
// isso não funciona diretamente — a fonte tem apenas 2 cores (0=bg, 1=fg).
//
// SOLUÇÃO: Ter sub-paletas: PAL1A (fg=preto) e PAL1B (fg=amarelo).
// Mas só temos 4 paletas no total. Alternativa: usar PAL0 com [0]=ciano para
// segmentos amarelos sobre ciano, e PAL1 para segmentos pretos sobre ciano.

// Paleta auxiliar para texto amarelo sobre ciano (na status bar):
// PAL0[0] = ciano (background), PAL0[3] = amarelo (foreground)
// Isso consome PAL0 para esse uso específico — conflito com uso geral.
//
// RECOMENDAÇÃO FINAL: Para simplificar, usar PAL1 para toda a status bar
// com fg=preto (PAL1[1]), e destacar valores monetários com PAL2 onde
// PAL2[0]=ciano e PAL2[1]=amarelo. Isso usa 3 paletas mas mantém a fidelidade.
```

### 4.10 Ajuste de Marrom/Laranja (CGA 6)

A cor CGA 6 (marrom/laranja, `#AA5500`) é raramente usada no Elifoot II. O valor Genesis
`0x0026` (R=6, G=2, B=0) produz um laranja amarronzado razoável. Se necessário, pode-se
usar `0x0046` (R=6, G=4, B=0) para um resultado um pouco mais alaranjado — experimentar
visualmente no emulador.

### 4.11 Script Python de Validação de Cores

Para conferir se os valores Genesis produzem a cor pretendida, usar este script no PC:

```python
def genesis_to_rgb(v):
    """Converte valor de paleta Genesis (0x0BGR) para RGB888."""
    r3 = (v >> 0) & 0x7
    g3 = (v >> 4) & 0x7
    b3 = (v >> 8) & 0x7
    # Cada nível de 3 bits mapeia para: 0->0, 1->36, 2->73, 3->109,
    # 4->146, 5->182, 6->219, 7->255 (aproximação linear)
    scale = [0, 36, 73, 109, 146, 182, 219, 255]
    return (scale[r3], scale[g3], scale[b3])

# Tabela completa
cga_to_genesis = {
    0:  0x0000,  # Preto
    1:  0x0600,  # Azul escuro    (B=6)
    2:  0x0060,  # Verde escuro   (G=6)
    3:  0x0660,  # Ciano escuro   (G=6, B=6)
    4:  0x0006,  # Vermelho escuro(R=6)
    5:  0x0606,  # Magenta escuro (R=6, B=6)
    6:  0x0026,  # Marrom         (R=6, G=2)
    7:  0x0666,  # Cinza claro    (R=6, G=6, B=6)
    8:  0x0222,  # Cinza escuro   (R=2, G=2, B=2)
    9:  0x0E22,  # Azul brilhante (R=2, G=2, B=E)
    10: 0x02E2,  # Verde brilhante(R=2, G=E, B=2)  -- atenção: E=7 em hex 0..7
    11: 0x0EE2,  # Ciano brilhante(R=2, G=E, B=E)
    12: 0x022E,  # Vermelho br.   (R=E, G=2, B=2)
    13: 0x0E2E,  # Magenta br.    (R=E, G=2, B=E)
    14: 0x02EE,  # Amarelo        (R=E, G=E, B=2)
    15: 0x0EEE,  # Branco         (R=E, G=E, B=E)
}

# Nota: no formato Genesis, "E" = 7 em binário 3-bit (111 = máximo).
# O nibble "E" em hex = 14 decimal, mas no formato 0x0BGR cada nibble
# representa os 4 bits incluindo o bit 3 que o hardware ignora.
# Valores válidos por nibble: 0,2,4,6,8,A,C,E (bit 0 do nibble é o LSB do canal).
# Na prática, para CGA: usar 0 (nível 0) e 6 (nível ≈67%) e E (nível 100%).

for cga_idx, genesis_val in cga_to_genesis.items():
    rgb = genesis_to_rgb(genesis_val)
    print(f"CGA {cga_idx:2d}: Genesis 0x{genesis_val:04X} → RGB{rgb}")
```

---

## 5. Estruturas de Dados em C

### 5.1 Tipos Fundamentais

**ATENÇÃO: `int` = 16 bits no m68k (SGDK/GCC `-m68000`).** Esta é a armadilha mais
importante do toolchain. Todo valor monetário usa `long` (32 bits).

```c
// game/types.h
#ifndef ELIFOOT_TYPES_H
#define ELIFOOT_TYPES_H

#include <genesis.h>

typedef enum {
    POS_GR = 0,
    POS_DF = 1,
    POS_MD = 2,
    POS_AV = 3,
} PlayerPos;

typedef struct {
    char  name[15];    // 14 chars + null — nomes transliterados (CP850→ASCII)
    u8    pos;         // PlayerPos
    u8    nat;         // índice de nacionalidade (0=POR, 1=BRA, ...)
    u8    strength;    // força 1–99
    long  salary;      // ordenado mensal — 32 bits (pode ser > 32767)
    u8    on_field;    // 1=titular, 0=banco
} Player;

typedef struct {
    char  name[15];
    u8    league_idx;
    u8    n1;              // pote de sorteio da copa
    u8    n2;              // fator de orçamento/força inicial
    u8    player_count;    // jogadores no plantel (sempre 16 no arquivo original)
    u8    player_start;    // índice em g_players[] do primeiro jogador
    u8    formation;       // formação 0–9
    u8    division;        // 0=Div1, 1=Div2, 2=Div3, 3=Div4
    long  money;           // 32 bits
    long  salary_total;    // soma de salários — 32 bits
    u16   stadium_cap;
    u16   ticket_price;
    u8    coach_idx;
    u8    wins, draws, losses;
    u8    goals_for, goals_against;
    u16   points;
    u8    cup_active;
} Team;

typedef struct {
    u8 home_team, away_team;
    u8 home_goals, away_goals;
    u8 round;
    u8 division;
} MatchResult;

typedef struct {
    u8 team_a, team_b;
    u8 goals_a_leg1, goals_b_leg1;
    u8 goals_a_leg2, goals_b_leg2;
    u8 phase;    // 0=eliminatórias, 1=quartos, 2=meias, 3=final
} CupTie;

typedef struct {
    u8 season;       // número da temporada (0–99)
    u8 round;        // jornada atual
    u8 player_team;  // índice da equipe do jogador
    u8 cup_phase;    // fase atual da copa
} SeasonState;

#endif
```

### 5.2 Arrays Globais em RAM

```c
// game/data.h
extern Player      g_players[464];    // 29 equipes × 16 = 464 jogadores
                                       // 464 × 24 bytes ≈ 11 KB
extern Team        g_teams[29];       // 29 equipes × ~48 bytes ≈ 1.4 KB
extern char        g_coaches[50][16]; // 50 × 16 = 800 bytes
extern MatchResult g_results[200];    // 200 × 6 = 1.2 KB
extern CupTie      g_cup_ties[16];    // 16 × 8 = 128 bytes
extern SeasonState g_season;

// Total estimado: ~14 KB de 64 KB disponíveis → confortável
```

---

## 6. Módulos do Jogo

### 6.1 `match.c` — Simulação de Partidas

```c
static u16 calc_team_strength(u8 team_idx) {
    Team *t = &g_teams[team_idx];
    u16 total = 0, count = 0;
    for (u16 i = t->player_start; i < t->player_start + t->player_count; i++) {
        if (g_players[i].on_field) {
            total += g_players[i].strength;
            count++;
        }
    }
    if (count == 0) return 1;
    return total / count;
}

MatchResult match_simulate(u8 home, u8 away) {
    MatchResult r;
    r.home_team = home;
    r.away_team = away;

    u16 str_h = calc_team_strength(home) + 5;  // bônus de casa
    u16 str_a = calc_team_strength(away);
    u16 total  = str_h + str_a;
    u16 avg_goals = 2 + rng_next() % 2;
    u16 home_prob = (str_h * 100) / total;

    u8 hg = 0, ag = 0;
    for (u16 i = 0; i <= avg_goals; i++) {
        if ((rng_next() % 100) < home_prob) hg++;
        else ag++;
    }
    r.home_goals = hg;
    r.away_goals = ag;
    return r;
}
```

### 6.2 `rng.c` — Gerador Pseudo-Aleatório

LCG 32 bits, compatível em comportamento com o LCG interno do Turbo Pascal:

```c
static u32 rng_state;

void rng_init(u16 seed) {
    rng_state = (u32)seed * 1664525UL + 1013904223UL;
}

u16 rng_next(void) {
    rng_state = rng_state * 1664525UL + 1013904223UL;
    return (u16)(rng_state >> 16);
}
```

### 6.3 `economy.c` — Sistema Econômico

```c
long economy_min_salary(u8 strength) {
    return (long)strength * 100L;
}

u8 economy_salary_request(u8 player_idx, u8 team_idx) {
    Player *p = &g_players[player_idx];
    long min_sal = economy_min_salary(p->strength);
    if (p->salary >= min_sal) return 0;

    u8 forced = (team_player_count(team_idx) <= 14) ||
                (p->pos == POS_GR && team_gk_count(team_idx) <= 1);
    return forced ? 1 : 2;
}

long economy_ticket_revenue(u8 home_team) {
    Team *home = &g_teams[home_team];
    u16 attendance = home->stadium_cap / 2 +
                     (rng_next() % (home->stadium_cap / 4));
    if (attendance > home->stadium_cap) attendance = home->stadium_cap;
    return (long)attendance * home->ticket_price;
}
```

### 6.4 `transfer.c` — Leilão por Salário

```c
u8 transfer_auction(u8 player_idx, u8 current_team) {
    // Estrutura local — não usar malloc
    u8 winner_team = 0xFF;  // 0xFF = sem ofertas
    long best_offer = 0;

    Player *p = &g_players[player_idx];
    long min_sal = economy_min_salary(p->strength);

    for (u8 i = 0; i < 29; i++) {
        if (i == current_team) continue;
        Team *t = &g_teams[i];
        long budget = t->money / 10L;
        if (budget < min_sal) continue;

        long offer = min_sal + (long)(rng_next() % (u16)(min_sal / 2 + 1));
        if (offer > budget) offer = budget;

        if (offer > best_offer) {
            best_offer = offer;
            winner_team = i;
        }
    }

    if (winner_team != 0xFF)
        transfer_player(player_idx, current_team, winner_team, best_offer);

    return winner_team;
}
```

### 6.5 `league.c` — Classificação

Selection sort evita necessidade de `qsort` (não disponível garantidamente em `libmd.a`):

```c
void league_sort_standings(u8 division) {
    u8 div_teams[16];
    u8 count = 0;
    for (u8 i = 0; i < 29; i++) {
        if (g_teams[i].division == division)
            div_teams[count++] = i;
    }
    // Selection sort descendente por pontos (desempate: saldo de gols)
    for (u8 i = 0; i < count - 1; i++) {
        u8 best = i;
        for (u8 j = i + 1; j < count; j++) {
            Team *a = &g_teams[div_teams[j]];
            Team *b = &g_teams[div_teams[best]];
            s16 gd_a = (s16)a->goals_for - (s16)a->goals_against;
            s16 gd_b = (s16)b->goals_for - (s16)b->goals_against;
            if (a->points > b->points ||
                (a->points == b->points && gd_a > gd_b))
                best = j;
        }
        if (best != i) {
            u8 tmp = div_teams[i];
            div_teams[i] = div_teams[best];
            div_teams[best] = tmp;
        }
    }
    // Armazena índices ordenados no buffer de exibição (array estático global)
    // standings_buffer[division] = { div_teams[0..count-1] }
}
```

---

## 7. Input: Mapeamento de Controles

### 7.1 Tabela de Mapeamento Geral

| Função Original (DOS) | Botão Genesis 6-btn | Botão Genesis 3-btn |
|---|---|---|
| Confirmar / Selecionar | `A` | `A` |
| Cancelar / Voltar | `B` | `B` |
| Auto-escalar (Alt+F3) | `C` | `C` (longo) |
| Menu principal / Pausa | `Start` | `Start` |
| Navegar lista ↑↓ | `D-pad ↑↓` | `D-pad ↑↓` |
| Navegar lista ←→ | `D-pad ←→` | `D-pad ←→` |
| Formação ofensiva (F1–F5) | `X` + `D-pad ←` | — |
| Formação defensiva (F6–F10) | `X` + `D-pad →` | — |
| Próxima formação | `Y` | — |
| Gravar (Alt+F1) | `Start` (hold 2s) | `Start` (hold) |

### 7.2 Help Bar Contextual

A linha 26 do WINDOW exibe os controles disponíveis no contexto atual:

```
Em lista de seleção: [A]Confirmar  [B]Voltar  [↑↓]Navegar
Em escalação:        [A]Titular    [B]Banco   [Y]Formação  [C]Auto
Em leilão:           [A]Aceitar    [B]Recusar [C]Contraofertar
```

### 7.3 Implementação de Input com Edge Detection

```c
// engine/input.c
static u16 prev_state;
static u16 curr_state;
static u16 held_frames[16]; // contador de frames para hold detection

void input_update(void) {
    prev_state = curr_state;
    curr_state = JOY_readJoypad(JOY_1);
    // Atualiza contadores de hold
    for (u8 i = 0; i < 16; i++) {
        if (curr_state & (1 << i)) held_frames[i]++;
        else held_frames[i] = 0;
    }
}

u16 input_pressed(u16 btn) {
    return (curr_state & btn) & ~(prev_state & btn);
}

u16 input_held(u16 btn) { return curr_state & btn; }

// Botão segurado por >= 60 frames (1 segundo a 60fps)
u16 input_held_long(u16 btn_bit_pos) {
    return held_frames[btn_bit_pos] >= 60;
}
```

### 7.4 Navegação em Listas

```c
typedef struct {
    u8 selected;
    u8 count;
    u8 page_top;
    u8 page_size;
} ListNav;

void list_nav_update(ListNav *nav) {
    if (input_pressed(BUTTON_DOWN) && nav->selected < nav->count - 1) {
        nav->selected++;
        if (nav->selected >= nav->page_top + nav->page_size)
            nav->page_top++;
    }
    if (input_pressed(BUTTON_UP) && nav->selected > 0) {
        nav->selected--;
        if (nav->selected < nav->page_top)
            nav->page_top--;
    }
}
```

---

## 8. Sistema de SRAM

### 8.1 Layout dos 3 Slots em 8 KB

```
Offset 0x0000–0x0003: Magic "ELF2" (valida presença de dados)
Offset 0x0004–0x0005: Versão do formato de save (u16)
Offset 0x0006–0x07FF: Slot 0 (2042 bytes)
Offset 0x0800–0x0FFF: Slot 1 (2048 bytes)
Offset 0x1000–0x17FF: Slot 2 (2048 bytes)
Offset 0x1800–0x1FFF: Reserva
```

### 8.2 Conteúdo de Cada Slot (≤ 2048 bytes)

```
Bytes 0–1:    CRC16 do slot (valida integridade)
Byte  2:      Número da temporada
Byte  3:      Jornada atual
Byte  4:      Índice da equipe do jogador
Byte  5:      Fase da copa

— Dados das 29 equipes (29 × 14 bytes = 406 bytes) —
Por equipe: division(1) + money(4) + wins(1) + draws(1) + losses(1) +
            goals_for(1) + goals_against(1) + points(2) + stadium_cap(2) = 14 bytes

— Dados dos jogadores (464 × 3 bytes = 1392 bytes) —
Por jogador: strength(1) + salary_lo(1) + salary_hi(1)
(salary comprimido para 2 bytes = 16-bit → limitar a 65535 escudos no save)

— Palmares (últimas 5 temporadas × 4 bytes = 20 bytes) —

Total: 2 + 4 + 406 + 1392 + 20 = 1824 bytes ← cabe folgado em 2048!
```

### 8.3 Funções de I/O de SRAM

```c
// engine/sram_io.c
// SGDK 1.70 não tem SRAM_readBuffer/writeBuffer — implementar como loop de bytes.
static void sram_write_buffer(u32 offset, const u8 *buf, u16 len) {
    SRAM_enable();
    for (u16 i = 0; i < len; i++)
        SRAM_writeByte(offset + i, buf[i]);
    SRAM_disable();
}

static void sram_read_buffer(u32 offset, u8 *buf, u16 len) {
    SRAM_enable();
    for (u16 i = 0; i < len; i++)
        buf[i] = SRAM_readByte(offset + i);
    SRAM_disable();
}

// CRC-16/CCITT para validação de integridade do slot
u16 sram_crc16(const u8 *data, u16 len) {
    u16 crc = 0xFFFF;
    for (u16 i = 0; i < len; i++) {
        crc ^= (u16)data[i] << 8;
        for (u8 b = 0; b < 8; b++)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
    }
    return crc;
}

u8 sram_save(u8 slot) {
    u8 buf[1824];
    u16 pos = 2;  // reserva espaço para CRC
    buf[pos++] = g_season.season;
    buf[pos++] = g_season.round;
    buf[pos++] = g_season.player_team;
    buf[pos++] = g_season.cup_phase;
    // serializa equipes, jogadores, palmares...
    u16 crc = sram_crc16(buf + 2, pos - 2);
    buf[0] = (u8)(crc >> 8);
    buf[1] = (u8)(crc & 0xFF);
    u32 base = 0x0006 + (u32)slot * 0x0800;
    sram_write_buffer(base, buf, pos);
    return 1;
}

u8 sram_load(u8 slot) {
    u8 buf[1824];
    u32 base = 0x0006 + (u32)slot * 0x0800;
    sram_read_buffer(base, buf, sizeof(buf));
    u16 stored_crc = ((u16)buf[0] << 8) | buf[1];
    u16 calc_crc   = sram_crc16(buf + 2, sizeof(buf) - 2);
    if (stored_crc != calc_crc) return 0;
    // deserializa...
    return 1;
}
```

---

## 9. Dados em ROM: rescomp

### 9.1 Pré-processamento: `tools/pack_data.py`

Executado antes do `make`, converte `EQUIPAS.EF2` e `TREINAD.EF2` para binários compactos.
Trata as anomalias de parsing documentadas na análise (Seção 16): nomes sem separador entre
nome e posição, encoding CP850, código de nacionalidade errado (`POK`→`POR`).

```python
#!/usr/bin/env python3
# tools/pack_data.py
import struct, re, unicodedata, sys

CP850_MAP = {
    0x80:'C', 0x82:'e', 0x83:'a', 0x84:'a', 0x85:'a', 0x87:'c',
    0x88:'e', 0x89:'e', 0x8A:'e', 0x8B:'i', 0x8C:'i', 0x8D:'i',
    0x8E:'A', 0x8F:'A', 0x90:'E', 0x91:'o', 0x92:'o', 0x93:'o',
    0x94:'o', 0x95:'o', 0x96:'u', 0x97:'u', 0x98:'u', 0x99:'O',
    0x9A:'U', 0x9B:'c', 0xA0:'a', 0xA1:'i', 0xA2:'o', 0xA3:'u',
    0xA4:'n', 0xE0:'a', 0xE1:'s', 0xE2:'o', 0xE3:'u', 0xE4:'n',
    # ... mapa completo CP850→ASCII para letras com acento
}

def transliterate(s):
    """Converte string CP850 bytes para ASCII, removendo acentos."""
    result = []
    for b in s if isinstance(s, bytes) else s.encode('latin-1', errors='replace'):
        if b < 128:
            result.append(chr(b))
        elif b in CP850_MAP:
            result.append(CP850_MAP[b])
        else:
            result.append('?')
    return ''.join(result)

NAT_MAP = {'POR':0, 'BRA':1, 'ARG':2, 'ESP':3, 'ING':4,
           'FRA':5, 'ITA':6, 'ALE':7, 'SER':8, 'CRO':9,
           'JUG':8, 'POK':0}  # POK → POR (anomalia conhecida)

POS_MAP = {'gr':0, 'df':1, 'md':2, 'av':3}

def parse_player_line(line):
    """
    Parsing robusto: split pelo fim da linha.
    Trata casos de nome sem espaço antes do código de posição.
    """
    line = line.rstrip()
    # Os últimos 6 chars são sempre: "xx XXX" (pos + espaço + nat) ou variantes
    # Estratégia: rsplit com limite para extrair os 2 últimos tokens
    parts = line.rsplit(None, 2)
    if len(parts) < 3:
        return None
    name_raw, pos_raw, nat_raw = parts[0], parts[1].lower(), parts[2].upper()
    name = transliterate(name_raw.strip())[:14]
    pos  = POS_MAP.get(pos_raw, 1)
    nat  = NAT_MAP.get(nat_raw[:3], 0)
    return name, pos, nat

# Formato binário por jogador: 14+1+1 = 16 bytes
PLAYER_FMT = '14sBB'  # name(14), pos(1), nat(1)
# Formato binário por equipe: 14+1+1+1+1+1 = 19 bytes (sem jogadores)
TEAM_FMT   = '14sBBBBB'  # name(14), nat(1), n1(1), n2(1), nplayers(1), player_start(1)
```

### 9.2 Arquivo `res/resources.res`

```
# Fonte de texto
TILESET font_tiles "gfx/font_cp850.png" NONE NONE

# Dados de equipes e jogadores
BIN teams_data   "data/teams.bin"   2 2 0 FAST
BIN coaches_data "data/coaches.bin" 2 2 0 FAST

# Paletas (PNGs indexados 16px × 1px com as 16 cores exatas)
PALETTE pal_main      "gfx/palette_main.png"
PALETTE pal_selection "gfx/palette_selection.png"
```

### 9.3 Uso no Código

```c
// res/resources.h (gerado pelo rescomp automaticamente):
//   extern const TileSet  font_tiles;
//   extern const u8       teams_data[];
//   extern const u8       coaches_data[];
//   extern const Palette  pal_main;
//   extern const Palette  pal_selection;

void data_init(void) {
    #include "resources.h"

    // Carrega paletas no hardware
    PAL_setPalette(PAL0, pal_main.data,      DMA);
    PAL_setPalette(PAL1, pal_selection.data, DMA);
    // PAL2 e PAL3: iguais a PAL0 por padrão, customizadas depois por equipe
    PAL_setPalette(PAL2, pal_main.data,      DMA);
    PAL_setPalette(PAL3, pal_main.data,      DMA);

    // Carrega fonte na VRAM
    VDP_loadTileSet(&font_tiles, FONT_BASE_TILE, DMA);

    // Descompacta dados de equipes e jogadores para RAM
    data_unpack_teams(teams_data);
    data_unpack_coaches(coaches_data);
}
```

**Nota sobre rescomp e nomes de símbolos:** O nome declarado no `.res` (`teams_data`) é
o nome do símbolo exportado em `resources.h`. Se o nome no `.res` não corresponder ao
usado no código, o erro é de linkagem ("undefined reference"), não de compilação — o que
pode confundir. Manter os nomes consistentes entre `.res` e as chamadas em `data.c`.

---

## 10. Caveats SGDK 1.70 Aplicados

### 10.1 `int` = 16 bits — Tabela de Tipos

| Dado | Tipo correto | Tipo errado | Consequência do erro |
|---|---|---|---|
| Salário de jogador | `long` | `int` | Overflow > 32767 escudos |
| Dinheiro da equipe | `long` | `int` | Overflow catastrófico |
| Receita de bilheteria | `long` | `int` | Overflow fácil |
| Pontos na classificação | `u16` | `u8` | OK para até 255, mas u16 é mais seguro |
| Gols por temporada | `u8` | — | Max ~100: OK |
| Índice de jogador global | `u16` | `u8` | 464 jogadores > 255 |
| Seed do RNG | `u32` | — | LCG precisa de 32 bits |

### 10.2 `SYS_doVBlankProcess()` — Somente no Loop Principal

```c
// CORRETO
void season_run(void) {
    while (!season_is_over()) {
        input_update();
        update_game_state();   // lógica: sem vsync aqui
        render_frame();
        SYS_doVBlankProcess(); // UMA VEZ por frame, aqui
    }
}

// ERRADO — nunca fazer:
void league_sort_standings(u8 div) {
    for (...) {
        SYS_doVBlankProcess(); // NUNCA — 16ms de freeze por iteração
    }
}
```

A simulação de 29 partidas em uma rodada acontece em microsegundos no m68k @ 7.67 MHz —
não há necessidade de vsync intermédio.

### 10.3 `JOY_init()` — Primeira Chamada do `main()`

```c
u16 main(u16 hardReset) {
    JOY_init();   // OBRIGATÓRIO ser a primeira chamada
    VDP_init();
    // ...
}
```

Sem `JOY_init()`, `JOY_readJoypad()` retorna 0 eternamente — o jogo parecerá travado na
tela de título.

### 10.4 `VDP_clearPlane(WINDOW, TRUE)` em Toda Troca de Tela

```c
void screen_transition(GameScreen new_screen) {
    VDP_clearPlane(BG_A,   TRUE);  // conteúdo principal
    VDP_clearPlane(WINDOW, TRUE);  // CRÍTICO — WINDOW mantém lixo se não limpar
    current_screen = new_screen;
    render_status_bar();
}
```

### 10.5 Funções Ausentes em `libmd.a`

O projeto implementa em `engine/compat.c` **sem guards `#ifndef SGDK_GCC`** (precisam
compilar E linkar no build SGDK):

```c
// engine/compat.c
s16 game_strncmp(const char *a, const char *b, u16 n) {
    for (u16 i = 0; i < n; i++) {
        if (a[i] != b[i]) return (s16)((u8)a[i] - (u8)b[i]);
        if (a[i] == '\0') return 0;
    }
    return 0;
}

long game_atol(const char *s) {
    long r = 0;
    u8 neg = 0;
    while (*s == ' ') s++;
    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') {
        r = r * 10L + (*s - '0');
        s++;
    }
    return neg ? -r : r;
}

void *game_memmove(void *dst, const void *src, u16 n) {
    u8 *d = (u8*)dst;
    const u8 *s = (const u8*)src;
    if (d < s) { while (n--) *d++ = *s++; }
    else { d += n; s += n; while (n--) *--d = *--s; }
    return dst;
}
```

### 10.6 LTO e Tipos de Retorno

Declarações em headers devem corresponder **exatamente** às definições em `.c`. O LTO
(Link Time Optimization, ativado por padrão no SGDK) detecta discrepâncias entre
compilation units e pode gerar comportamento incorreto silenciosamente.

```c
// CORRETO — declarações e definições com tipos idênticos
u16  match_get_goals(u8 team_idx);       // .h e .c concordam
s16  economy_get_balance(u8 team_idx);   // pode ser negativo
u16  rng_next(void);

// ERRADO — int/bool são armadilhas
int  match_get_goals(int team_idx);      // int = 16 bits no m68k, confunde
bool economy_is_bankrupt(int team_idx);  // bool = u16, mas int ≠ u16 em LTO
```

### 10.7 `VDP_drawText()` vs Fonte Customizada

O projeto **não usa `VDP_drawText()`** — ela só escreve no WINDOW com paleta fixa e não
suporta texto colorido. Toda renderização usa `VDP_setTileMapXY()` com tiles da fonte
customizada. Exceção única: modo debug.

```c
#ifdef DEBUG
    VDP_showFPS(FALSE);  // usa VDP_drawText internamente — OK só para debug
#endif
```

### 10.8 SRAM: Par `enable()`/`disable()` Obrigatório

Todo acesso à SRAM deve usar os wrappers `sram_write_buffer`/`sram_read_buffer` que
garantem o par. **Nunca** chamar `SRAM_writeByte` ou `SRAM_readByte` diretamente no
código do jogo — somente nos wrappers.

---

## 11. Cronograma de Desenvolvimento

### Fase 0 — Infraestrutura (1–2 semanas)
- [ ] Projeto SGDK criado, `makefile.gen` funcionando, ROM vazia compilando.
- [ ] `render.c` com fonte customizada, texto colorido com PAL0/PAL1, `render_box()`.
- [ ] **Validação visual das paletas CGA:** tela de teste mostrando as 16 cores com labels.
- [ ] `input.c` com edge detection e hold detection.
- [ ] `rng.c` implementado.
- [ ] Status bar (WINDOW) renderizando corretamente com cores de ciano.
- [ ] Tela de título estática renderizando com cores corretas.

### Fase 1 — Dados e Estruturas (1 semana)
- [ ] `tools/pack_data.py` gerando `teams.bin` e `coaches.bin` sem erros de parsing.
- [ ] `data.c` carregando 29 equipes e 464 jogadores de ROM para RAM.
- [ ] Print de debug: listar equipes e jogadores no log do emulador.

### Fase 2 — Motor de Jogo (2–3 semanas)
- [ ] `match.c`: simulação de partidas, verificar resultados plausíveis (média 2-3 gols).
- [ ] `league.c`: classificação, pontos, ordenação com desempate por saldo de gols.
- [ ] `cup.c`: sorteio e eliminatórias em 2 mãos.
- [ ] `economy.c`: salários, bilheteria, prêmios.
- [ ] `transfer.c`: venda direta + leilão por salário.
- [ ] `season.c`: coordenação de uma temporada completa sem UI.

### Fase 3 — Telas de UI (2–3 semanas)
- [ ] `title.c`: tela inicial com seleção de equipe, cores corretas (amarelo/azul).
- [ ] `main_menu.c`: menu pré-rodada com todas as opções.
- [ ] `squad.c`: gestão de plantel, seleção de formação, cursor ciano.
- [ ] `results.c`: exibição de resultados com cores verde/vermelho para gols.
- [ ] `standings.c`: classificação com scroll, posição do jogador em destaque.
- [ ] `finances.c`: salários, receitas, prêmios com valores em verde/vermelho.
- [ ] `transfers.c`: venda, leilão, histórico.
- [ ] `palmares.c`: histórico de títulos (últimas 5 temporadas).
- [ ] `stadium.c`: gestão de construção de bancadas.
- [ ] `coaches.c`: gestão de treinadores.

### Fase 4 — Save/Load e Polish (1 semana)
- [ ] `sram_io.c`: save/load completo com CRC16.
- [ ] Tela de save com 3 slots e indicação de data da gravação.
- [ ] Teste de integridade após power-cycle (real ou emulado).
- [ ] Cursor piscando (animação de 2 frames a 30fps).
- [ ] Música de fundo com XGM (opcional — implementar por último).

### Fase 5 — Testes e Ajuste (1 semana)
- [ ] Jogar uma temporada completa sem crashes.
- [ ] Verificar ausência de overflow em valores monetários.
- [ ] Testar com controle de 3 botões.
- [ ] Testar em hardware real (especialmente SRAM com bateria).
- [ ] Ajustar probabilidades de simulação para resultados realistas.
- [ ] Validação final das cores em TV/monitor real (CRT se possível).

---

## 12. Estrutura de Diretórios do Projeto

```
elifoot2_genesis/
├── makefile                          ← invoca makefile.gen do SGDK
├── src/
│   ├── main.c
│   ├── engine/
│   │   ├── render.c  / render.h      ← VDP_setTileMapXY, paletas, boxes
│   │   ├── input.c   / input.h       ← JOY_readJoypad, edge/hold detection
│   │   ├── rng.c     / rng.h         ← LCG 32 bits
│   │   ├── sram_io.c / sram_io.h     ← save/load + CRC16
│   │   └── compat.c  / compat.h      ← memmove, strncmp, atol (sem #ifndef)
│   ├── game/
│   │   ├── types.h                   ← Player, Team, MatchResult, etc.
│   │   ├── data.c    / data.h        ← carregamento de ROM → RAM
│   │   ├── match.c   / match.h       ← simulação
│   │   ├── league.c  / league.h      ← classificação
│   │   ├── cup.c     / cup.h         ← copa eliminatória
│   │   ├── economy.c / economy.h     ← salários, bilheteria
│   │   ├── transfer.c/ transfer.h    ← leilão
│   │   └── season.c  / season.h      ← coordenação da temporada
│   └── screens/
│       ├── title.c        / title.h
│       ├── main_menu.c    / main_menu.h
│       ├── squad.c        / squad.h
│       ├── results.c      / results.h
│       ├── standings.c    / standings.h
│       ├── finances.c     / finances.h
│       ├── transfers.c    / transfers.h
│       ├── palmares.c     / palmares.h
│       ├── stadium.c      / stadium.h
│       ├── coaches.c      / coaches.h
│       └── save_load.c    / save_load.h
├── res/
│   └── resources.res
├── gfx/
│   ├── font_cp850.png         ← 8×768px, 1bpp indexado (96 ASCII + 13 box-drawing)
│   ├── palette_main.png       ← 16×1px, indexado — PAL0 (texto sobre preto)
│   └── palette_selection.png  ← 16×1px, indexado — PAL1 (texto sobre ciano)
├── data/
│   ├── teams.bin              ← gerado por tools/pack_data.py
│   └── coaches.bin            ← gerado por tools/pack_data.py
├── tools/
│   └── pack_data.py           ← converte EQUIPAS.EF2/TREINAD.EF2 → binários
├── original/
│   ├── EQUIPAS.EF2
│   └── TREINAD.EF2
└── out/
    └── rom.bin                ← saída do build (make -f /sgdk/makefile.gen)
```

---

## Apêndice A: Tela de Teste de Paleta (Fase 0)

Implementar como primeira tela renderizada após `render_init()`, para validar que as cores
CGA estão corretas visualmente antes de prosseguir com o desenvolvimento.

```c
void render_palette_test(void) {
    const char *cga_names[] = {
        "0:PRETO    ", "1:AZUL-ESC ", "2:VERDE-ESC", "3:CIANO-ESC",
        "4:VERM-ESC ", "5:MAGEN-ESC", "6:MARROM   ", "7:CINZA-CL ",
        "8:CINZA-ESC", "9:AZUL-BR  ", "A:VERDE-BR ", "B:CIANO-BR ",
        "C:VERM-BR  ", "D:MAGEN-BR ", "E:AMARELO  ", "F:BRANCO   ",
    };
    // PAL0: mostrar cada fg sobre preto
    render_text(BG_A, "=== TESTE DE PALETA CGA->GENESIS ===", 2, 1, PAL0);
    render_text(BG_A, "PAL0 foregrounds sobre preto:", 2, 3, PAL0);
    // Note: como render_text usa a mesma paleta para todos os chars,
    // precisamos de um loop com PAL0 variante que usa fg diferente —
    // ou usar a extensão render_text_fg(plane, str, x, y, pal, fg_pal_entry)
    // Isso confirma se cada entrada da paleta tem a cor CGA correta.
}
```

---

## Apêndice B: Checklist de Fidelidade de Cores

Antes de considerar a Fase 0 concluída, verificar visualmente no emulador (Gens, Fusion,
ou BlastEm) cada item:

- [ ] Texto padrão: **branco sobre preto** — sem amarelamento nem acinzentamento
- [ ] Títulos de tela: **amarelo sobre preto** — amarelo limpo, não laranja
- [ ] Subtítulos/info: **ciano brilhante sobre preto** — azul-esverdeado intenso
- [ ] Valores positivos: **verde brilhante sobre preto** — verde limpo
- [ ] Valores negativos: **vermelho brilhante sobre preto** — vermelho limpo
- [ ] Item selecionado: **preto sobre ciano escuro** — inversão de vídeo nítida
- [ ] Status bar: **fundo ciano escuro** preenchendo as linhas 0–2 e 25–27 do WINDOW
- [ ] Bordas de caixa: **ciano escuro sobre preto** — visíveis mas não dominantes
- [ ] Texto desabilitado: **cinza claro sobre preto** — legível mas subordinado
- [ ] Fundo de tela: **preto sólido** sem pixels espúrios de outras cores

---

*Plano elaborado com base em engenharia reversa de `elifoot.exe` (MS-DOS, Turbo Pascal,
143.648 bytes), análise binária de `EQUIPAS.EF2` (29 equipes, 464 jogadores) e
`TREINAD.EF2` (50 treinadores), documentação SGDK 1.70 (rescomp 3.39b, API VDP/PAL/
SRAM/JOY), e caveats específicos do toolchain m68k-elf-gcc (int=16bits, LTO, libmd.a).*

*v2 — Seção 4 completamente reescrita e expandida com: tabela canônica de cores CGA→Genesis
com formato 0x0BGR correto; distribuição detalhada nas 4 paletas com arrays C completos;
mapeamento de cores por elemento de UI; estratégia para texto sobre fundo colorido; script
Python de validação; tela de teste de paleta (Apêndice A); checklist de fidelidade visual
(Apêndice B). Contagem de equipes corrigida: 28→29, jogadores: 507→464, treinadores: 55→50.*
