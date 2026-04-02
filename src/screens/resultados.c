/*
 * screens/resultados.c -- Resultados anteriores
 */

#include <genesis.h>
#include "resultados.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"
#include "../game/types.h"

void screen_resultados(void) {
    u8  top    = 0u;
    u8  redraw = 1u;
    u8  total  = g_results_count;

    for (;;) {
        ui_wait_vblank();
        input_update();

        if (input_pressed(BTN_CANCEL) || input_pressed(BTN_START)) return;

        if (input_repeat(BTN_DOWN) && (u8)(top + 20u) < total) { top++; redraw = 1u; }
        if (input_repeat(BTN_UP)   && top > 0u)                 { top--; redraw = 1u; }

        if (!redraw) continue;
        redraw = 0u;

        ui_clear();
        ui_puts(10u, 0u, UI_PAL_NORMAL, "RESULTADOS ANTERIORES");
        ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);

        if (total == 0u) {
            ui_puts(8u, 13u, UI_PAL_NORMAL, "Nao ha resultados ainda.");
        } else {
            ui_puts(0u, 2u, UI_PAL_NORMAL, "Jorn Equipa Casa     Res  Equipa Fora");
            ui_hline(0u, 3u, UI_COLS, UI_PAL_NORMAL);

            {
                u8 i;
                for (i = 0u; i < 20u && (u8)(top + i) < total; i++) {
                    MatchResult *mr  = &g_results[top + i];
                    u16          row = (u16)(4u + i);
                    u8  is_player   = (mr->home_team == g_player_team_idx
                                    || mr->away_team == g_player_team_idx);
                    u16 pal = is_player ? UI_PAL_SELECT : UI_PAL_NORMAL;
                    if (is_player) ui_fill_row(row, UI_PAL_SELECT);
                    ui_printf(0u, row, pal, " %2u  %-14s %u-%u  %s",
                              (u16)mr->round,
                              g_teams[mr->home_team].name,
                              (u16)mr->home_goals,
                              (u16)mr->away_goals,
                              g_teams[mr->away_team].name);
                }
            }
        }

        ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
        ui_puts(0u, 27u, UI_PAL_NORMAL, "CIMA/BAIXO: navegar  B: voltar");
    }
}
