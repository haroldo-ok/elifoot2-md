/*
 * screens/calendario.c -- Calendario completo da temporada
 */

#include <genesis.h>
#include "calendario.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"
#include "../game/league.h"

void screen_calendario(void) {
    u8  total  = league_total_rounds(g_division);
    u8  view   = 1u;
    u8  redraw = 1u;

    for (;;) {
        ui_wait_vblank();
        input_update();

        if (input_pressed(BTN_CANCEL) || input_pressed(BTN_START)) return;
        if (input_repeat(BTN_DOWN) && view < total)  { view++; redraw = 1u; }
        if (input_repeat(BTN_UP)   && view > 1u)     { view--; redraw = 1u; }

        if (!redraw) continue;
        redraw = 0u;

        ui_clear();
        ui_printf(0u, 0u, UI_PAL_NORMAL,
                  "CALENDARIO -- Div%u  (%u jornadas)", (u16)(g_division + 1u), (u16)total);
        ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);

        {
            LeagueRound rnd;
            u8 i, row = 3u;
            u8 status = (view < g_round) ? 1u : (view == g_round ? 2u : 0u);
            /* status: 0=futura, 1=jogada, 2=actual */
            const char *tag = (status == 1u) ? "[jog]" : (status == 2u) ? "[***]" : "     ";

            ui_printf(1u, 2u, UI_PAL_NORMAL, "%uu JORNADA  %s", (u16)view, tag);
            ui_hline(0u, (u16)(row-1u), UI_COLS, UI_PAL_NORMAL);

            league_build_round(g_division, view, &rnd);
            for (i = 0u; i < rnd.count && row < 26u; i++) {
                u8  h = rnd.home[i];
                u8  a = rnd.away[i];
                u8  is_my = (h == g_player_team_idx || a == g_player_team_idx);
                u16 pal = is_my ? UI_PAL_SELECT : UI_PAL_NORMAL;
                if (is_my) ui_fill_row((u16)row, UI_PAL_SELECT);

                /* If already played, show result */
                {
                    u8 shown = 0u;
                    if (status == 1u) {
                        u8 r;
                        for (r = 0u; r < g_results_count; r++) {
                            MatchResult *mr = &g_results[r];
                            if (mr->round == view && mr->home_team == h && mr->away_team == a) {
                                ui_printf(2u, (u16)row, pal, "%-16s %u-%u  %s",
                                          g_teams[h].name, (u16)mr->home_goals,
                                          (u16)mr->away_goals, g_teams[a].name);
                                shown = 1u;
                                break;
                            }
                        }
                    }
                    if (!shown) {
                        ui_printf(2u, (u16)row, pal, "%-16s vs   %s",
                                  g_teams[h].name, g_teams[a].name);
                    }
                }
                row++;
            }
        }

        ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
        ui_printf(0u, 27u, UI_PAL_NORMAL,
                  "Jornada %u/%u  CIMA/BAIXO: navegar  B: sair",
                  (u16)view, (u16)total);
    }
}
