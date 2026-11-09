/*
 * screens/kbd.c -- Entrada de texto e numeros sem teclado, e utilitarios
 */

#include <genesis.h>
#include "screens.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"

/* ---- Teclado no ecra ------------------------------------------------ */
static const char s_keys[4][11] = {
    "ABCDEFGHIJ", "KLMNOPQRST", "UVWXYZ.-' ", "0123456789"
};
#define KB_ROWS 5u     /* 4 de caracteres + linha de comandos */

u8 kbd_text(const char *title, char *buf, u8 maxlen) {
    u8 len = 0u, cx = 0u, cy = 0u, lower = 1u, redraw = 1u;
    while (buf[len] && len < maxlen) len++;
    buf[len] = '\0';
    for (;;) {
        if (redraw) {
            u8 r, c;
            redraw = 0u;
            ui_cls(C_BLUE);
            ui_title(0u, title, C_YELLOW, C_RED, C_WHITE);
            ui_box(4u, 4u, 32u, 3u, C_WHITE, C_BLACK);
            ui_textw(5u, 5u, 30u, C_YELLOW, C_BLACK, buf);
            if (len < maxlen) ui_putc_c((u16)(5u + len), 5u, C_YELLOW, C_BLACK, '_');
            for (r = 0u; r < 4u; r++)
                for (c = 0u; c < 10u; c++) {
                    char ch = s_keys[r][c];
                    u8 sel = (r == cy && c == cx);
                    if (lower && ch >= 'A' && ch <= 'Z') ch = (char)(ch + 32);
                    ui_putc_c((u16)(9u + c * 2u), (u16)(10u + r * 2u),
                              sel ? C_BLACK : C_WHITE, sel ? C_CYAN : C_BLUE,
                              ch == ' ' ? '_' : ch);
                }
            ui_textw(9u, 19u, 6u, cy == 4u && cx == 0u ? C_BLACK : C_WHITE,
                     cy == 4u && cx == 0u ? C_CYAN : C_BLUE, "Aa/AA");
            ui_textw(17u, 19u, 6u, cy == 4u && cx == 1u ? C_BLACK : C_WHITE,
                     cy == 4u && cx == 1u ? C_CYAN : C_BLUE, "Apaga");
            ui_textw(25u, 19u, 4u, cy == 4u && cx == 2u ? C_BLACK : C_WHITE,
                     cy == 4u && cx == 2u ? C_CYAN : C_BLUE, "Fim");
            ui_help("A:escreve B:apaga C:maiusc START:fim");
        }
        ui_wait_vblank();
        input_update();
        if (input_repeat(BTN_RIGHT)) { cx++; if (cx >= (cy == 4u ? 3u : 10u)) cx = 0u; redraw = 1u; }
        if (input_repeat(BTN_LEFT))  { cx = (cx == 0u) ? (u8)((cy == 4u ? 3u : 10u) - 1u) : (u8)(cx - 1u); redraw = 1u; }
        if (input_repeat(BTN_DOWN))  { cy = (u8)((cy + 1u) % KB_ROWS); if (cy == 4u && cx > 2u) cx = 2u; redraw = 1u; }
        if (input_repeat(BTN_UP))    { cy = (cy == 0u) ? (u8)(KB_ROWS - 1u) : (u8)(cy - 1u); if (cy == 4u && cx > 2u) cx = 2u; redraw = 1u; }
        if (input_pressed(BTN_ACTION)) { lower ^= 1u; redraw = 1u; }
        if (input_repeat(BTN_CANCEL) && len) { buf[--len] = '\0'; redraw = 1u; }
        if (input_pressed(BTN_START)) return len;
        if (input_pressed(BTN_CONFIRM)) {
            redraw = 1u;
            if (cy == 4u) {
                if (cx == 0u) lower ^= 1u;
                else if (cx == 1u) { if (len) buf[--len] = '\0'; }
                else return len;
            } else if (len < maxlen) {
                char ch = s_keys[cy][cx];
                if (lower && ch >= 'A' && ch <= 'Z') ch = (char)(ch + 32);
                /* primeira letra de cada palavra em maiuscula */
                if (lower && ch >= 'a' && ch <= 'z' && (len == 0u || buf[len - 1u] == ' '))
                    ch = (char)(ch - 32);
                buf[len++] = ch;
                buf[len] = '\0';
            }
        }
    }
}

