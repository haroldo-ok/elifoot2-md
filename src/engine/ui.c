/*
 * engine/ui.c -- Camada de UI para Elifoot II Genesis
 */

#include <genesis.h>
#include "ui.h"
#include "font_data.h"

/* stdarg via GCC builtins (SGDK nao tem stdarg.h) */
#ifndef va_list
typedef __builtin_va_list va_list;
#define va_start(v,l)  __builtin_va_start(v,l)
#define va_end(v)      __builtin_va_end(v)
#endif

/* ------------------------------------------------------------------ */
#define FONT_FIRST_TILE  TILE_USERINDEX
#define FONT_NUM_TILES   109
#define TILE_SPACE       FONT_FIRST_TILE
#define CHAR_TILE(c)     ((u16)(FONT_FIRST_TILE + (u16)((u8)(c) - 32u)))
/* flipH=TRUE compensates for SGDK VDP_loadTileData word-order storage */
#define TILE_ATTR(pal, tile) TILE_ATTR_FULL((pal), 0, FALSE, TRUE, (tile))

/* ------------------------------------------------------------------ */
static const u16 s_pal0[16] = {
    0x0600,  /* [0] azul escuro -- fundo                              */
    0x0EEE,  /* [1] branco     -- texto normal                       */
    0x02EE,  /* [2] amarelo    -- titulos                            */
    0x0EE2,  /* [3] ciano      -- info                               */
    0x022E,  /* [4] vermelho   -- avisos                             */
    0x0666,  /* [5] cinzento   -- secundario                         */
    0x02E2,  /* [6] verde      -- positivos                          */
    0x0606,  /* [7] magenta    -- alertas                            */
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0EEE
};

static const u16 s_pal1[16] = {
    0x0660,  /* [0] ciano escuro -- fundo seleccionado               */
    0x0000,  /* [1] preto        -- texto sobre seleccao             */
    0x02EE,  /* [2] amarelo      -- destaque sobre seleccao          */
    0x0EEE,  /* [3] branco       -- texto sec sobre seleccao         */
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000
};

/* ------------------------------------------------------------------ */
void ui_init(void) {
    VDP_setWindowVPos(FALSE, 0);
    VDP_setWindowHPos(FALSE, 0);
    VDP_setVerticalScroll(BG_A, 0);
    VDP_setHorizontalScroll(BG_A, 0);
    VDP_setVerticalScroll(BG_B, 0);
    VDP_setHorizontalScroll(BG_B, 0);
    PAL_setColors(0,  s_pal0, 16, CPU);
    PAL_setColors(16, s_pal1, 16, CPU);
    VDP_setBackgroundColor(0);
    VDP_loadTileData(font_tile_data, FONT_FIRST_TILE, FONT_NUM_TILES, CPU);
    ui_clear();
}

/* ------------------------------------------------------------------ */
void ui_clear(void) {
    u16 r, c;
    u16 attr = TILE_ATTR(PAL0, TILE_SPACE);
    for (r = 0u; r < UI_ROWS; r++)
        for (c = 0u; c < UI_COLS; c++)
            VDP_setTileMapXY(BG_B, attr, c, r);
}

/* ------------------------------------------------------------------ */
void ui_putc(u16 x, u16 y, u16 pal, char ch) {
    u16 tile;
    if (x >= UI_COLS || y >= UI_ROWS) return;
    if ((u8)ch < 32u || (u8)ch > 127u) ch = ' ';
    tile = CHAR_TILE(ch);
    VDP_setTileMapXY(BG_B, TILE_ATTR(pal, tile), x, y);
}

/* ------------------------------------------------------------------ */
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

/* ------------------------------------------------------------------ */
/* ui_num() -- escreve numero inteiro sem sinal, alinhado a direita   */
static void ui_num(u16 x, u16 y, u16 pal, u16 val, u16 width) {
    char buf[8];
    u8 i = 7u;
    buf[7] = '\0';
    if (val == 0u) { buf[--i] = '0'; }
    else { while (val > 0u) { buf[--i] = (char)('0' + val % 10u); val /= 10u; } }
    /* pad with spaces */
    while ((u8)(7u - i) < (u8)width) { buf[--i] = ' '; }
    ui_puts(x, y, pal, buf + i);
}

