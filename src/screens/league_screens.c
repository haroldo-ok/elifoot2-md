/*
 * screens/league_screens.c -- CLASSIFICACAO, Calendario, Resultados,
 * Proximas jornadas, Melhores marcadores
 */

#include <genesis.h>
#include "screens.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"
#include "../game/season.h"

static u8 league_rounds_played(void) {
    /* todas as equipas jogam em todas as jornadas */
    return TEAM_J(&g_teams[g_div_team[0][0]]);
}

static void div_block(u8 d, u16 y) {
    u8 order[TEAMS_PER_DIV], i;
    data_sort_div(d, order);
    ui_textf(0u, y, C_LGREEN, C_BLACK, "%ua DIVISAO", (u16)(d + 1u));
    ui_text(15u, y, C_LGREEN, C_BLACK, " V  E  D    G      P");
    for (i = 0u; i < TEAMS_PER_DIV; i++) {
        u8 t = order[i];
        Team *tm = &g_teams[t];
        u8 fg = ui_team_fg(t), bg = ui_team_bg(t);
        ui_fill(0u, (u16)(y + 1u + i), UI_COLS, 1u, bg);
        ui_textw(0u, (u16)(y + 1u + i), 14u, fg, bg, tm->name);
        ui_textf(15u, (u16)(y + 1u + i), fg, bg, "%2u %2u %2u %3u:%-3u %2u",
                 (u16)TEAM_W(tm), (u16)TEAM_D(tm), (u16)TEAM_L(tm),
                 (u16)TEAM_GF(tm), (u16)TEAM_GA(tm), (u16)TEAM_PTS(tm));
        if (data_is_human(t)) ui_putc_c(39u, (u16)(y + 1u + i), C_YELLOW, bg, '*');
    }
}

void screen_standings_all(void) {
    u8 page = 0u, redraw = 1u;
    for (;;) {
        if (redraw) {
            char t[40];
            redraw = 0u;
            ui_cls(C_BLACK);
            ui_sprintf(t, sizeof(t), "CLASSIFICACAO  %ua JORNADA", (u16)league_rounds_played());
            ui_title(0u, t, C_LGREEN, C_BLACK, C_GREEN);
            div_block((u8)(page * 2u), 3u);
            div_block((u8)(page * 2u + 1u), 13u);
            ui_help(page ? "<: divisoes 1-2   A: continuar" : ">: divisoes 3-4   A: continuar");
        }
        ui_wait_vblank();
        input_update();
        if (input_pressed(BTN_RIGHT) || input_pressed(BTN_LEFT)) { page ^= 1u; redraw = 1u; }
        if (input_pressed(BTN_CONFIRM) || input_pressed(BTN_CANCEL) || input_pressed(BTN_START)) return;
    }
}

