/*
 * screens/tactic.c -- TACTICAS F1..F10, ficha do jogo e substituicoes
 *
 * Original: F1..F10 escala os melhores para a formacao e mostra
 * "1 Avestruz" .. "11 Baiano" em escada e SUPLENTES 12..16; Enter joga.
 * Ao intervalo: "JOGADORES EM CAMPO / JOGADORES NO BANCO",
 * "F1 Substituir", "Esc Fim".
 */

#include <genesis.h>
#include "screens.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"
#include "../game/tactics.h"
#include "../game/match.h"

static u8 pick_formation(u8 t) {
    u8 sel = g_teams[t].formation, redraw = 1u, i;
    if (sel >= FORM_COUNT) sel = 2u;
    for (;;) {
        if (redraw) {
            redraw = 0u;
            ui_cls(C_BLACK);
            ui_box(10u, 3u, 20u, 15u, C_WHITE, C_GREEN);
            ui_text(12u, 4u, C_BLUE, C_GREEN, "TACTICAS");
            for (i = 0u; i < FORM_COUNT; i++) {
                u8 s = (i == sel);
                ui_textf(12u, (u16)(6u + i), s ? C_BLACK : C_BLUE, s ? C_LGRAY : C_GREEN,
                         "F%-2u %s ", (u16)(i + 1u), tactics_name(i));
            }
            ui_help("CIMA/BAIXO: tactica  A: escalar  B: voltar");
        }
        ui_wait_vblank();
        input_update();
        if (input_repeat(BTN_DOWN) && sel + 1u < FORM_COUNT) { sel++; redraw = 1u; }
        if (input_repeat(BTN_UP) && sel > 0u) { sel--; redraw = 1u; }
        if (input_pressed(BTN_CANCEL)) return 0xFFu;
        if (input_pressed(BTN_CONFIRM)) return sel;
    }
}

/* Ficha: titulares em escada + suplentes, com trocas manuais. */
static u8 team_sheet(u8 t) {
    u8 cur = 0u, mark = 0xFFu, redraw = 1u;
    u8 fg = ui_team_fg(t), bg = ui_team_bg(t);
    for (;;) {
        u16 st[16], bn[MAX_SQUAD + 8];
        u8 ns = tactics_starters(t, st), nb = tactics_bench(t, bn), i, total;
        total = (u8)(ns + nb);
        if (cur >= total) cur = (u8)(total - 1u);
        if (redraw) {
            redraw = 0u;
            ui_cls(C_BLACK);
            ui_fill(0u, 0u, UI_COLS, 14u, bg);
            ui_textf(1u, 0u, fg, bg, "%s   %s", g_teams[t].name, tactics_name(g_teams[t].formation));
            for (i = 0u; i < ns; i++) {
                u8 s = (i == cur), mk = (i == mark);
                ui_textf((u16)(1u + i), (u16)(2u + i), s ? C_BLACK : (mk ? C_YELLOW : fg),
                         s ? C_LGRAY : bg, "%2u %c %s %u", (u16)(i + 1u),
                         pos_letter(g_players[st[i]].pos), g_players[st[i]].name,
                         (u16)g_players[st[i]].str);
            }
            ui_fill(0u, 14u, UI_COLS, 12u, bg);
            ui_text(1u, 14u, fg, bg, "SUPLENTES");
            for (i = 0u; i < nb && i < 11u; i++) {
                u8 k = (u8)(ns + i), s = (k == cur);
                {
                    u16 x = (u16)(1u + (i % 2u) * 20u), y = (u16)(15u + i / 2u);
                    u8 f2 = s ? C_BLACK : fg, b2 = s ? C_LGRAY : bg;
                    ui_textf(x, y, f2, b2, "%2u %c ", (u16)(k + 1u), pos_letter(g_players[bn[i]].pos));
                    ui_textw((u16)(x + 5u), y, 11u, f2, b2, g_players[bn[i]].name);
                    ui_textf((u16)(x + 16u), y, f2, b2, "%2u", (u16)g_players[bn[i]].str);
                }
            }
            ui_help(mark == 0xFFu ? "A:trocar jogador START:JOGAR B:voltar"
                                  : "Escolha quem entra: A  B:cancelar");
        }
        ui_wait_vblank();
        input_update();
        if (input_repeat(BTN_DOWN) && cur + 1u < total) { cur++; redraw = 1u; }
        if (input_repeat(BTN_UP) && cur > 0u) { cur--; redraw = 1u; }
        if (input_repeat(BTN_RIGHT) && cur >= ns && cur + 1u < total) { cur++; redraw = 1u; }
        if (input_repeat(BTN_LEFT) && cur > ns) { cur--; redraw = 1u; }
        if (input_pressed(BTN_START)) return 1u;
        if (input_pressed(BTN_CANCEL)) {
            if (mark != 0xFFu) { mark = 0xFFu; redraw = 1u; }
            else return 0u;
        }
        if (input_pressed(BTN_CONFIRM)) {
            if (mark == 0xFFu) { mark = cur; }
            else if (mark != cur) {
                u16 a = (mark < ns) ? st[mark] : bn[mark - ns];
                u16 b = (cur < ns) ? st[cur] : bn[cur - ns];
                u8 fa = g_players[a].flags & PF_FIELD, fb = g_players[b].flags & PF_FIELD;
                g_players[a].flags = (u8)((g_players[a].flags & (u8)~PF_FIELD) | fb);
                g_players[b].flags = (u8)((g_players[b].flags & (u8)~PF_FIELD) | fa);
                mark = 0xFFu;
            } else mark = 0xFFu;
            redraw = 1u;
        }
    }
}

