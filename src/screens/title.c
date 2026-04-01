/*
 * screens/title.c -- Ecra de seleccao de equipa
 *
 * Logica pura: navegar lista, devolver indice.
 * Display: apenas ui_*. Nenhum detalhe VDP aqui.
 */

#include <genesis.h>
#include "title.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"

#define PAGE_TEAMS  18u

u8 screen_title(void) {
    u8  sel   = 0u;
    u8  top   = 0u;
    u8  total = (u8)TEAM_COUNT;
    u8  i;
    u8  redraw = 1u;

    for (;;) {
        ui_wait_vblank();
        input_update();

        if (input_repeat(BTN_DOWN)) {
            if (sel < (u8)(total - 1u)) {
                sel++;
                if (sel >= (u8)(top + PAGE_TEAMS)) top++;
                redraw = 1u;
            }
        }
        if (input_repeat(BTN_UP)) {
            if (sel > 0u) {
                sel--;
                if (sel < top) top = sel;
                redraw = 1u;
            }
        }

        if (input_pressed(BTN_CONFIRM) || input_pressed(BTN_ACTION)
            || input_pressed(BTN_START)) {
            return sel;
        }

        if (!redraw) continue;
        redraw = 0u;

        ui_clear();

        ui_puts(13u, 0u, UI_PAL_NORMAL, "* ELIFOOT II *");
        ui_puts( 0u, 1u, UI_PAL_NORMAL, "Selecione a sua equipa:");
        ui_hline(0u, 2u, UI_COLS, UI_PAL_NORMAL);

        for (i = 0u; i < PAGE_TEAMS; i++) {
            u8  idx = (u8)(top + i);
            u16 row = (u16)(4u + i);
            u16 pal;

            if (idx >= total) break;

            pal = (idx == sel) ? UI_PAL_SELECT : UI_PAL_NORMAL;
            if (idx == sel) ui_fill_row(row, UI_PAL_SELECT);
            ui_printf(1u, row, pal, "%2u. %s",
                      (u16)(idx + 1u), g_teams[idx].name);
        }

        ui_hline(0u, 25u, UI_COLS, UI_PAL_NORMAL);
        ui_puts( 0u, 26u, UI_PAL_NORMAL, "CIMA/BAIXO: navegar   A/C: confirmar");
    }
}