/* ---- Numero --------------------------------------------------------- */
long kbd_number(u16 y, const char *prompt, long initial, u8 digits, u8 fg, u8 bg) {
    u8 d[10], i, cur = 0u, plen = 0u, redraw = 1u;
    long v = (initial < 0) ? 0 : initial;
    if (digits > 9u) digits = 9u;
    for (i = 0u; i < digits; i++) { d[digits - 1u - i] = (u8)(v % 10L); v /= 10L; }
    while (prompt[plen]) plen++;
    if (g_autoplay) return -1L;
    /* cursor no primeiro digito nao nulo */
    for (cur = 0u; cur + 1u < digits && d[cur] == 0u; cur++) ;
    for (;;) {
        if (redraw) {
            redraw = 0u;
            ui_fill(0u, y, UI_COLS, 1u, bg);
            ui_text(0u, y, fg, bg, prompt);
            for (i = 0u; i < digits; i++)
                ui_putc_c((u16)(plen + 1u + i), y, i == cur ? C_BLACK : C_YELLOW,
                          i == cur ? C_YELLOW : bg, (char)('0' + d[i]));
            ui_help("</>:digito  CIMA/BAIXO:valor  A:ok  B:nao");
        }
        ui_wait_vblank();
        input_update();
        if (input_repeat(BTN_LEFT)  && cur > 0u)               { cur--; redraw = 1u; }
        if (input_repeat(BTN_RIGHT) && cur + 1u < digits)      { cur++; redraw = 1u; }
        if (input_repeat(BTN_UP))   { d[cur] = (u8)((d[cur] + 1u) % 10u); redraw = 1u; }
        if (input_repeat(BTN_DOWN)) { d[cur] = (u8)((d[cur] + 9u) % 10u); redraw = 1u; }
        if (input_pressed(BTN_CANCEL)) return -1L;
        if (input_pressed(BTN_CONFIRM) || input_pressed(BTN_START)) {
            long r = 0L;
            for (i = 0u; i < digits; i++) r = r * 10L + d[i];
            return r;
        }
    }
}

/* ---- Utilitarios ---------------------------------------------------- */
char pos_letter(u8 pos) {
    static const char L[4] = { 'G', 'D', 'M', 'A' };
    return L[pos & 3u];
}

const char *pos_name(u8 pos) {
    static const char * const N[4] = { "Guarda-redes", "Defesa", "Medio", "Avancado" };
    return N[pos & 3u];
}

void cup_leg_label(char *out, u8 phase, u8 leg) {
    static const char * const P[5] = {
        "1a ELIMINATORIA", "2a ELIMINATORIA", "QUARTOS DE FINAL", "MEIAS FINAIS", "FINAL"
    };
    ui_sprintf(out, 40u, "%s (%ua MAO)", P[phase < 5u ? phase : 4u], (u16)(leg + 1u));
}

void draw_player_row(u16 x, u16 y, u16 p, u8 fg, u8 bg, u8 show_pos) {
    Player *pl = &g_players[p];
    char buf[44];
    ui_sprintf(buf, sizeof(buf), "%c %-15s %2u %5u %2u %c",
               show_pos ? pos_letter(pl->pos) : ' ', pl->name, (u16)pl->str,
               (u16)pl->salary, (u16)pl->goals, (pl->flags & PF_RAISED) ? '*' : ' ');
    ui_textw(x, y, (u16)(UI_COLS - x), fg, bg, buf);
}

u16 pick_player(u8 t, const char *title) {
    u16 sq[MAX_SQUAD + 8];
    u8 n = data_squad(t, sq, (u8)(MAX_SQUAD + 8u)), sel = 0u, top = 0u, redraw = 1u, i;
    u8 fg = ui_team_fg(t), bg = ui_team_bg(t);
    if (!n) return PLAYER_NONE;
    for (;;) {
        if (redraw) {
            redraw = 0u;
            ui_cls(C_BLUE);
            ui_title(0u, title, C_YELLOW, C_RED, C_WHITE);
            ui_fill(0u, 3u, UI_COLS, 23u, bg);
            ui_textw(0u, 3u, UI_COLS, fg, bg, "  Jogador         For Orden. G");
            for (i = 0u; i < 22u && (u8)(top + i) < n; i++) {
                u8 k = (u8)(top + i);
                u8 show = (k == 0u) || g_players[sq[k]].pos != g_players[sq[k - 1u]].pos;
                if (k == sel) draw_player_row(0u, (u16)(4u + i), sq[k], C_BLACK, C_LGRAY, show);
                else          draw_player_row(0u, (u16)(4u + i), sq[k], fg, bg, show);
            }
            ui_help("CIMA/BAIXO: escolher  A: ok  B: voltar");
        }
        ui_wait_vblank();
        input_update();
        if (input_repeat(BTN_DOWN) && sel + 1u < n) { sel++; if (sel >= top + 22u) top++; redraw = 1u; }
        if (input_repeat(BTN_UP) && sel > 0u) { sel--; if (sel < top) top = sel; redraw = 1u; }
        if (input_pressed(BTN_CANCEL)) return PLAYER_NONE;
        if (input_pressed(BTN_CONFIRM)) return sq[sel];
    }
}
