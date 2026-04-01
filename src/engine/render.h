#ifndef RENDER_H
#define RENDER_H

/*
 * render.h — Sistema de renderização textual para Elifoot II Genesis
 *
 * Toda renderização de texto usa VDP_setTileMapXY() com tiles da fonte
 * customizada e o tile attribute word para controle de paleta. Nunca usa
 * VDP_drawText() (escreve apenas no WINDOW com paleta fixa).
 *
 * Tile attribute word do VDP Genesis (16 bits):
 *   Bit 15:      Prioridade (0=baixa, 1=alta)
 *   Bits 14–13:  Índice de paleta (0=PAL0, 1=PAL1, 2=PAL2, 3=PAL3)
 *   Bit 12:      Flip vertical
 *   Bit 11:      Flip horizontal
 *   Bits 10–0:   Índice de tile em VRAM (0–2047)
 *
 * Layout da tela (40 colunas × 28 linhas):
 *   Linha  0:    WINDOW — borda superior da status bar
 *   Linhas 1–2:  WINDOW — status bar (jornada, divisão, dinheiro)
 *   Linhas 3–24: BG_A   — conteúdo principal (menus, listas, resultados)
 *   Linha  25:   WINDOW — separador
 *   Linhas 26–27:WINDOW — help bar (dicas de contexto)
 */

#include <genesis.h>

/* ------------------------------------------------------------------ */
/* Constantes de VRAM                                                  */
/* ------------------------------------------------------------------ */

/* Tiles 1..96: ASCII chars 32 (space) até 127 (DEL/~).               */
#define FONT_BASE_TILE    1

/* Tiles 97..109: box-drawing CP437.                                   */
#define BOX_BASE_TILE     97

/* Número total de tiles de fonte carregados na VRAM.                  */
#define FONT_TILE_COUNT   128

/* ------------------------------------------------------------------ */
/* Tile attribute helpers                                              */
/* ------------------------------------------------------------------ */

/* Converte char ASCII para índice de tile na VRAM.                    */
/* Chars fora do range 32–127 ficam fora do range mas render_text()   */
/* os pula, então não há risco de acesso inválido.                     */
#define CHAR_TO_TILE(c)  (FONT_BASE_TILE + (u16)((u8)(c) - 32u))

/*
 * Monta tile attribute word.
 *   tile:  índice do tile em VRAM (0–2047)
 *   pal:   índice de paleta (PAL0=0 .. PAL3=3)
 *   prio:  prioridade (0=baixa, 1=alta)
 */
#define TILE_ATTR(tile, pal, prio) \
    ( (u16)(((u16)(prio) << 15) | ((u16)(pal) << 13) | ((u16)(tile) & 0x07FFu)) )

/* ------------------------------------------------------------------ */
/* Índices de paleta                                                   */
/* ------------------------------------------------------------------ */

/*
 * PAL0 — Interface geral: texto colorido sobre fundo preto/azul escuro.
 *   [0]  0x0000  preto          — background transparente (padrão)
 *   [1]  0x0600  azul escuro    — background alternativo (menu, título)
 *   [2]  0x0EEE  branco         — texto padrão, mais frequente
 *   [3]  0x02EE  amarelo        — títulos de tela, cabeçalhos
 *   [4]  0x0EE2  ciano brilhante— subtítulos, nomes, informações
 *   [5]  0x02E2  verde brilhante— valores positivos ($, vitórias)
 *   [6]  0x022E  vermelho br.   — valores negativos (dívida, derrotas)
 *   [7]  0x0666  cinza claro    — texto secundário, desabilitado
 *   [8]  0x0606  magenta        — alertas, avisos de copa
 *   [9]  0x0660  ciano escuro   — bordas de caixa, separadores
 *   [10] 0x0222  cinza escuro   — comentários muito discretos
 *   [11] 0x0E2E  magenta br.    — reserva
 *   [12] 0x0026  marrom         — reserva (raro no original)
 *   [13] 0x0060  verde escuro   — reserva
 *   [14] 0x0E00  azul brilhante — reserva
 *   [15] 0x0EEE  branco (dup.)  — tile de cursor animado
 *
 * PAL1 — Seleção / cursor / status bar: texto preto sobre ciano escuro.
 *   [0]  0x0660  ciano escuro   — background dominante (cursor, status bar)
 *   [1]  0x0000  preto          — texto sobre ciano (inversão de vídeo)
 *   [2]  0x0EEE  branco         — texto secundário sobre ciano
 *   [3]  0x02EE  amarelo        — destaque sobre ciano (dinheiro, posição)
 *   [4]  0x02E2  verde brilhante— valores positivos sobre ciano
 *   [5]  0x022E  vermelho br.   — valores negativos sobre ciano
 *   [6]  0x0EE2  ciano brilhante— reserva
 *   [7..15]: cópias das entradas acima para uso futuro
 *
 * PAL2 — Equipe do jogador humano (customizável por equipe).
 *   Inicializada igual a PAL0; pal2[0] e pal2[1] podem ser alteradas
 *   para refletir a cor da camisa da equipe selecionada.
 *
 * PAL3 — Equipe adversária / copa (análoga à PAL2).
 */
