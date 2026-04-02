/*
 * screens/main_menu.c -- Menu principal (versao final)
 */

#include <genesis.h>
#include "main_menu.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"

typedef enum {
    OPT_SQUAD      = 0,
    OPT_FINANCES   = 1,
    OPT_PLAY       = 2,
    OPT_TRANSFERS  = 3,
    OPT_COACHES    = 4,
    OPT_CUP        = 5,
    OPT_STANDINGS  = 6,
    OPT_PALMARES   = 7,
    OPT_SAVE       = 8,
    OPT_COUNT      = 9
} MenuOpt;

static const char * const s_labels[OPT_COUNT] = {
    "Plantel",
    "Financas",
    "Jogar Jornada",
    "Transferencias",
    "Treinadores",
    "Taca",
    "Classificacao",
    "Palmares",
    "Guardar Jogo"
};

static const u8 s_results[OPT_COUNT] = {
    MENU_RESULT_SQUAD, MENU_RESULT_FINANCES, MENU_RESULT_PLAY,
    MENU_RESULT_TRANSFERS, MENU_RESULT_COACHES, MENU_RESULT_CUP,
    MENU_RESULT_STANDINGS, MENU_RESULT_PALMARES, MENU_RESULT_SAVE
};

u8 screen_main_menu(void) {
    u8  sel    = OPT_PLAY;
    u8  redraw = 1u;
    u8  i;

    for (;;) {
        ui_wait_vblank();
        input_update();

        if (input_pressed(BTN_CANCEL)) return MENU_RESULT_BACK;
        if (input_pressed(BTN_CONFIRM) || input_pressed(BTN_START))
            return s_results[sel];

        if (input_repeat(BTN_DOWN) && sel < (u8)(OPT_COUNT - 1u)) { sel++; redraw = 1u; }
        if (input_repeat(BTN_UP)   && sel > 0u)                    { sel--; redraw = 1u; }

        if (!redraw) continue;
        redraw = 0u;

        ui_clear();

        ui_printf(0u, 0u, UI_PAL_NORMAL, "%-20s  Jornada %2u  Div%u",
                  g_teams[g_player_team_idx].name,
                  (u16)g_round, (u16)(g_division + 1u));
        ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);
        ui_puts(13u, 3u, UI_PAL_NORMAL, "MENU PRINCIPAL");
        ui_hline(12u, 4u, 16u, UI_PAL_NORMAL);

        for (i = 0u; i < (u8)OPT_COUNT; i++) {
            u16 row = (u16)(6u + i);
            u16 pal = (i == sel) ? UI_PAL_SELECT : UI_PAL_NORMAL;
            if (i == sel) ui_fill_row(row, UI_PAL_SELECT);
            ui_puts(13u, row, pal, s_labels[i]);
        }

        ui_printf(0u, 24u, UI_PAL_NORMAL, "Dinheiro: %ld Esc", g_money);
        ui_hline(0u, 25u, UI_COLS, UI_PAL_NORMAL);
        ui_puts(0u, 26u, UI_PAL_NORMAL, "CIMA/BAIXO: navegar  A: confirmar");
    }
}
