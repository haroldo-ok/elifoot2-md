/*
 * screens/history_screens.c -- PALMARES, ULTIMOS VENCEDORES, TITULOS DAS
 * EQUIPAS, TITULOS DOS TREINADORES NAS ULTIMAS 20 EPOCAS, PREMIOS,
 * PREMIOS DA TEMPORADA
 */

#include <genesis.h>
#include "screens.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"
#include "../game/history.h"
#include "../game/economy.h"

void screen_palmares(u8 t) {
    TeamHonours *h = &g_honours[t];
    u8 fg = ui_team_fg(t), bg = ui_team_bg(t), d;
    u16 y = 6u;
    ui_cls(C_BLUE);
    ui_box(0u, 0u, UI_COLS, 3u, C_BLUE, C_GREEN);
    ui_text(16u, 1u, C_BLUE, C_GREEN, "PALMARES");
    ui_box(0u, 4u, UI_COLS, 21u, C_WHITE, bg);
    ui_textw(2u, 5u, 20u, fg, bg, g_teams[t].name);
    for (d = 0u; d < DIVISIONS; d++) {
        if (!h->seasons[d] && !h->titles[d] && g_teams[t].div != d) continue;
        ui_textf(2u, ++y, fg, bg, "%ua DIVISAO: %u epoca%s", (u16)(d + 1u),
                 (u16)(h->seasons[d] + (g_teams[t].div == d ? 1u : 0u)),
                 (h->seasons[d] + (g_teams[t].div == d ? 1u : 0u)) == 1u ? "" : "s");
        if (h->titles[d] == 1u)
            ui_textf(4u, ++y, fg, bg, "1 campeonato da %ua divisao", (u16)(d + 1u));
        else if (h->titles[d] > 1u)
            ui_textf(4u, ++y, fg, bg, "%u campeonatos da %ua divisao", (u16)h->titles[d], (u16)(d + 1u));
    }
    y++;
    if (h->cup_wins)
        ui_textf(2u, ++y, fg, bg, "%u taca%s", (u16)h->cup_wins, h->cup_wins == 1u ? "" : "s");
    if (h->cup_finals)
        ui_textf(2u, ++y, fg, bg, "%u presenca%s na final da taca", (u16)h->cup_finals,
                 h->cup_finals == 1u ? "" : "s");
    if (h->cup_semis)    ui_textf(2u, ++y, fg, bg, "%u nas meias-finais", (u16)h->cup_semis);
    if (h->cup_quarters) ui_textf(2u, ++y, fg, bg, "%u nos quartos-de-final", (u16)h->cup_quarters);
    if (h->cup_r2)       ui_textf(2u, ++y, fg, bg, "%u na 2a eliminatoria", (u16)h->cup_r2);
    ui_help("A: continuar");
    ui_wait_key();
}

void screen_last_winners(void) {
    u8 i, k;
    ui_cls(C_BLUE);
    ui_title(0u, "ULTIMOS VENCEDORES", C_YELLOW, C_GREEN, C_WHITE);
    if (!g_winners_n) ui_text(5u, 10u, C_WHITE, C_BLUE, "Ainda nao terminou nenhuma epoca.");
    ui_text(0u, 3u, C_LGREEN, C_BLUE, "EP CAMPEONATO (1a DIV)  TACA");
    for (i = 0u, k = g_winners_n; k > 0u && i < 22u; i++) {
        WinnerRec *w = &g_winners[--k];
        u16 y = (u16)(4u + i);
        ui_textf(0u, y, C_WHITE, C_BLUE, "%2u", (u16)w->season);
        ui_team(3u, y, 16u, w->champ[0]);
        ui_team(23u, y, 16u, w->cup);
    }
    ui_help("B: voltar");
    ui_wait_key();
}