u8 screen_tactic(u8 t) {
    for (;;) {
        u8 f = pick_formation(t);
        if (f == 0xFFu) return 0u;
        tactics_autoselect(t, f);
        if (team_sheet(t)) return 1u;
    }
}

/* ---- Intervalo: substituicoes -------------------------------------- */
void screen_subs(u8 game, u8 t, u8 *used) {
    u8 cur = 0u, panel = 0u, out_sel = 0xFFu, redraw = 1u;
    LiveGame *g = &g_live[game];
    u8 fg = ui_team_fg(t), bg = ui_team_bg(t);
    if (g_autoplay) return;
    for (;;) {
        u16 st[16], bn[MAX_SQUAD + 8];
        u8 ns = tactics_starters(t, st), nb = tactics_bench(t, bn), i;
        u8 max = panel ? nb : ns;
        if (max && cur >= max) cur = (u8)(max - 1u);
        if (redraw) {
            redraw = 0u;
            ui_cls(C_BLUE);
            ui_fill(0u, 0u, UI_COLS, 2u, C_RED);
            ui_team(1u, 0u, 15u, g->home);
            ui_textf(17u, 0u, C_WHITE, C_RED, "%u", (u16)g->hg);
            ui_team(20u, 0u, 15u, g->away);
            ui_textf(36u, 0u, C_WHITE, C_RED, "%u", (u16)g->ag);
            ui_textf(1u, 1u, C_YELLOW, C_RED, "INTERVALO  Substituicoes: %u/%u",
                     (u16)*used, (u16)MAX_SUBS);
            ui_box(0u, 3u, 20u, 15u, C_WHITE, bg);
            ui_text(1u, 3u, fg, bg, "EM CAMPO");
            for (i = 0u; i < ns; i++) {
                u8 s = (panel == 0u && i == cur) || (i == out_sel);
                ui_textf(1u, (u16)(4u + i), s ? C_BLACK : fg, s ? C_LGRAY : bg, "%c %-13s%2u",
                         pos_letter(g_players[st[i]].pos), g_players[st[i]].name,
                         (u16)g_players[st[i]].str);
            }
            ui_box(20u, 3u, 20u, 15u, C_WHITE, bg);
            ui_text(21u, 3u, fg, bg, "NO BANCO");
            for (i = 0u; i < nb && i < 13u; i++) {
                u8 s = (panel == 1u && i == cur);
                u8 dim = (g_players[bn[i]].flags & PF_SUBBED) != 0u;
                ui_textf(21u, (u16)(4u + i), s ? C_BLACK : (dim ? C_DGRAY : fg),
                         s ? C_LGRAY : bg, "%c %-13s%2u",
                         pos_letter(g_players[bn[i]].pos), g_players[bn[i]].name,
                         (u16)g_players[bn[i]].str);
            }
            ui_box(0u, 19u, 40u, 4u, C_WHITE, C_RED);
            ui_text(2u, 20u, C_WHITE, C_RED, "A  Substituir");
            ui_text(2u, 21u, C_WHITE, C_RED, "B  Fim");
            ui_help(out_sel == 0xFFu ? "A: quem sai   B: fim" : "A: quem entra  B: cancelar");
        }
        ui_wait_vblank();
        input_update();
        if (input_repeat(BTN_DOWN) && cur + 1u < max) { cur++; redraw = 1u; }
        if (input_repeat(BTN_UP) && cur > 0u) { cur--; redraw = 1u; }
        if (input_pressed(BTN_CANCEL)) {
            if (out_sel != 0xFFu) { out_sel = 0xFFu; panel = 0u; cur = 0u; redraw = 1u; }
            else { match_recompute(game); return; }
        }
        if (input_pressed(BTN_CONFIRM)) {
            if (panel == 0u) {
                if (*used < MAX_SUBS && nb) { out_sel = cur; panel = 1u; cur = 0u; }
            } else if (nb && !(g_players[bn[cur]].flags & PF_SUBBED)) {
                u16 o = st[out_sel], in = bn[cur];
                g_players[o].flags = (u8)((g_players[o].flags & (u8)~PF_FIELD) | PF_SUBBED);
                g_players[in].flags |= PF_FIELD;
                (*used)++;
                out_sel = 0xFFu; panel = 0u; cur = 0u;
            }
            redraw = 1u;
        }
    }
}
