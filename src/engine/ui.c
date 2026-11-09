/*
 * engine/ui.c -- Texto 40x28 com cores CGA arbitrarias (tiles dinamicos)
 *
 * A fonte (font_data.c) tem 109 glifos (32..140) com pixeis 0/1, um por
 * nibble. Para cada (caracter, fg, bg) gera-se um tile onde 1 -> fg e
 * 0 -> bg, carregado em VRAM na primeira utilizacao. PAL0 = 16 cores CGA.
 */

#include <genesis.h>
#include "ui.h"
#include "font_data.h"
#include "input.h"
#include "../game/data.h"

#ifndef va_start
typedef __builtin_va_list va_list;
#define va_start(v,l)  __builtin_va_start(v,l)
#define va_end(v)      __builtin_va_end(v)
#endif
#ifndef va_arg
#define va_arg(v,t)    __builtin_va_arg(v,t)
#endif

#define FONT_FIRST_TILE  TILE_USERINDEX
#define FONT_NUM_TILES   109u
#define FONT_LAST_CHAR   140u
#define CH_BLOCK_CODE    141u

#define DYN_BASE   ((u16)(TILE_USERINDEX + 128u))
#define DYN_MAX    700u
#define HASH_SIZE  1024u           /* potencia de 2 */
#define HASH_EMPTY 0xFFFFu

/* flipH compensa a ordem dos nibbles em font_tile_data */
#define ATTR(tile) TILE_ATTR_FULL(PAL0, 0, FALSE, TRUE, (tile))

/* CGA -> Genesis (0x0BGR). 0x55 -> 4, 0xAA -> A, 0xFF -> E */
static const u16 s_cga[16] = {
    0x0000, 0x0A00, 0x00A0, 0x0AA0, 0x000A, 0x0A0A, 0x004A, 0x0AAA,
    0x0444, 0x0E44, 0x04E4, 0x0EE4, 0x044E, 0x0E4E, 0x04EE, 0x0EEE
};

#ifdef ELF_AUTOPLAY
u8 g_autoplay = 1u;
#else
u8 g_autoplay = 0u;
#endif

static u16 s_hkey[HASH_SIZE];
static u16 s_htile[HASH_SIZE];
static u16 s_dyn_count;

static const u32 s_block[8] = {
    0x11111111U, 0x11111111U, 0x11111111U, 0x11111111U,
    0x11111111U, 0x11111111U, 0x11111111U, 0x11111111U
};

static void cache_reset(void) {
    u16 i;
    for (i = 0u; i < HASH_SIZE; i++) s_hkey[i] = HASH_EMPTY;
    s_dyn_count = 0u;
}

static u16 tile_for(u8 c, u8 fg, u8 bg) {
    u16 key, h;
    if (c < 32u || c > CH_BLOCK_CODE) c = 32u;
    /* espaco: so interessa a cor de fundo */
    if (c == 32u) fg = bg;
    key = (u16)(((u16)(c - 32u) << 8) | ((u16)(fg & 15u) << 4) | (u16)(bg & 15u));
    h   = (u16)((key * 40503u) >> 5) & (u16)(HASH_SIZE - 1u);
    for (;;) {
        if (s_hkey[h] == key) return s_htile[h];
        if (s_hkey[h] == HASH_EMPTY) break;
        h = (u16)((h + 1u) & (HASH_SIZE - 1u));
    }
    if (s_dyn_count >= DYN_MAX) {
        /* cache cheia: usa o glifo base (nao deve acontecer num ecra) */
        if (c > FONT_LAST_CHAR) c = 32u;
        return (u16)(FONT_FIRST_TILE + (u16)(c - 32u));
    }
    {
        u32 buf[8];
        const u32 *src = (c == CH_BLOCK_CODE) ? s_block
                       : &font_tile_data[(u16)(c - 32u) * 8u];
        u32 f = (u32)(fg & 15u), b = (u32)(bg & 15u);
        u16 r, tile = (u16)(DYN_BASE + s_dyn_count);
        for (r = 0u; r < 8u; r++) {
            u32 px = src[r];
            buf[r] = px * f + (0x11111111UL - px) * b;
        }
        VDP_loadTileData(buf, tile, 1, CPU);
        s_dyn_count++;
        s_hkey[h]  = key;
        s_htile[h] = tile;
        return tile;
    }
}

/* ------------------------------------------------------------------ */
void ui_init(void) {
    VDP_setScreenWidth320();
    VDP_setScreenHeight224();
    VDP_setWindowVPos(FALSE, 0);
    VDP_setWindowHPos(FALSE, 0);
    VDP_setVerticalScroll(BG_A, 0);
    VDP_setHorizontalScroll(BG_A, 0);
    VDP_setVerticalScroll(BG_B, 0);
    VDP_setHorizontalScroll(BG_B, 0);
    PAL_setColors(0, s_cga, 16, CPU);
    VDP_setBackgroundColor(0);
    VDP_loadTileData(font_tile_data, FONT_FIRST_TILE, FONT_NUM_TILES, CPU);
    VDP_clearPlane(BG_A, TRUE);
    cache_reset();
    ui_cls(C_BLUE);
}

