/*
 * screens/palmares.c -- Palmares (historial de temporadas)
 */

#include <genesis.h>
#include "palmares.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"
#include "../game/types.h"

void screen_palmares(void) {
    u8 i;

    ui_clear();
    ui_puts(14u, 0u, UI_PAL_NORMAL, "PALMARES");
    ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);
    ui_puts(0u, 2u, UI_PAL_NORMAL, "Temp  Div  Pos  Campeao Div1      Vencedor Taca");
    ui_hline(0u, 3u, UI_COLS, UI_PAL_NORMAL);

    if (g_season_num <= 1u) {
        ui_puts(8u, 12u, UI_PAL_NORMAL, "Sem historial ainda.");
    } else {
        for (i = 0u; i < (u8)PALMARES_COUNT; i++) {
            SeasonRecord *sr = &g_palmares[i];
            u16 row = (u16)(4u + i);
            if (sr->season_num == 0u) break;
            ui_printf(0u, row, UI_PAL_NORMAL,
                      " %2u   Div%u  %2u   %-18s %-18s",
                      (u16)sr->season_num,
                      (u16)(sr->player_div + 1u),
                      (u16)(sr->player_pos + 1u),
                      sr->div1_champion < (u8)TEAM_COUNT
                          ? g_teams[sr->div1_champion].name : "---",
                      sr->cup_champion  < (u8)TEAM_COUNT
                          ? g_teams[sr->cup_champion].name  : "---");
        }
    }

    ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
    ui_puts(0u, 27u, UI_PAL_NORMAL, "B: voltar");

    for (;;) {
        ui_wait_vblank();
        input_update();
        if (input_pressed(BTN_CANCEL) || input_pressed(BTN_START)) return;
    }
}
