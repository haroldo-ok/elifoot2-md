/*
 * screens/play_round.c -- Simula e exibe uma jornada
 */

#include <genesis.h>
#include "play_round.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"
#include "../game/league.h"
#include "../game/economy.h"
#include "../game/types.h"

static void wait_confirm(void) {
    ui_puts(0u, 27u, UI_PAL_NORMAL, "A/B: continuar");
    for (;;) {
        ui_wait_vblank();
        input_update();
        if (input_pressed(BTN_CONFIRM) || input_pressed(BTN_CANCEL)
                || input_pressed(BTN_START)) return;
    }
}

static void show_round_results(const LeagueRound *rnd, u8 div) {
    u8 i;
    ui_clear();
    ui_printf(0u, 0u, UI_PAL_NORMAL, "Jornada %u -- Div%u   Resultados:",
              (u16)g_round, (u16)(div + 1u));
    ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);

    for (i = 0u; i < rnd->count && i < 8u; i++) {
        u8  h = rnd->home[i];
        u8  a = rnd->away[i];
        u16 row = (u16)(3u + i * 2u);
        /* Find result in g_results */
        u8  r;
        u8  hg = 0u, ag = 0u;
        u16 pal = UI_PAL_NORMAL;

        for (r = 0u; r < g_results_count; r++) {
            MatchResult *mr = &g_results[r];
            if (mr->round == g_round && mr->home_team == h && mr->away_team == a) {
                hg = mr->home_goals;
                ag = mr->away_goals;
                break;
            }
        }

        if (h == g_player_team_idx || a == g_player_team_idx) {
            pal = UI_PAL_SELECT;
            ui_fill_row(row, UI_PAL_SELECT);
        }

        ui_printf(0u, row, pal, "%-13s %u - %-u  %s",
                  g_teams[h].name, (u16)hg, (u16)ag, g_teams[a].name);
    }

    ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
    wait_confirm();
}

static void show_standings_brief(u8 div) {
    u8  order[29];
    u8  count, i;
    u8  top = (count > 10u) ? 10u : count;

    data_sort_standings(div, order, &count);
    top = (count > 10u) ? 10u : count;

    ui_clear();
    ui_printf(0u, 0u, UI_PAL_NORMAL, "Classificacao -- Div%u  Jornada %u",
              (u16)(div + 1u), (u16)g_round);
    ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);
    ui_puts( 0u, 2u, UI_PAL_NORMAL, "#  Equipa             J  V  E  D  Pts");
    ui_hline(0u, 3u, UI_COLS, UI_PAL_NORMAL);

    for (i = 0u; i < top; i++) {
        u8     t   = order[i];
        Team  *tm  = &g_teams[t];
        u16    row = (u16)(4u + i);
        u8     j   = (u8)(tm->wins + tm->draws + tm->losses);
        u16    pts = TEAM_POINTS(tm->wins, tm->draws);
        u16    pal = (t == g_player_team_idx) ? UI_PAL_SELECT : UI_PAL_NORMAL;

        if (t == g_player_team_idx) ui_fill_row(row, UI_PAL_SELECT);
        ui_printf(0u, row, pal, "%2u %-18s %2u %2u %2u %2u  %3u",
                  (u16)(i + 1u), tm->name, (u16)j,
                  (u16)tm->wins, (u16)tm->draws, (u16)tm->losses, pts);
    }

    ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
    wait_confirm();
}

void screen_play_round(void) {
    LeagueRound rnd;
    u8 total_rounds = league_total_rounds(g_division);

    if (g_round > total_rounds) {
        ui_clear();
        ui_puts(8u, 13u, UI_PAL_NORMAL, "Temporada terminada!");
        wait_confirm();
        return;
    }

    /* Show "simulating" message */
    ui_clear();
    ui_printf(8u, 12u, UI_PAL_NORMAL, "Simulando jornada %u...", (u16)g_round);
    ui_wait_vblank();
    ui_wait_vblank();

    /* Build and simulate */
    league_build_round(g_division, g_round, &rnd);
    league_simulate_round(&rnd);

    /* Pay salaries every 4 rounds */
    if ((g_round % 4u) == 0u) {
        economy_pay_monthly_salaries();
    }

    /* Show results then standings */
    show_round_results(&rnd, g_division);
    show_standings_brief(g_division);

    g_round++;
}