void ui_cls(u8 bg) {
    u16 r, c, a;
    cache_reset();
    a = ATTR(tile_for(32u, bg, bg));
    for (r = 0u; r < UI_ROWS; r++)
        for (c = 0u; c < UI_COLS; c++)
            VDP_setTileMapXY(BG_B, a, c, r);
}

void ui_putc_c(u16 x, u16 y, u8 fg, u8 bg, char ch) {
    if (x >= UI_COLS || y >= UI_ROWS) return;
    VDP_setTileMapXY(BG_B, ATTR(tile_for((u8)ch, fg, bg)), x, y);
}

void ui_text(u16 x, u16 y, u8 fg, u8 bg, const char *s) {
    while (*s && x < UI_COLS) ui_putc_c(x++, y, fg, bg, *s++);
}

void ui_textw(u16 x, u16 y, u16 w, u8 fg, u8 bg, const char *s) {
    u16 i;
    for (i = 0u; i < w; i++) {
        char ch = ' ';
        if (*s) ch = *s++;
        ui_putc_c((u16)(x + i), y, fg, bg, ch);
    }
}

/* ------------------------------------------------------------------ */
/* Formatacao: %s %c %u %d %lu %ld, largura, '-' (esquerda), '0'.     */
static u16 vformat(char *out, u16 cap, const char *p, va_list ap) {
    u16 n = 0u;
    if (cap == 0u) return 0u;
    cap--;
    while (*p && n < cap) {
        u8 left = 0u, zero = 0u, is_long = 0u;
        u16 width = 0u;
        char tmp[14];
        const char *s;
        u8 len, i;
        if (*p != '%') { out[n++] = *p++; continue; }
        p++;
        if (*p == '-') { left = 1u; p++; }
        if (*p == '0') { zero = 1u; p++; }
        while (*p >= '0' && *p <= '9') { width = (u16)(width * 10u + (u16)(*p - '0')); p++; }
        if (*p == 'l') { is_long = 1u; p++; }
        if (*p == 'u' || *p == 'd') {
            unsigned long uv; u8 neg = 0u;
            i = 13u; tmp[13] = '\0';
            if (*p == 'd') {
                long v = is_long ? va_arg(ap, long) : (long)va_arg(ap, int);
                if (v < 0) { neg = 1u; uv = (unsigned long)(-v); } else uv = (unsigned long)v;
            } else {
                uv = is_long ? va_arg(ap, unsigned long) : (unsigned long)va_arg(ap, unsigned int);
            }
            if (uv == 0u) tmp[--i] = '0';
            while (uv) { tmp[--i] = (char)('0' + (u8)(uv % 10u)); uv /= 10u; }
            if (neg) tmp[--i] = '-';
            s = &tmp[i];
            p++;
        } else if (*p == 's') {
            s = va_arg(ap, const char *);
            if (!s) s = "";
            p++;
        } else if (*p == 'c') {
            tmp[0] = (char)va_arg(ap, int); tmp[1] = '\0';
            s = tmp; p++;
        } else {
            tmp[0] = '%'; tmp[1] = *p; tmp[2] = '\0';
            if (*p) p++;
            s = tmp;
        }
        len = 0u; while (s[len]) len++;
        if (!left) while (len < width && n < cap) { out[n++] = zero ? '0' : ' '; width--; }
        while (*s && n < cap) out[n++] = *s++;
        if (left) while (len < width && n < cap) { out[n++] = ' '; width--; }
    }
    out[n] = '\0';
    return n;
}

void ui_sprintf(char *out, u16 cap, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vformat(out, cap, fmt, ap);
    va_end(ap);
}