void screen_team_titles(void) {
    u8 order[TEAM_COUNT], n = 0u, t, i, j;
    for (t = 0u; t < (u8)TEAM_COUNT; t++)
        if (g_honours[t].titles[0] || g_honours[t].cup_wins) order[n++] = t;
    for (i = 1u; i < n; i++) {
        u8 v = order[i];
        j = i;
        while (j > 0u) {
            TeamHonours *a = &g_honours[order[j - 1u]], *b = &g_honours[v];
            if (b->titles[0] > a->titles[0] ||
                (b->titles[0] == a->titles[0] && b->cup_wins > a->cup_wins)) {
                order[j] = order[j - 1u]; j--;
            } else break;
        }
        order[j] = v;
    }
    ui_cls(C_BLUE);
    ui_title(0u, "TITULOS DAS EQUIPAS", C_YELLOW, C_GREEN, C_WHITE);
    ui_text(0u, 3u, C_LGREEN, C_BLUE, "EQUIPA          CAMPEONATOS   TACAS");
    if (!n) ui_text(10u, 10u, C_WHITE, C_BLUE, "Ainda sem titulos.");
    for (i = 0u; i < n && i < 22u; i++) {
        u16 y = (u16)(4u + i);
        ui_team(0u, y, 15u, order[i]);
        ui_textf(20u, y, C_WHITE, C_BLUE, "%3u", (u16)g_honours[order[i]].titles[0]);
        ui_textf(32u, y, C_WHITE, C_BLUE, "%3u", (u16)g_honours[order[i]].cup_wins);
    }
    ui_help("B: voltar");
    ui_wait_key();
}

static u8 coach_list(u8 cup, u8 *ids) {
    u8 n = 0u, c, i, j;
    for (c = 0u; c < (u8)COACH_COUNT; c++) if (history_coach_titles(c, cup)) ids[n++] = c;
    for (c = 0u; c < MAX_MANAGERS; c++)
        if (history_coach_titles(COACH_HUMAN(c), cup)) ids[n++] = COACH_HUMAN(c);
    for (i = 1u; i < n; i++) {
        u8 v = ids[i];
        j = i;
        while (j > 0u && history_coach_titles(v, cup) > history_coach_titles(ids[j - 1u], cup)) {
            ids[j] = ids[j - 1u]; j--;
        }
        ids[j] = v;
    }
    return n;
}

void screen_coach_titles(void) {
    u8 a[COACH_COUNT + MAX_MANAGERS], b[COACH_COUNT + MAX_MANAGERS];
    u8 na = coach_list(0u, a), nb = coach_list(1u, b), i;
    ui_cls(C_BLUE);
    ui_box(0u, 0u, UI_COLS, 3u, C_WHITE, C_GREEN);
    ui_text(1u, 1u, C_YELLOW, C_GREEN, "TITULOS DOS TREINADORES - 20 EPOCAS");
    ui_text(0u, 3u, C_LGREEN, C_BLUE, "CAMPEONATOS");
    ui_text(20u, 3u, C_LGREEN, C_BLUE, "TACAS");
    if (!na && !nb) ui_text(10u, 10u, C_WHITE, C_BLUE, "Ainda sem titulos.");
    for (i = 0u; i < 22u; i++) {
        u16 y = (u16)(4u + i);
        if (i < na) {
            ui_textw(0u, y, 16u, a[i] >= 200u ? C_YELLOW : C_WHITE, C_BLUE, history_coach_name(a[i]));
            ui_textf(16u, y, C_WHITE, C_BLUE, "%2u", (u16)history_coach_titles(a[i], 0u));
        }
        if (i < nb) {
            ui_textw(20u, y, 16u, b[i] >= 200u ? C_YELLOW : C_WHITE, C_BLUE, history_coach_name(b[i]));
            ui_textf(36u, y, C_WHITE, C_BLUE, "%2u", (u16)history_coach_titles(b[i], 1u));
        }
    }
    ui_help("B: voltar");
    ui_wait_key();
}

