/*
 * engine/ui.c -- Implementacao da camada de UI
 *
 * Principios:
 *   1. Apenas BG_A. Sem WINDOW. Sem BG_B para conteudo.
 *   2. VDP_setTileMapXY directo -- sem DMA para o tilemap.
 *   3. Fonte carregada com VDP_loadTileData (CPU, sincrono).
 *   4. Paletas carregadas com PAL_setColors (sincrono).
 *   5. Zero chamadas a SYS_doVBlankProcess() dentro do render.
 */

#include <genesis.h>
/* stdarg via GCC builtins -- SGDK does not ship stdarg.h */
int vsprintf(char *buf, const char *fmt, __builtin_va_list ap);
#ifndef va_list
typedef __builtin_va_list va_list;
#define va_start(v,l)  __builtin_va_start(v,l)
#define va_end(v)      __builtin_va_end(v)
#define va_arg(v,l)    __builtin_va_arg(v,l)
#endif
#include "ui.h"
#include "font_data.h"

/* ------------------------------------------------------------------
 * Constantes internas
 * ------------------------------------------------------------------ */

/* O primeiro tile de utilizador no SGDK e o tile 16 (TILE_USERINDEX). */
#define FONT_FIRST_TILE  TILE_USERINDEX   /* = 16                     */
#define FONT_NUM_TILES   109              /* 96 ASCII + 13 box-drawing */

/* Tile de espaco (ASCII 32 = indice 0 na fonte = tile 16 em VRAM).   */
#define TILE_SPACE       FONT_FIRST_TILE

/* Converte char ASCII (32-127) para indice de tile em VRAM.          */
#define CHAR_TILE(c)  ((u16)(FONT_FIRST_TILE + (u16)((u8)(c) - 32u)))

/* Atributo de tile: palette, prioridade 0, sem flip, indice.         */
/* flipH=TRUE compensates for SGDK VDP_loadTileData word-order storage */
#define TILE_ATTR(pal, tile)  TILE_ATTR_FULL((pal), 0, FALSE, TRUE, (tile))

/* ------------------------------------------------------------------
 * Dados de paleta
 * Formato Genesis: 0x0BGR (9 bits, 3 por canal)
 * ------------------------------------------------------------------ */

static const u16 s_pal0[16] = {
    0x0600,   /* [0] azul escuro  -- fundo principal                  */
    0x0EEE,   /* [1] branco       -- texto padrao                     */
    0x02EE,   /* [2] amarelo      -- titulos                          */
    0x0EE2,   /* [3] ciano        -- informacoes                      */
    0x022E,   /* [4] vermelho     -- avisos                           */
    0x0666,   /* [5] cinzento     -- texto secundario                 */
    0x02E2,   /* [6] verde        -- valores positivos                */
    0x0606,   /* [7] magenta      -- alertas                          */
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0EEE
};

static const u16 s_pal1[16] = {
    0x0660,   /* [0] ciano escuro -- fundo seleccionado               */
    0x0000,   /* [1] preto        -- texto sobre seleccao             */
    0x02EE,   /* [2] amarelo      -- destaque sobre seleccao          */
    0x0EEE,   /* [3] branco       -- texto secundario sobre seleccao  */
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000
};

/* ------------------------------------------------------------------
 * ui_init()
 * ------------------------------------------------------------------ */

