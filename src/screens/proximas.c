/*
 * screens/proximas.c -- Proximas Jornadas
 */

#include <genesis.h>
#include "proximas.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"
#include "../game/league.h"

void screen_proximas(void) {
    u8  total = league_total_rounds(g_division);
    u8  view_round = g_round;  /* start from current round */
    u8  redraw = 1u;

    if (g_round > total) {
        ui_clear();
        ui_puts(6u, 13u, UI_PAL_NORMAL, "Temporada terminada.");
        ui_puts(6u, 15u, UI_PAL_NORMAL, "B: voltar");
        for(;;){ ui_wait_vblank(); input_update(); if(input_pressed(BTN_CANCEL)||input_pressed(BTN_START)) return; }
    }

    for (;;) {
        ui_wait_vblank();
        input_update();

        if (input_pressed(BTN_CANCEL) || input_pressed(BTN_START)) return;
        if (input_repeat(BTN_DOWN) && view_round < total)  { view_round++; redraw = 1u; }
        if (input_repeat(BTN_UP)   && view_round > g_round){ view_round--; redraw = 1u; }

        if (!redraw) continue;
        redraw = 0u;

        ui_clear();
        ui_printf(0u, 0u, UI_PAL_NORMAL,
                  "PROXIMAS JORNADAS -- Div%u", (u16)(g_division + 1u));
        ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);

        {
            u8 r, row = 3u;
            u8 rounds_shown = 0u;
            for (r = view_round; r <= total && rounds_shown < 4u; r++, rounds_shown++) {
                LeagueRound rnd;
                u8 i;
                league_build_round(g_division, r, &rnd);
                ui_printf(0u, (u16)row, UI_PAL_NORMAL, "%uu JORNADA", (u16)r);
                row++;
                for (i = 0u; i < rnd.count && row < 26u; i++) {
                    u8  h = rnd.home[i];
                    u8  a = rnd.away[i];
                    u16 pal = (h == g_player_team_idx || a == g_player_team_idx)
                              ? UI_PAL_SELECT : UI_PAL_NORMAL;
                    if (pal == UI_PAL_SELECT) ui_fill_row((u16)row, UI_PAL_SELECT);
                    ui_printf(2u, (u16)row, pal, "%-17s vs %s",
                              g_teams[h].name, g_teams[a].name);
                    row++;
                }
                if (row < 26u) { ui_hline(0u, (u16)row, UI_COLS, UI_PAL_NORMAL); row++; }
            }
        }

        ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
        ui_printf(0u, 27u, UI_PAL_NORMAL,
                  "Jornada %u/%u  CIMA/BAIXO: navegar  B: sair",
                  (u16)view_round, (u16)total);
    }
}
