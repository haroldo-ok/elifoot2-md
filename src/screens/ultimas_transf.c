/*
 * screens/ultimas_transf.c -- Ultimas Transferencias Realizadas
 *
 * ULTIMAS TRANSFERENCIAS REALIZADAS
 * JOGADOR  PS FC  DE            PARA            ORD
 */

#include <genesis.h>
#include "ultimas_transf.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"

static const char s_pos_ut[4][3] = { "GR", "DF", "MD", "AV" };

void screen_ultimas_transf(void) {
    u8  top    = 0u;
    u8  total  = g_transfer_count;
    u8  redraw = 1u;

    for (;;) {
        ui_wait_vblank();
        input_update();
        if (input_pressed(BTN_CANCEL) || input_pressed(BTN_START)) return;
        if (input_repeat(BTN_DOWN) && (u8)(top + 18u) < total) { top++; redraw = 1u; }
        if (input_repeat(BTN_UP)   && top > 0u)                 { top--; redraw = 1u; }

        if (!redraw) continue;
        redraw = 0u;

        ui_clear();
        ui_puts(4u, 0u, UI_PAL_NORMAL, "ULTIMAS TRANSFERENCIAS REALIZADAS");
        ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);

        if (total == 0u) {
            ui_puts(6u, 13u, UI_PAL_NORMAL, "Nao ha transferencias ainda.");
        } else {
            ui_puts(0u, 2u, UI_PAL_NORMAL, "JOGADOR        PS FC DE          PARA");
            ui_hline(0u, 3u, UI_COLS, UI_PAL_NORMAL);
            {
                u8 i;
                for (i = 0u; i < 18u && (u8)(top + i) < total; i++) {
                    TransferRecord *tr  = &g_transfer_history[top + i];
                    u16             row = (u16)(4u + i);
                    u8  is_mine = (tr->from_team == g_player_team_idx
                                || tr->to_team   == g_player_team_idx);
                    u16 pal = is_mine ? UI_PAL_SELECT : UI_PAL_NORMAL;
                    if (is_mine) ui_fill_row(row, UI_PAL_SELECT);
                    ui_printf(0u, row, pal, "%-13s %2s %2u %-11s %-11s",
                              tr->name,
                              s_pos_ut[tr->pos < 4u ? tr->pos : 0u],
                              (u16)tr->strength,
                              g_teams[tr->from_team].name,
                              g_teams[tr->to_team].name);
                }
            }
        }
        ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
        ui_puts(0u, 27u, UI_PAL_NORMAL, "CIMA/BAIXO: navegar  B: voltar");
    }
}
