/*
 * screens/marcadores.c -- Melhores Marcadores da temporada
 */

#include <genesis.h>
#include "marcadores.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"

void screen_marcadores(void) {
    /* Build sorted list of scorers */
    u16 total_players = (u16)((u16)TEAM_COUNT * (u16)PLAYERS_PER_TEAM);
    u16 order[464];  /* max players */
    u16 count = 0u;
    u16 i, j;
    u8  redraw = 1u;
    u8  top    = 0u;

    /* Collect players with goals */
    for (i = 0u; i < total_players; i++) {
        if (g_goals[i] > 0u) order[count++] = i;
    }

    /* Sort descending by goals (insertion sort -- small N) */
    for (i = 1u; i < count; i++) {
        u16 key = order[i];
        j = i;
        while (j > 0u && g_goals[order[j-1u]] < g_goals[key]) {
            order[j] = order[j-1u];
            j--;
        }
        order[j] = key;
    }

    for (;;) {
        ui_wait_vblank();
        input_update();

        if (input_pressed(BTN_CANCEL) || input_pressed(BTN_START)) return;

        if (input_repeat(BTN_DOWN) && (u16)(top + 20u) < count) { top++; redraw = 1u; }
        if (input_repeat(BTN_UP)   && top > 0u)                  { top--; redraw = 1u; }

        if (!redraw) continue;
        redraw = 0u;

        ui_clear();
        ui_puts(9u, 0u, UI_PAL_NORMAL, "MELHORES MARCADORES");
        ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);

        if (count == 0u) {
            ui_puts(8u, 13u, UI_PAL_NORMAL, "Ainda nao ha golos marcados.");
        } else {
            ui_puts(0u, 2u, UI_PAL_NORMAL, "#  Jogador          Equipa           Gol");
            ui_hline(0u, 3u, UI_COLS, UI_PAL_NORMAL);

            for (i = 0u; i < 20u && (u16)(top + i) < count; i++) {
                u16    pi  = order[top + i];
                u16    row = (u16)(4u + i);
                /* Find team */
                u8     t;
                u8     team_idx = 0u;
                for (t = 0u; t < (u8)TEAM_COUNT; t++) {
                    u16 ps = g_teams[t].player_start;
                    u16 pe = (u16)(ps + g_teams[t].player_count);
                    if (pi >= ps && pi < pe) { team_idx = t; break; }
                }
                {
                    u16 pal = (team_idx == g_player_team_idx) ? UI_PAL_SELECT : UI_PAL_NORMAL;
                    if (team_idx == g_player_team_idx) ui_fill_row(row, UI_PAL_SELECT);
                    ui_printf(0u, row, pal, "%2u %-17s %-16s %3u",
                              (u16)(top + i + 1u),
                              g_players[pi].name,
                              g_teams[team_idx].name,
                              (u16)g_goals[pi]);
                }
            }
        }

        ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
        ui_puts(0u, 27u, UI_PAL_NORMAL, "CIMA/BAIXO: navegar  B: voltar");
    }
}
