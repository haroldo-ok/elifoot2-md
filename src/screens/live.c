/*
 * screens/live.c -- Jornada ao vivo
 *
 * Original: as 4 divisoes (4 jogos cada) no mesmo ecra, ou as 16
 * eliminatorias da taca, com o ultimo marcador e o minuto nas cores do
 * clube, publico a direita, barra de tempo. Ao intervalo cada humano faz
 * substituicoes. Na 2a mao da taca: PROLONGAMENTO e penaltis
 * ("1o PENALTY (CASA) EXECUTADO").
 * C alterna entre marcadores e publico (falta espaco em 40 colunas).
 */

#include <genesis.h>
#include "screens.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"
#include "../game/match.h"
#include "../game/season.h"

static u8 s_show_crowd = 0u;

static u16 game_row(u8 i) {
    if (g_live_cup) return (u16)(3u + i);
    return (u16)(3u + (i / 4u) * 5u + (i % 4u));
}

static void draw_game(u8 i) {
    LiveGame *g = &g_live[i];
    u16 y = game_row(i);
    ui_team(0u, y, 11u, g->home);
    ui_textf(11u, y, C_WHITE, C_BLUE, "%2u ", (u16)g->hg);
    ui_team(14u, y, 11u, g->away);
    ui_textf(25u, y, C_WHITE, C_BLUE, "%2u ", (u16)g->ag);
    if (s_show_crowd) {
        ui_fill(28u, y, 12u, 1u, C_BLUE);
        ui_textf(33u, y, C_YELLOW, C_BLUE, "%6u", (u16)g->spectators);
    } else if (g->last_team != TEAM_NONE && g->last_player != PLAYER_NONE) {
        u8 fg = ui_team_fg(g->last_team), bg = ui_team_bg(g->last_team);
        ui_textw(28u, y, 9u, fg, bg, g_players[g->last_player].name);
        ui_textf(37u, y, fg, bg, "%2u'", (u16)g->last_min);
    } else {
        ui_fill(28u, y, 12u, 1u, C_BLUE);
    }
}

static void draw_frame(void) {
    char lbl[40];
    u8 i, kind = season_slot_kind(g_slot), idx = season_slot_index(g_slot);
    ui_cls(C_BLUE);
    ui_box(0u, 0u, UI_COLS, 3u, C_GREEN, C_BLUE);
    if (kind == SLOT_LEAGUE) ui_sprintf(lbl, sizeof(lbl), "%ua JORNADA", (u16)(idx + 1u));
    else cup_leg_label(lbl, CUP_PHASE_OF(idx), CUP_LEG_OF(idx));
    {
        u16 len = 0u; while (lbl[len]) len++;
        ui_text((u16)((UI_COLS - len) / 2u), 1u, C_LGREEN, C_BLUE, lbl);
    }
    if (!g_live_cup)
        for (i = 1u; i < DIVISIONS; i++) ui_hbar(0u, (u16)(2u + i * 5u), UI_COLS, C_LGRAY, C_BLUE);
    for (i = 0u; i < g_live_n; i++) draw_game(i);
    ui_help("A: acelerar  C: marcadores/publico");
    if (g_autoplay) ui_wait_vblank();
}

static void draw_clock(u8 m, u8 maxm, const char *label) {
    u16 w = (u16)(((u16)(maxm - m) * UI_COLS) / maxm);
    ui_fill(0u, 23u, UI_COLS, 1u, C_BLUE);
    ui_fill(0u, 23u, w, 1u, C_YELLOW);
    ui_fill(0u, 24u, UI_COLS, 1u, C_BLUE);
    ui_textf(0u, 24u, C_LGREEN, C_BLUE, "%3u'", (u16)m);
    if (label) ui_text(6u, 24u, C_YELLOW, C_BLUE, label);
}