/* ------------------------------------------------------------------ */
void ui_printf(u16 x, u16 y, u16 pal, const char *fmt, ...) {
    va_list ap;
    u16 cx = x;
    const char *p = fmt;
    va_start(ap, fmt);
    while (*p && cx < UI_COLS) {
        if (*p != '%') { ui_putc(cx++, y, pal, *p++); continue; }
        p++; /* skip '%' */
        /* flags */
        u8 left = 0u;
        if (*p == '-') { left = 1u; p++; }
        /* width */
        u16 width = 0u;
        while (*p >= '0' && *p <= '9') {
            width = (u16)(width * 10u + (u16)(*p - '0'));
            p++;
        }
        /* long modifier */
        u8 is_long = 0u;
        if (*p == 'l') { is_long = 1u; p++; }

        if (*p == 'u' || *p == 'd') {
            char tmp[12]; u8 i = 11u; tmp[11] = '\0';
            if (is_long) {
                long lv = __builtin_va_arg(ap, long);
                u8 neg = 0u;
                unsigned long uv;
                if (*p == 'd' && lv < 0) { neg = 1u; uv = (unsigned long)(-lv); }
                else { uv = (unsigned long)lv; }
                if (uv == 0u) { tmp[--i] = '0'; }
                else { while (uv) { tmp[--i] = (char)('0' + (u8)(uv % 10u)); uv /= 10u; } }
                if (neg) tmp[--i] = '-';
            } else {
                u16 val = (u16)(unsigned long)__builtin_va_arg(ap, unsigned long);
                if (val == 0u) { tmp[--i] = '0'; }
                else { while (val) { tmp[--i] = (char)('0' + val % 10u); val /= 10u; } }
            }
            { /* pad and print */
                u8 len = (u8)(11u - i);
                if (!left) { while (len < (u8)width && cx < UI_COLS) { ui_putc(cx++, y, pal, ' '); len++; } }
                { const char *q = tmp + i; while (*q && cx < UI_COLS) ui_putc(cx++, y, pal, *q++); }
                if (left) { while (len < (u8)width && cx < UI_COLS) { ui_putc(cx++, y, pal, ' '); len++; } }
            }
            p++;
        } else if (*p == 's') {
            const char *s = __builtin_va_arg(ap, const char *);
            u8 len = 0u;
            const char *q;
            if (!s) s = "";
            if (!left) {
                q = s; while (*q) { len++; q++; }
                while (len < (u8)width && cx < UI_COLS) { ui_putc(cx++, y, pal, ' '); len++; }
                len = 0u;
            }
            q = s;
            while (*q && cx < UI_COLS) { ui_putc(cx++, y, pal, *q++); len++; }
            if (left) { while (len < (u8)width && cx < UI_COLS) { ui_putc(cx++, y, pal, ' '); len++; } }
            p++;
        } else {
            ui_putc(cx++, y, pal, '%');
            if (*p) ui_putc(cx++, y, pal, *p++);
        }
    }
    va_end(ap);
}

/* ------------------------------------------------------------------ */
void ui_fill_row(u16 y, u16 pal) {
    u16 x;
    if (y >= UI_ROWS) return;
    for (x = 0u; x < UI_COLS; x++)
        VDP_setTileMapXY(BG_B, TILE_ATTR(pal, TILE_SPACE), x, y);
}

/* ------------------------------------------------------------------ */
void ui_hline(u16 x, u16 y, u16 len, u16 pal) {
    u16 cx, end = (u16)(x + len);
    if (end > UI_COLS) end = UI_COLS;
    for (cx = x; cx < end; cx++)
        VDP_setTileMapXY(BG_B, TILE_ATTR(pal, CHAR_TILE('-')), cx, y);
}

/* ------------------------------------------------------------------ */
void ui_wait_vblank(void) {
    SYS_doVBlankProcess();
}