void screen_standings_div(u8 d) {
    static const char * const VIEW[3] = { "TOTAL", "CASA", "FORA" };
    u8 view = 0u, redraw = 1u;
    for (;;) {
        if (redraw) {
            u8 order[TEAMS_PER_DIV], i;
            char t[40];
            redraw = 0u;
            data_sort_div(d, order);
            ui_cls(C_BLUE);
            ui_sprintf(t, sizeof(t), "%ua DIVISAO - CLASSIFICACAO - %ua JORN.",
                       (u16)(d + 1u), (u16)league_rounds_played());
            ui_box(0u, 0u, UI_COLS, 3u, C_GREEN, C_GREEN);
            ui_text(1u, 1u, C_BLUE, C_GREEN, t);
            ui_textf(16u, 4u, C_LGREEN, C_BLUE, "%s", VIEW[view]);
            ui_text(15u, 5u, C_LGREEN, C_BLUE, " J  V  E  D    G     P");
            for (i = 0u; i < TEAMS_PER_DIV; i++) {
                u8 tt = order[i];
                Team *tm = &g_teams[tt];
                u8 fg = ui_team_fg(tt), bg = ui_team_bg(tt);
                u8 w, e, l, gf, ga;
                u16 y = (u16)(7u + i * 2u);
                if (view == 1u)      { w = tm->hw; e = tm->hd; l = tm->hl; gf = tm->hgf; ga = tm->hga; }
                else if (view == 2u) { w = tm->aw; e = tm->ad; l = tm->al; gf = tm->agf; ga = tm->aga; }
                else { w = TEAM_W(tm); e = TEAM_D(tm); l = TEAM_L(tm); gf = TEAM_GF(tm); ga = TEAM_GA(tm); }
                ui_fill(0u, y, UI_COLS, 1u, bg);
                ui_textw(1u, y, 14u, fg, bg, tm->name);
                ui_textf(15u, y, fg, bg, "%2u %2u %2u %2u %3u:%-3u", (u16)(w + e + l),
                         (u16)w, (u16)e, (u16)l, (u16)gf, (u16)ga);
                if (view == 0u) ui_textf(37u, y, fg, bg, "%2u", (u16)TEAM_PTS(tm));
            }
            ui_help("</>: TOTAL / CASA / FORA   B: voltar");
        }
        ui_wait_vblank();
        input_update();
        if (input_pressed(BTN_RIGHT)) { view = (u8)((view + 1u) % 3u); redraw = 1u; }
        if (input_pressed(BTN_LEFT))  { view = (u8)((view + 2u) % 3u); redraw = 1u; }
        if (input_pressed(BTN_CANCEL) || input_pressed(BTN_CONFIRM) || input_pressed(BTN_START)) return;
    }
}

/* ---- Calendario (Shift+F1): a equipa e o proximo adversario --------- */
static void calendar_col(u8 t, u16 x) {
    u8 r, d = g_teams[t].div;
    u8 fg = ui_team_fg(t), bg = ui_team_bg(t);
    ui_fill(x, 3u, 20u, 16u, bg);
    ui_textw((u16)(x + 1u), 3u, 18u, fg, bg, g_teams[t].name);
    for (r = 0u; r < LEAGUE_ROUNDS; r++) {
        u8 home, opp = league_opponent(t, r, &home);
        u16 y = (u16)(5u + r);
        if (opp == TEAM_NONE) continue;
        ui_textw(x, y, 13u, fg, bg, g_teams[opp].name);
        ui_putc_c((u16)(x + 13u), y, fg, bg, home ? 'C' : 'F');
        {
            u8 hs = data_div_slot(home ? t : opp), as = data_div_slot(home ? opp : t);
            u8 res = g_res[d][hs][as];
            if (res != 0xFFu)
                ui_textf((u16)(x + 15u), y, fg, bg, "%u:%u", (u16)(res >> 4), (u16)(res & 15u));
        }
    }
}

void screen_calendar(u8 t) {
    u8 home, slot, opp = season_next_match(t, &home, &slot);
    ui_cls(C_BLACK);
    ui_title(0u, "CALENDARIO", C_YELLOW, C_RED, C_WHITE);
    calendar_col(t, 0u);
    if (opp != TEAM_NONE && g_teams[opp].div == g_teams[t].div) calendar_col(opp, 20u);
    ui_help("C = casa  F = fora      B: voltar");
    ui_wait_key();
}

/* ---- Resultados (Shift+F2): tabela cruzada da divisao ---------------- */
void screen_results_grid(u8 d) {
    u8 i, j;
    ui_cls(C_BLUE);
    ui_fill(0u, 2u, UI_COLS, 18u, C_RED);
    ui_textf(0u, 2u, C_WHITE, C_RED, "%ua DIV.", (u16)(d + 1u));
    for (j = 0u; j < TEAMS_PER_DIV; j++) {
        char ab[4];
        u8 t = g_div_team[d][j];
        ab[0] = g_teams[t].name[0]; ab[1] = g_teams[t].name[1];
        ab[2] = g_teams[t].name[2]; ab[3] = '\0';
        ui_text((u16)(9u + j * 4u), 2u, C_WHITE, C_RED, ab);
    }
    for (i = 0u; i < TEAMS_PER_DIV; i++) {
        u16 y = (u16)(4u + i * 2u);
        u8 t = g_div_team[d][i];
        ui_team(0u, y, 8u, t);
        for (j = 0u; j < TEAMS_PER_DIV; j++) {
            u8 r = g_res[d][i][j];
            u16 x = (u16)(9u + j * 4u);
            if (i == j) ui_text(x, y, C_WHITE, C_RED, "---");
            else if (r != 0xFFu) ui_textf(x, y, C_WHITE, C_RED, "%u:%u", (u16)(r >> 4), (u16)(r & 15u));
            else ui_text(x, y, C_LRED, C_RED, " . ");
        }
    }
    ui_text(0u, 21u, C_WHITE, C_BLUE, "Linha = casa, coluna = fora");
    ui_help("B: voltar");
    ui_wait_key();
}

