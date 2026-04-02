/*
 * engine/render.c -- COMPATIBILITY SHIM
 *
 * Wraps the new ui.c API for screens not yet migrated.
 * New screens should use ui.h directly.
 */

#include <genesis.h>
/* stdarg via GCC builtins -- SGDK does not ship stdarg.h */
#ifndef va_list
typedef __builtin_va_list va_list;
#define va_start(v,l)  __builtin_va_start(v,l)
#define va_end(v)      __builtin_va_end(v)
#define va_arg(v,l)    __builtin_va_arg(v,l)
#endif
#include "render.h"
#include "ui.h"

void render_init(void) { /* ui_init() called from main.c */ }

void render_clear_content(void) {
    ui_clear();
}

void render_set_bg_color(u16 entry) { (void)entry; }

void render_text(VDPPlane plane, const char *str, u16 x, u16 y, u16 pal_idx) {
    (void)plane;
    ui_puts(x, y, pal_idx ? UI_PAL_SELECT : UI_PAL_NORMAL, str);
}

void render_textf(VDPPlane plane, u16 x, u16 y, u16 pal_idx,
                  const char *fmt, ...) {
    char buf[42];
    va_list ap;
    (void)plane;
    va_start(ap, fmt);
    (void)vsprintf(buf, fmt, ap);
    va_end(ap);
    buf[41] = '\0';
    ui_puts(x, y, pal_idx ? UI_PAL_SELECT : UI_PAL_NORMAL, buf);
}

void render_num_right(VDPPlane plane, long val, u16 x, u16 y,
                      u16 width, u16 pal_idx) {
    (void)plane;
    ui_printf(x, y, pal_idx ? UI_PAL_SELECT : UI_PAL_NORMAL,
              "%*ld", (int)width, val);
}

void render_clear_rect(VDPPlane plane, u16 x, u16 y, u16 w, u16 h) {
    u16 row, col;
    (void)plane;
    for (row = y; row < (u16)(y + h); row++)
        for (col = x; col < (u16)(x + w); col++)
            ui_putc(col, row, UI_PAL_NORMAL, ' ');
}

void render_fill_rect(VDPPlane plane, u16 x, u16 y, u16 w, u16 h,
                      u16 pal_idx, u16 tile_idx) {
    u16 row, col;
    (void)plane; (void)tile_idx;
    for (row = y; row < (u16)(y + h); row++)
        for (col = x; col < (u16)(x + w); col++)
            ui_putc(col, row, pal_idx ? UI_PAL_SELECT : UI_PAL_NORMAL, ' ');
}

void render_hline(VDPPlane plane, u16 x, u16 y, u16 len,
                  u16 style, u16 pal_idx) {
    (void)plane; (void)style;
    ui_hline(x, y, len, pal_idx ? UI_PAL_SELECT : UI_PAL_NORMAL);
}

void render_vline(VDPPlane plane, u16 x, u16 y, u16 len,
                  u16 style, u16 pal_idx) {
    u16 i;
    (void)plane; (void)style;
    for (i = 0u; i < len; i++)
        ui_putc(x, (u16)(y + i), pal_idx ? UI_PAL_SELECT : UI_PAL_NORMAL, '|');
}

void render_box(VDPPlane plane, u16 x, u16 y, u16 w, u16 h,
                u16 style, u16 pal_idx) {
    render_hline(plane, x,           y,           w, style, pal_idx);
    render_hline(plane, x,           (u16)(y+h-1), w, style, pal_idx);
    render_vline(plane, x,           (u16)(y+1), (u16)(h-2), style, pal_idx);
    render_vline(plane, (u16)(x+w-1), (u16)(y+1), (u16)(h-2), style, pal_idx);
}

void render_status_bar(void) { /* no-op -- handled in ui layer */ }

void render_help_bar(const char *line1, const char *line2) {
    ui_hline(0u, 25u, UI_COLS, UI_PAL_NORMAL);
    if (line1) ui_puts(0u, 26u, UI_PAL_NORMAL, line1);
    if (line2) ui_puts(0u, 27u, UI_PAL_NORMAL, line2);
}

void render_text_pad(VDPPlane plane, const char *str,
                     u16 x, u16 y, u16 width, u16 pal_idx) {
    u16 pal = pal_idx ? UI_PAL_SELECT : UI_PAL_NORMAL;
    u16 len = 0u;
    const char *p = str;
    (void)plane;
    while (*p++) len++;
    if (len > width) len = width;
    ui_puts(x, y, pal, str);
    /* pad with spaces */
    while (len < width) { ui_putc((u16)(x + len), y, pal, ' '); len++; }
}

void render_number(VDPPlane plane, long value,
                   u16 x, u16 y, u16 width, u16 pal_idx) {
    (void)plane;
    ui_printf(x, y, pal_idx ? UI_PAL_SELECT : UI_PAL_NORMAL,
              "%*ld", (int)width, value);
}
