/*
 * screens/squad.c -- Ecra do plantel
 */

#include <genesis.h>
#include "squad.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"

static const char s_pos[4][3] = { "GR", "DF", "MD", "AV" };

void screen_squad(void) {
    u8  start;
    u8  sel   = 0u;
    u8  total;
    u8  redraw = 1u;
    u8  i;
    Team   *team = &g_teams[g_player_team_idx];
    Player *pl;

    total = team->player_count;
    start = team->player_start;

    for (;;) {
        ui_wait_vblank();
        input_update();

        if (input_pressed(BTN_CANCEL) || input_pressed(BTN_START))
            return;

        if (input_repeat(BTN_DOWN) && sel < (u8)(total - 1u)) {
            sel++; redraw = 1u;
        }
        if (input_repeat(BTN_UP) && sel > 0u) {
            sel--; redraw = 1u;
        }
        /* Toggle titular/suplente com A */
        if (input_pressed(BTN_CONFIRM)) {
            pl = &g_players[(u16)(start + sel)];
            pl->on_field = pl->on_field ? 0u : 1u;
            redraw = 1u;
        }

        if (!redraw) continue;
        redraw = 0u;

        ui_clear();
        ui_puts( 0u, 0u, UI_PAL_NORMAL, team->name);
        ui_puts(28u, 0u, UI_PAL_NORMAL, "PLANTEL");
        ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);

        /* Cabecalho da tabela */
        ui_puts(0u, 2u, UI_PAL_NORMAL, "#  Nome            Pos For Sal     T");
        ui_hline(0u, 3u, UI_COLS, UI_PAL_NORMAL);

        for (i = 0u; i < total && i < 22u; i++) {
            u16 row = (u16)(4u + i);
            u16 pal = (i == sel) ? UI_PAL_SELECT : UI_PAL_NORMAL;
            pl = &g_players[(u16)(start + i)];

            if (i == sel) ui_fill_row(row, UI_PAL_SELECT);

            ui_printf(0u, row, pal, "%2u %-15s %2s  %2u %7ld %s",
                      (u16)(i + 1u),
                      pl->name,
                      s_pos[pl->pos < 4u ? pl->pos : 0u],
                      (u16)pl->strength,
                      pl->salary,
                      pl->on_field ? "T" : "S");
        }

        ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
        ui_puts(0u, 27u, UI_PAL_NORMAL, "A:titular/suplente  B:voltar");
    }
}
