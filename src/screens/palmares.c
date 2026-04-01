/*
 * screens/palmares.c -- PALMARES (?ltimas 5 temporadas)
 *
 * Exibe o historial das ?ltimas 5 temporadas com:
 *   - Campe?o da Div1
 *   - Campe?o da Copa (futuro)
 *   - Posi??o do jogador
 *   - Divis?o do jogador
 *
 * Fiel ao original: "PALMARES" sem acento (encoding ASCII do Genesis).
 */

#include <genesis.h>
#include "palmares.h"
#include "../game/data.h"
#include "../game/types.h"
#include "../engine/input.h"
#include "../engine/render.h"

void screen_palmares(void) {
    u16  row;
    u8   i;

    render_clear_content();
    row = (u16)CONTENT_ROW_FIRST;

    render_text(BG_A, "PALMARES -- ULTIMAS TEMPORADAS",
                5u, row++, PAL_MAIN);
    render_hline(BG_A, 1u, row++, 38u, BOX_DOUBLE, PAL_MAIN);
    render_text(BG_A, " Temp  Campeao Div1        Pos  Div",
                1u, row++, PAL_MAIN);
    render_hline(BG_A, 1u, row++, 38u, BOX_SIMPLE, PAL_MAIN);

    for (i = 0u; i < (u8)PALMARES_COUNT; i++) {
        SeasonRecord *rec = &g_palmares[i];

        if (rec->season_num == 0u) {
            render_text(BG_A, " ---  (sem dados)",
                        1u, row, PAL_MAIN);
        } else {
            render_textf(BG_A, 1u, row, PAL_MAIN,
                         "  %2u  %-18s %3u  Div%u",
                         (u16)rec->season_num,
                         g_teams[rec->div1_champion].name,
                         (u16)rec->player_pos,
                         (u16)(rec->player_div + 1u));
        }
        row++;
    }

    render_hline(BG_A, 1u, row++, 38u, BOX_SIMPLE, PAL_MAIN);
    render_textf(BG_A, 1u, row, PAL_MAIN,
                 "Temporada actual: %u  Divisao: Div%u",
                 (u16)g_season_num, (u16)(g_division + 1u));

    render_help_bar("[A/B] Voltar", NULL);

    for (;;) {
        SYS_doVBlankProcess();
        input_update();
        if (input_pressed(BTN_CONFIRM) || input_pressed(BTN_CANCEL)) return;
    }
}
