/*
 * engine/ui.h -- Camada de UI para Elifoot II Genesis
 *
 * Interface limpa entre logica de jogo e display VDP.
 * Toda a logica de jogo e cega a estes detalhes.
 *
 * DECISOES DE DESIGN:
 *   - Um unico plano BG_A para todo o texto (sem WINDOW, sem BG_B para texto).
 *   - BG_B = fundo solido (cor backdrop, preenchido uma vez).
 *   - Sem scroll. Tilemap 64x32, ecra visivel 40x28.
 *   - Paleta PAL0: [0]=bg (azul escuro), [1]=fg (branco), [2]=amarelo, [3]=ciano.
 *   - Paleta PAL1: [0]=sel-bg (ciano), [1]=sel-fg (preto).
 *   - Fonte CP850 8x8, carregada como array C no FONT_SLOT (tile 16..124).
 */

#ifndef ELF_UI_H
#define ELF_UI_H

#include <genesis.h>

/* ------------------------------------------------------------------
 * Constantes publicas
 * ------------------------------------------------------------------ */

#define UI_COLS      40   /* colunas visiveis do ecra                 */
#define UI_ROWS      28   /* linhas  visiveis do ecra                 */

/* Indices de cor da paleta PAL0 (texto normal)                       */
#define UI_COL_BG     0   /* azul escuro -- fundo / "transparente"    */
#define UI_COL_FG     1   /* branco      -- texto padrao              */
#define UI_COL_TITLE  2   /* amarelo     -- titulos                   */
#define UI_COL_INFO   3   /* ciano       -- valores / info            */
#define UI_COL_WARN   4   /* vermelho    -- avisos / negativos        */
#define UI_COL_DIM    5   /* cinzento    -- texto secundario          */

/* Paleta para linha seleccionada / cursor                            */
#define UI_PAL_NORMAL   PAL0
#define UI_PAL_SELECT   PAL1

/* ------------------------------------------------------------------
 * API publica
 * ------------------------------------------------------------------ */

/*
 * ui_init() -- Inicializa VDP, carrega paletas e fonte.
 * Deve ser chamada uma vez antes de qualquer outra funcao de UI.
 */
void ui_init(void);

/*
 * ui_clear() -- Limpa o ecra inteiro (escreve tiles de espaco em toda BG_A).
 */
void ui_clear(void);

/*
 * ui_puts(x, y, pal, str) -- Escreve string em (x,y) com paleta pal.
 * pal = UI_PAL_NORMAL ou UI_PAL_SELECT.
 * Clips ao limite do ecra. Nao faz wrap.
 */
void ui_puts(u16 x, u16 y, u16 pal, const char *str);

/*
 * ui_putc(x, y, pal, c) -- Escreve um caracter.
 */
void ui_putc(u16 x, u16 y, u16 pal, char c);

/*
 * ui_printf(x, y, pal, fmt, ...) -- puts com formato.
 * Usa buffer interno de 42 chars.
 */
void ui_printf(u16 x, u16 y, u16 pal, const char *fmt, ...);

/*
 * ui_fill_row(y, pal) -- Preenche linha y inteira com espaco na paleta pal.
 * Util para realcar linha seleccionada.
 */
void ui_fill_row(u16 y, u16 pal);

/*
 * ui_hline(x, y, len, pal) -- Linha horizontal com caracter '─'.
 */
void ui_hline(u16 x, u16 y, u16 len, u16 pal);

/*
 * ui_wait_vblank() -- Sincroniza com o VBlank (chama SYS_doVBlankProcess).
 * Deve ser chamada uma vez por frame no loop principal.
 */
void ui_wait_vblank(void);

#endif /* ELF_UI_H */
