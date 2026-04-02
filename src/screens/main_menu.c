/*
 * screens/main_menu.c -- Menu principal completo
 */

#include <genesis.h>
#include "main_menu.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"

/* Options -- split into two pages */
#define OPT_COUNT 14u

static const char * const s_labels[OPT_COUNT] = {
    "Plantel",          /*  0 */
    "Financas",         /*  1 */
    "Jogar Jornada",    /*  2 */
    "Transferencias",   /*  3 */
    "Treinadores",      /*  4 */
    "Taca",             /*  5 */
    "Classificacao",    /*  6 */
    "Palmares",         /*  7 */
    "Guardar Jogo",     /*  8 */
    "Marcadores",       /*  9 */
    "Prox. Jornadas",   /* 10 */
    "Resultados",       /* 11 */
    "Calendario",       /* 12 */
    "Ult. Transfer."     /* 13 */
};

static const u8 s_results[OPT_COUNT] = {
    MENU_RESULT_SQUAD, MENU_RESULT_FINANCES, MENU_RESULT_PLAY,
    MENU_RESULT_TRANSFERS, MENU_RESULT_COACHES, MENU_RESULT_CUP,
    MENU_RESULT_STANDINGS, MENU_RESULT_PALMARES, MENU_RESULT_SAVE,
    MENU_RESULT_MARCADORES, MENU_RESULT_PROXIMAS,
    MENU_RESULT_RESULTADOS, MENU_RESULT_CALENDARIO
};

u8 screen_main_menu(void) {
    u8  sel    = MENU_RESULT_PLAY;
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

        for (i = 0u; i < (u8)OPT_COUNT; i++) {
            u16 col = (i < 9u) ? 13u : 1u;
            u16 row = (i < 9u) ? (u16)(3u + i) : (u16)(3u + i - 9u);
            u16 pal = (i == sel) ? UI_PAL_SELECT : UI_PAL_NORMAL;
            if (i == 9u) {
                ui_hline(0u, 13u, UI_COLS, UI_PAL_NORMAL);
                ui_puts(0u, 14u, UI_PAL_NORMAL, "INFO:");
                row = (u16)(3u + i - 9u + 13u);
                col = 6u;
            }
            if (i >= 9u) row = (u16)(3u + (i - 9u) + 2u + 13u);
            if (i == sel) ui_fill_row(row, UI_PAL_SELECT);
            ui_puts(col, row, pal, s_labels[i]);
        }

        ui_printf(0u, 24u, UI_PAL_NORMAL, "Dinheiro: %ld Esc", g_money);
        ui_hline(0u, 25u, UI_COLS, UI_PAL_NORMAL);
        ui_puts(0u, 26u, UI_PAL_NORMAL, "CIMA/BAIXO: navegar  A: confirmar");
    }
}