void ui_textf(u16 x, u16 y, u8 fg, u8 bg, const char *fmt, ...) {
    char buf[64];
    va_list ap;
    va_start(ap, fmt);
    vformat(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    ui_text(x, y, fg, bg, buf);
}

/* ------------------------------------------------------------------ */
void ui_fill(u16 x, u16 y, u16 w, u16 h, u8 bg) {
    u16 r, c, a = ATTR(tile_for(32u, bg, bg));
    for (r = y; r < (u16)(y + h) && r < UI_ROWS; r++)
        for (c = x; c < (u16)(x + w) && c < UI_COLS; c++)
            VDP_setTileMapXY(BG_B, a, c, r);
}

void ui_box(u16 x, u16 y, u16 w, u16 h, u8 fg, u8 bg) {
    u16 i;
    if (w < 2u || h < 2u) return;
    ui_fill(x, y, w, h, bg);
    ui_putc_c(x, y, fg, bg, CH_TL);
    ui_putc_c((u16)(x + w - 1u), y, fg, bg, CH_TR);
    ui_putc_c(x, (u16)(y + h - 1u), fg, bg, CH_BL);
    ui_putc_c((u16)(x + w - 1u), (u16)(y + h - 1u), fg, bg, CH_BR);
    for (i = 1u; i < (u16)(w - 1u); i++) {
        ui_putc_c((u16)(x + i), y, fg, bg, CH_H);
        ui_putc_c((u16)(x + i), (u16)(y + h - 1u), fg, bg, CH_H);
    }
    for (i = 1u; i < (u16)(h - 1u); i++) {
        ui_putc_c(x, (u16)(y + i), fg, bg, CH_V);
        ui_putc_c((u16)(x + w - 1u), (u16)(y + i), fg, bg, CH_V);
    }
}

void ui_title(u16 y, const char *s, u8 fg, u8 bg, u8 frame) {
    u16 len = 0u;
    while (s[len]) len++;
    ui_box(0u, y, UI_COLS, 3u, frame, bg);
    ui_text((u16)((UI_COLS - len) / 2u), (u16)(y + 1u), fg, bg, s);
}

void ui_hbar(u16 x, u16 y, u16 len, u8 fg, u8 bg) {
    u16 i;
    for (i = 0u; i < len; i++) ui_putc_c((u16)(x + i), y, fg, bg, CH_H);
}

/* ------------------------------------------------------------------ */
u8 ui_team_fg(u8 t) {
    u8 fg, bg;
    if (t >= (u8)TEAM_COUNT) return C_WHITE;
    fg = g_teams[t].fg & 15u;
    bg = g_teams[t].bg & 7u;
    if (fg == bg) fg = (u8)(fg ^ 8u);   /* garante leitura */
    return fg;
}

u8 ui_team_bg(u8 t) {
    if (t >= (u8)TEAM_COUNT) return C_BLACK;
    return (u8)(g_teams[t].bg & 7u);   /* fundo CGA so tem 8 cores */
}

void ui_team(u16 x, u16 y, u16 w, u8 t) {
    if (t >= (u8)TEAM_COUNT) { ui_textw(x, y, w, C_WHITE, C_BLACK, "---"); return; }
    ui_textw(x, y, w, ui_team_fg(t), ui_team_bg(t), g_teams[t].name);
}

void ui_help(const char *s) {
    ui_textw(0u, (u16)(UI_ROWS - 1u), UI_COLS, C_BLACK, C_LGRAY, s);
}

/* ------------------------------------------------------------------ */
void ui_wait_vblank(void) {
    SYS_doVBlankProcess();
}

u8 ui_delay(u16 frames, u8 skip) {
    if (g_autoplay && frames > 1u) frames = 1u;
    while (frames--) {
        ui_wait_vblank();
        input_update();
        if (skip && (input_pressed(BTN_CONFIRM) || input_pressed(BTN_CANCEL)
                     || input_pressed(BTN_ACTION) || input_pressed(BTN_START)))
            return 1u;
    }
    return 0u;
}

u16 ui_wait_key(void) {
    if (g_autoplay) { ui_wait_vblank(); return BTN_CONFIRM; }
    for (;;) {
        ui_wait_vblank();
        input_update();
        if (input_pressed(BTN_CONFIRM)) return BTN_CONFIRM;
        if (input_pressed(BTN_CANCEL))  return BTN_CANCEL;
        if (input_pressed(BTN_ACTION))  return BTN_ACTION;
        if (input_pressed(BTN_START))   return BTN_START;
    }
}

u8 ui_ask_yn(u16 y, u8 fg, u8 bg, const char *q) {
    ui_fill(0u, y, UI_COLS, 1u, bg);
    ui_text(0u, y, fg, bg, q);
    ui_help("A: Sim   B: Nao");
    if (g_autoplay) { ui_wait_vblank(); return 1u; }
    for (;;) {
        ui_wait_vblank();
        input_update();
        if (input_pressed(BTN_CONFIRM)) return 1u;
        if (input_pressed(BTN_CANCEL))  return 0u;
    }
}

/* ---- Compatibilidade ---------------------------------------------- */
static u8 pfg(u16 pal) { return pal ? C_BLACK : C_WHITE; }
static u8 pbg(u16 pal) { return pal ? C_CYAN  : C_BLUE;  }

void ui_clear(void) { ui_cls(C_BLUE); }
void ui_puts(u16 x, u16 y, u16 pal, const char *s) { ui_text(x, y, pfg(pal), pbg(pal), s); }
void ui_putc(u16 x, u16 y, u16 pal, char c) { ui_putc_c(x, y, pfg(pal), pbg(pal), c); }
void ui_fill_row(u16 y, u16 pal) { ui_fill(0u, y, UI_COLS, 1u, pbg(pal)); }
void ui_hline(u16 x, u16 y, u16 len, u16 pal) { ui_hbar(x, y, len, pfg(pal), pbg(pal)); }

void ui_printf(u16 x, u16 y, u16 pal, const char *fmt, ...) {
    char buf[64];
    va_list ap;
    va_start(ap, fmt);
    vformat(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    ui_text(x, y, pfg(pal), pbg(pal), buf);
}