/* Corre minutos [from..to]; only_extra = so jogos com prolongamento. */
static void run_minutes(u8 from, u8 to, u8 only_extra, const char *label) {
    u8 m, i, kind = season_slot_kind(g_slot);
    for (m = from; m <= to; m++) {
        u8 d, wait;
        for (i = 0u; i < g_live_n; i++) {
            if (only_extra && !g_live[i].extra) continue;
            if (match_minute(i, m, kind == SLOT_LEAGUE)) draw_game(i);
        }
        draw_clock((u8)(m - from + 1u), (u8)(to - from + 1u), label);
        ui_textf(0u, 24u, C_LGREEN, C_BLUE, "%3u'", (u16)m);
        wait = g_autoplay ? 0u : 5u;
        for (d = 0u; d < wait; d++) {
            ui_wait_vblank();
            input_update();
            if (input_pressed(BTN_ACTION)) {
                s_show_crowd ^= 1u;
                for (i = 0u; i < g_live_n; i++) draw_game(i);
            }
            if (input_held(BTN_CONFIRM)) break;
        }
    }
}

static void halftime(void) {
    u8 m, i;
    for (m = 0u; m < g_mgr_count; m++) {
        u8 t = g_mgr[m].team, used = 0u;
        for (i = 0u; i < g_live_n; i++)
            if (g_live[i].home == t || g_live[i].away == t) {
                screen_subs(i, t, &used);
                break;
            }
    }
}

/* ---- Penaltis ------------------------------------------------------- */
static u8 s_pen_line;
static void pen_cb(u8 game, u8 kick, u8 is_home, u8 scored) {
    u16 y = (u16)(8u + s_pen_line);
    (void)game;
    if (s_pen_line >= 14u) { ui_fill(5u, 8u, 30u, 14u, C_BLACK); s_pen_line = 0u; y = 8u; }
    ui_textf(6u, y, is_home ? C_LCYAN : C_YELLOW, C_BLACK, "%uo PENALTY (%s) %s",
             (u16)(kick + 1u), is_home ? "CASA" : "FORA", scored ? "EXECUTADO" : "FALHADO");
    s_pen_line++;
    ui_delay(35u, 0u);
}

static void penalties(u8 i) {
    LiveGame *g = &g_live[i];
    ui_box(4u, 4u, 32u, 19u, C_WHITE, C_BLACK);
    ui_team(6u, 5u, 13u, g->home);
    ui_team(21u, 5u, 13u, g->away);
    ui_text(14u, 6u, C_WHITE, C_BLACK, "PENALTIS");
    s_pen_line = 0u;
    match_penalties(i, pen_cb);
    ui_textf(14u, 6u, C_WHITE, C_BLACK, "%u - %u    ", (u16)g->ph, (u16)g->pa);
    ui_help("A: continuar");
    ui_wait_key();
}

void screen_live_round(void) {
    u8 i, kind = season_slot_kind(g_slot), idx = season_slot_index(g_slot);
    draw_frame();
    run_minutes(1u, 45u, 0u, NULL);
    draw_clock(45u, 45u, "INTERVALO");
    ui_delay(60u, 1u);
    halftime();
    draw_frame();
    run_minutes(46u, 90u, 0u, NULL);

    if (kind == SLOT_CUP && CUP_LEG_OF(idx) == 1u) {
        u8 any = 0u;
        for (i = 0u; i < g_live_n; i++)
            if (season_cup_tied(i)) { g_live[i].extra = 1u; any = 1u; }
        if (any) {
            draw_clock(90u, 90u, "PROLONGAMENTO");
            ui_delay(90u, 1u);
            run_minutes(91u, 120u, 1u, "PROLONGAMENTO");
            for (i = 0u; i < g_live_n; i++)
                if (g_live[i].extra && season_cup_tied(i)) {
                    penalties(i);
                    draw_frame();
                }
        }
    }
    draw_clock(0u, 1u, "FIM");
    ui_help("A: continuar  C: marcadores/publico");
    for (;;) {
        u16 k = ui_wait_key();
        if (k == BTN_ACTION) {
            s_show_crowd ^= 1u;
            for (i = 0u; i < g_live_n; i++) draw_game(i);
            continue;
        }
        break;
    }
}