/* ---- PREMIOS (Ctrl+F3), valores observados no original -------------- */
void screen_prizes_table(void) {
    static const u8 ord[7] = { PRIZE_FIRST, PRIZE_SECOND, PRIZE_THIRD, PRIZE_CUP,
                               PRIZE_ATTACK, PRIZE_DEFENCE, PRIZE_SCORER };
    static const u8 row[7] = { 6u, 7u, 8u, 10u, 12u, 13u, 14u };
    u8 i;
    ui_cls(C_BLACK);
    ui_box(2u, 2u, 36u, 3u, C_LRED, C_RED);
    ui_text(16u, 3u, C_WHITE, C_RED, "PREMIOS");
    ui_box(2u, 5u, 36u, 11u, C_LRED, C_RED);
    for (i = 0u; i < 7u; i++) {
        ui_text(4u, row[i], C_WHITE, C_RED, economy_prize_label(ord[i]));
        ui_textf(28u, row[i], C_WHITE, C_RED, "%8ld", economy_prize(ord[i]));
    }
    ui_text(3u, 18u, C_DGRAY, C_BLACK, "Pagos em todas as divisoes.");
    ui_help("B: voltar");
    ui_wait_key();
}

/* ---- PREMIOS DA TEMPORADA (fim de epoca) ---------------------------- */
void screen_season_prizes(void) {
    u8 d = 0u, redraw = 1u;
    for (;;) {
        if (redraw) {
            DivAwards *aw = &g_awards[d];
            redraw = 0u;
            ui_cls(C_BLUE);
            ui_box(0u, 0u, UI_COLS, 3u, C_WHITE, C_RED);
            ui_textf(4u, 1u, C_YELLOW, C_RED, "PREMIOS DA TEMPORADA - %ua DIV.", (u16)(d + 1u));
            ui_text(0u, 4u, C_WHITE, C_BLUE, "VENCEDOR DO CAMPEONATO");
            ui_team(2u, 5u, 20u, aw->first);
            ui_text(0u, 7u, C_WHITE, C_BLUE, "2o CLASSIFICADO");
            ui_team(2u, 8u, 20u, aw->second);
            ui_text(0u, 10u, C_WHITE, C_BLUE, "3o CLASSIFICADO");
            ui_team(2u, 11u, 20u, aw->third);
            if (d == 0u) {
                ui_text(0u, 13u, C_WHITE, C_BLUE, "VENCEDOR DA TACA");
                ui_team(2u, 14u, 20u, g_cup_winner);
            }
            ui_textf(0u, 16u, C_WHITE, C_BLUE, "MELHOR ATAQUE (%u GOLOS)", (u16)aw->attack_goals);
            ui_team(2u, 17u, 20u, aw->attack);
            ui_textf(0u, 19u, C_WHITE, C_BLUE, "MELHOR DEFESA (%u GOLOS)", (u16)aw->defence_goals);
            ui_team(2u, 20u, 20u, aw->defence);
            ui_textf(0u, 22u, C_WHITE, C_BLUE, "MELHOR MARCADOR (%u GOLOS)", (u16)aw->scorer_goals);
            if (aw->scorer != PLAYER_NONE) {
                u8 t = g_players[aw->scorer].team;
                ui_textw(2u, 23u, 16u, ui_team_fg(t), ui_team_bg(t), g_players[aw->scorer].name);
                ui_team(19u, 23u, 15u, t);
            }
            ui_help("</>: divisao   A: continuar");
        }
        ui_wait_vblank();
        input_update();
        if (input_pressed(BTN_RIGHT)) { d = (u8)((d + 1u) % DIVISIONS); redraw = 1u; }
        if (input_pressed(BTN_LEFT))  { d = (u8)((d + DIVISIONS - 1u) % DIVISIONS); redraw = 1u; }
        if (input_pressed(BTN_CONFIRM) || input_pressed(BTN_START) || input_pressed(BTN_CANCEL)) return;
    }
}