#define PAL_MAIN      PAL0   /* interface geral                        */
#define PAL_SELECTED  PAL1   /* item selecionado, status bar           */
#define PAL_PLAYER    PAL2   /* equipe do jogador                      */
#define PAL_OPPONENT  PAL3   /* equipe adversária / copa               */

/*
 * Índices de foreground dentro de cada paleta.
 * Usados apenas como documentação — a cor real é determinada pelo
 * hardware com base na paleta carregada. Passar PAL0 ou PAL1 em
 * render_text() já escolhe o esquema de cores correto.
 */
#define FG_WHITE      2   /* PAL0[2] = branco                          */
#define FG_YELLOW     3   /* PAL0[3] = amarelo                         */
#define FG_CYAN_BR    4   /* PAL0[4] = ciano brilhante                 */
#define FG_GREEN_BR   5   /* PAL0[5] = verde brilhante                 */
#define FG_RED_BR     6   /* PAL0[6] = vermelho brilhante              */
#define FG_GRAY       7   /* PAL0[7] = cinza claro                     */
#define FG_MAGENTA    8   /* PAL0[8] = magenta                         */
#define FG_CYAN_DK    9   /* PAL0[9] = ciano escuro (bordas)           */
#define FG_BLACK_ON_CYAN  1  /* PAL1[1] = preto sobre ciano            */

/* ------------------------------------------------------------------ */
/* Box-drawing: constantes de estilo                                   */
/* ------------------------------------------------------------------ */

#define BOX_SIMPLE  0   /* bordas simples ─│┌┐└┘  (tiles 97–102) */
#define BOX_DOUBLE  1   /* bordas duplas  ═║╔╗╚╝  (tiles 103–108) */

/* Índices dentro de BOX_BASE_TILE para cada caractere:               */
/* +0 ─   +1 │   +2 ┌   +3 ┐   +4 └   +5 ┘                          */
/* +6 ═   +7 ║   +8 ╔   +9 ╗   +10 ╚  +11 ╝   +12 ┼                 */

/* ------------------------------------------------------------------ */
/* Limites de tela                                                     */
/* ------------------------------------------------------------------ */

#define SCREEN_COLS        40
#define SCREEN_ROWS        28
#define CONTENT_ROW_FIRST   3   /* primeira linha do BG_A (conteúdo)  */
#define CONTENT_ROW_LAST   24   /* última linha do BG_A (conteúdo)    */
#define CONTENT_ROWS       22   /* CONTENT_ROW_LAST - CONTENT_ROW_FIRST + 1 */
#define STATUS_ROW          1   /* linha da status bar no WINDOW       */
#define HELP_ROW           26   /* linha da help bar no WINDOW         */

/* ------------------------------------------------------------------ */
/* API pública                                                         */
/* ------------------------------------------------------------------ */

/*
 * render_init() — Deve ser chamada após VDP_init() e antes de qualquer
 * outra função de render. Carrega a fonte na VRAM, configura as paletas
 * e limpa todos os planes.
 */