/* ---- Proximas jornadas (Shift+F7) ----------------------------------- */
void screen_next_rounds(u8 t) {
    u8 home, slot, opp = season_next_match(t, &home, &slot);
    u8 played = league_rounds_played(), r, shown = 0u;
    u16 y = 3u;
    ui_cls(C_BLACK);
    ui_text(11u, 1u, C_WHITE, C_BLACK, "PROXIMAS JORNADAS");
    if (opp != TEAM_NONE && g_teams[opp].div != g_teams[t].div) opp = TEAM_NONE;
    for (r = played; r < LEAGUE_ROUNDS && shown < 5u; r++, shown++) {
        u8 h1, o1 = league_opponent(t, r, &h1);
        ui_textf(0u, y, C_WHITE, C_BLACK, "%2ua", (u16)(r + 1u));
        ui_team(5u, y, 14u, h1 ? t : o1);
        ui_team(20u, y, 14u, h1 ? o1 : t);
        y++;
        if (opp != TEAM_NONE && o1 != opp) {
            u8 h2, o2 = league_opponent(opp, r, &h2);
            ui_team(5u, y, 14u, h2 ? opp : o2);
            ui_team(20u, y, 14u, h2 ? o2 : opp);
            y++;
        }
        y++;
    }
    if (shown == 0u) ui_text(4u, 10u, C_WHITE, C_BLACK, "O campeonato terminou.");
    ui_help("B: voltar");
    ui_wait_key();
}

/* ---- Melhores marcadores (Shift+F4) --------------------------------- */
void screen_scorers(void) {
    u16 best[20];
    u8 n = 0u, i, j;
    u16 p;
    for (p = 0u; p < PLAYER_COUNT; p++) {
        u8 g = g_players[p].goals;
        if (!g || g_players[p].team >= (u8)TEAM_COUNT) continue;
        if (n < 20u) best[n++] = p;
        else if (g > g_players[best[19]].goals) best[19] = p;
        else continue;
        for (j = (u8)(n - 1u); j > 0u && g_players[best[j]].goals > g_players[best[j - 1u]].goals; j--) {
            u16 x = best[j]; best[j] = best[j - 1u]; best[j - 1u] = x;
        }
    }
    ui_cls(C_BLACK);
    ui_box(2u, 0u, 36u, 3u, C_YELLOW, C_RED);
    ui_text(11u, 1u, C_YELLOW, C_RED, "MELHORES MARCADORES");
    if (!n) ui_text(8u, 10u, C_WHITE, C_BLACK, "Ainda nao houve golos.");
    for (i = 0u; i < n; i++) {
        u8 t = g_players[best[i]].team;
        u8 fg = ui_team_fg(t), bg = ui_team_bg(t);
        u16 y = (u16)(4u + i);
        ui_fill(2u, y, 36u, 1u, bg);
        ui_textf(2u, y, fg, bg, "%2u %-15s", (u16)(i + 1u), g_players[best[i]].name);
        ui_textw(21u, y, 13u, fg, bg, g_teams[t].name);
        ui_textf(35u, y, fg, bg, "%3u", (u16)g_players[best[i]].goals);
    }
    ui_help("B: voltar");
    ui_wait_key();
}