void ui_init(void) {
    KLog("ui_init: START");
    /* 1. VDP_init() e chamado em main() antes de ui_init(). */

    /* 2. Desactivar o plano WINDOW completamente.
     *    Reg 0x11 (HPos) = 0 com bit7=0 -> sem window horizontal.
     *    Reg 0x12 (VPos) = 0 com bit7=0 -> window cobre 0 linhas do topo.
     *    Isto garante que BG_A e visivel em TODAS as 28 linhas. */
    KLog("ui_init: disabling WINDOW");
    VDP_setWindowVPos(FALSE, 0);
    VDP_setWindowHPos(FALSE, 0);

    /* 3. Scroll a zero em ambos os planos. */
    KLog("ui_init: resetting scroll");
    VDP_setVerticalScroll(BG_A, 0);
    VDP_setHorizontalScroll(BG_A, 0);
    VDP_setVerticalScroll(BG_B, 0);
    VDP_setHorizontalScroll(BG_B, 0);

    /* 4. Carregar paletas (sincrono, sem DMA). */
    KLog("ui_init: loading palettes");
    PAL_setColors(0,  s_pal0, 16, CPU);
    PAL_setColors(16, s_pal1, 16, CPU);

    /* 5. Cor de backdrop = PAL0[0] = azul escuro.
     *    VDP_setBackgroundColor toma um indice global (0-63).
     *    PAL0 ocupa indices 0-15, logo PAL0[0] = indice 0. */
    KLog("ui_init: backdrop color set");
    VDP_setBackgroundColor(0);

    /* 6. Carregar fonte (CPU, sincrono -- sem DMA para evitar problemas
     *    de timing antes do primeiro VBlank). */
    KLog("ui_init: loading font CPU");
    VDP_loadTileData(font_tile_data, FONT_FIRST_TILE, FONT_NUM_TILES, CPU);
    KLog("ui_init: font loaded");

    KLog("ui_init: clearing BG_A");
    /* 7. Limpar BG_A inteiro com tile de espaco.
     * Loop manual via VDP_setTileMapXY = CPU, sincrono, sem DMA.
     * Evita a race condition onde o DMA apaga o texto escrito depois. */
    {
        u16 _r, _c;
        u16 _attr = TILE_ATTR(PAL0, TILE_SPACE);
        for (_r = 0u; _r < UI_ROWS; _r++)
            for (_c = 0u; _c < UI_COLS; _c++)
                VDP_setTileMapXY(BG_B, _attr, _c, _r);
    }
    /* DIAGNOSTIC: write visible tile directly after clear.
     * PAL1[0]=teal(0x0660), PAL1[1]=black(0x0000).
     * If (0,0) shows teal+black, VDP writes work.
     * If (0,0) is pure blue, the tilemap writes are invisible. */
    {
        u16 tile_X = (u16)(FONT_FIRST_TILE + (u16)('X' - 32u));
        VDP_setTileMapXY(BG_B,
            TILE_ATTR_FULL(PAL1, 0, FALSE, FALSE, tile_X),
            0u, 0u);
        KLog_U1("ui_init: wrote X tile idx=", (u32)tile_X);
    }
    KLog("ui_init: DONE");
}

/* ------------------------------------------------------------------
 * ui_clear()
 * ------------------------------------------------------------------ */

void ui_clear(void) {
    KLog("ui_clear: START");
    /* Loop CPU -- VDP_setTileMapXY e sincrono, sem DMA queue. */
    u16 _r, _c;
    u16 _attr = TILE_ATTR(PAL0, TILE_SPACE);
    for (_r = 0u; _r < UI_ROWS; _r++)
        for (_c = 0u; _c < UI_COLS; _c++)
            VDP_setTileMapXY(BG_B, _attr, _c, _r);
    KLog("ui_clear: DONE");
}

/* ------------------------------------------------------------------
 * ui_putc()
 * ------------------------------------------------------------------ */

void ui_putc(u16 x, u16 y, u16 pal, char c) {
    u16 tile;
    if (x >= UI_COLS || y >= UI_ROWS) return;
    if ((u8)c < 32u || (u8)c > 127u) c = ' ';
    tile = CHAR_TILE(c);
    VDP_setTileMapXY(BG_B, TILE_ATTR(pal, tile), x, y);
}

/* ------------------------------------------------------------------
 * ui_puts()
 * ------------------------------------------------------------------ */

void ui_puts(u16 x, u16 y, u16 pal, const char *str) {
    u16 cx = x;
    const char *p = str;
    u8 c;
    while (*p != '\0' && cx < UI_COLS) {
        c = (u8)*p++;
        if (c < 32u || c > 127u) c = 32u;
        VDP_setTileMapXY(BG_B, TILE_ATTR(pal, CHAR_TILE((char)c)), cx, y);
        cx++;
    }
}

/* ------------------------------------------------------------------
 * ui_printf()
 * ------------------------------------------------------------------ */

void ui_printf(u16 x, u16 y, u16 pal, const char *fmt, ...) {
    char buf[42];
    va_list ap;
    va_start(ap, fmt);
    (void)vsprintf(buf, fmt, ap);
    va_end(ap);
    buf[41] = '\0';
    ui_puts(x, y, pal, buf);
}

/* ------------------------------------------------------------------
 * ui_fill_row()
 * ------------------------------------------------------------------ */

void ui_fill_row(u16 y, u16 pal) {
    u16 x;
    if (y >= UI_ROWS) return;
    for (x = 0u; x < UI_COLS; x++) {
        VDP_setTileMapXY(BG_B, TILE_ATTR(pal, TILE_SPACE), x, y);
    }
}

/* ------------------------------------------------------------------
 * ui_hline()
 * ------------------------------------------------------------------ */

void ui_hline(u16 x, u16 y, u16 len, u16 pal) {
    /* Caracter horizontal: ASCII 196 no CP850 = '-', mas usamos '-'
     * simples (ASCII 45) para evitar dependencias de tiles extra.    */
    u16 cx;
    u16 end = (u16)(x + len);
    if (end > UI_COLS) end = UI_COLS;
    for (cx = x; cx < end; cx++) {
        VDP_setTileMapXY(BG_B, TILE_ATTR(pal, CHAR_TILE('-')), cx, y);
    }
}

/* ------------------------------------------------------------------
 * ui_wait_vblank()
 * ------------------------------------------------------------------ */

void ui_wait_vblank(void) {
    SYS_doVBlankProcess();
}
