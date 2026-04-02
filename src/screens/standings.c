/*
 * screens/standings.c -- Classificacao consultavel
 */

#include <genesis.h>
#include "standings.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"
#include "../game/types.h"

void screen_standings(void) {
    u8  order[29];
    u8  count, i;
    u8  div    = g_division;
    u8  redraw = 1u;

    for (;;) {
        ui_wait_vblank();
        input_update();

        if (input_pressed(BTN_CANCEL) || input_pressed(BTN_START)) return;

        /* C: toggle between player's division and div1 */
        if (input_pressed(BTN_ACTION)) {
            div = (div == 0u) ? g_division : 0u;
            redraw = 1u;
        }

        if (!redraw) continue;
        redraw = 0u;

        data_sort_standings(div, order, &count);

        ui_clear();
        ui_printf(0u, 0u, UI_PAL_NORMAL,
                  "CLASSIFICACAO -- Div%u  Jornada %u",
                  (u16)(div + 1u), (u16)g_round);
        ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);
        ui_puts(0u, 2u, UI_PAL_NORMAL, "#  Equipa             J  V  E  D  G+ G- Pts");
        ui_hline(0u, 3u, UI_COLS, UI_PAL_NORMAL);

        for (i = 0u; i < count && i < 20u; i++) {
            u8    t   = order[i];
            Team *tm  = &g_teams[t];
            u16   row = (u16)(4u + i);
            u16   pts = TEAM_POINTS(tm->wins, tm->draws);
            u8    j   = (u8)(tm->wins + tm->draws + tm->losses);
            u16   pal = (t == g_player_team_idx) ? UI_PAL_SELECT : UI_PAL_NORMAL;

            if (t == g_player_team_idx) ui_fill_row(row, UI_PAL_SELECT);
            ui_printf(0u, row, pal, "%2u %-18s %2u %2u %2u %2u %3u %3u %3u",
                      (u16)(i + 1u), tm->name, (u16)j,
                      (u16)tm->wins, (u16)tm->draws, (u16)tm->losses,
                      (u16)tm->goals_for, (u16)tm->goals_against, pts);
        }

        /* Top scorer */
        {
            u16  best_goals = 0u;
            u16  best_pi    = 0xFFFFu;
            u16  pi;
            u16  total_players = (u16)((u16)TEAM_COUNT * 16u);
            for (pi = 0u; pi < total_players; pi++) {
                if (g_goals[pi] > best_goals) {
                    best_goals = g_goals[pi];
                    best_pi    = pi;
                }
            }
            if (best_pi != 0xFFFFu && best_goals > 0u) {
                ui_printf(0u, 25u, UI_PAL_NORMAL,
                          "Art: %-15s %2u gol",
                          g_players[best_pi].name,
                          best_goals);
            }
        }
        ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
        ui_puts(0u, 27u, UI_PAL_NORMAL, "C: outra divisao  B: voltar");
    }
}