void render_init(void);

/*
 * render_text() — Escreve string NUL-terminated na posição (x, y) do
 * plane especificado, usando a paleta pal_idx.
 * Chars fora do range ASCII 32–127 são saltados (avançam x).
 * A escrita para ao atingir a coluna 39 (borda direita da tela).
 */
void render_text(VDPPlane plane, const char *str, u16 x, u16 y, u16 pal_idx);

/*
 * render_textf() — Versão formatada de render_text() usando sprintf
 * interno. Buffer interno de 64 chars — não formatar strings longas.
 */
void render_textf(VDPPlane plane, u16 x, u16 y, u16 pal_idx,
                  const char *fmt, ...);

/*
 * render_text_pad() — Escreve string no plano, preenchendo com espaços
 * até atingir width colunas. Útil para sobrescrever texto anterior sem
 * chamar render_clear_rect().
 */
void render_text_pad(VDPPlane plane, const char *str,
                     u16 x, u16 y, u16 width, u16 pal_idx);

/*
 * render_number() — Escreve número inteiro 32 bits com separadores de
 * milhar (ponto). Ex: 1234567 → "1.234.567". width = largura total
 * (preenchida com espaços à esquerda se necessário).
 */
void render_number(VDPPlane plane, long value,
                   u16 x, u16 y, u16 width, u16 pal_idx);

/*
 * render_clear_rect() — Preenche retângulo (w×h tiles a partir de x,y)
 * com tile transparente/preto (tile 0 da PAL0). Equivale a apagar.
 */
void render_clear_rect(VDPPlane plane, u16 x, u16 y, u16 w, u16 h);

/*
 * render_fill_rect() — Preenche retângulo com tile sólido da entrada
 * bg_entry da paleta pal_idx. Útil para blocos de cor sólida (ex: fundo
 * da status bar, blocos de seleção).
 */
void render_fill_rect(VDPPlane plane, u16 x, u16 y, u16 w, u16 h,
                      u16 pal_idx, u16 tile_idx);

/*
 * render_box() — Desenha caixa com bordas usando tiles box-drawing.
 * style: BOX_SIMPLE ou BOX_DOUBLE.
 * O interior da caixa NÃO é limpo — chamar render_clear_rect() antes
 * se necessário.
 */
void render_box(VDPPlane plane, u16 x, u16 y, u16 w, u16 h,
                u16 style, u16 pal_idx);

/*
 * render_hline() / render_vline() — Linhas simples sem cantos.
 * Úteis para separadores dentro do conteúdo.
 */
void render_hline(VDPPlane plane, u16 x, u16 y, u16 len,
                  u16 style, u16 pal_idx);
void render_vline(VDPPlane plane, u16 x, u16 y, u16 len,
                  u16 style, u16 pal_idx);

/*
 * render_set_bg_color() — Define a cor de fundo da tela inteira
 * preenchendo o plane BG_B com tile sólido da cor bg_pal0_entry
 * na paleta PAL0. Chamar ao entrar em uma nova tela.
 *
 * bg_pal0_entry: índice na PAL0 da cor desejada.
 *   0 = preto (padrão, maioria das telas)
 *   1 = azul escuro (menu principal, tela de título)
 */
void render_set_bg_color(u16 bg_pal0_entry);

/*
 * render_status_bar() — Redesenha as linhas fixas do WINDOW (0–2 e 25–27)
 * com o estado atual de g_season. Chamar após cada transição de tela.
 */
void render_status_bar(void);

/*
 * render_help_bar() — Escreve a barra de ajuda contextual nas linhas 26–27
 * do WINDOW. str deve ter no máximo 38 chars por linha.
 * Linha 1 e linha 2 são strings separadas (NULL = linha vazia).
 */
void render_help_bar(const char *line1, const char *line2);

/*
 * render_clear_content() — Limpa a área de conteúdo BG_A (linhas 3–24)
 * e o WINDOW (preserva status bar). Chamar ao entrar em nova tela para
 * garantir que não reste lixo de tela anterior.
 */
void render_clear_content(void);

#endif /* RENDER_H */
