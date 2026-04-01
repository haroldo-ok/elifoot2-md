/*
 * screens/main_menu.c -- Menu principal do Elifoot II Genesis
 *
 * Aparece apos a seleccao de equipa.
 * Opcoes: Plantel | Financas | Jogar Jornada | Transferencias | Guardar | Sair
 */

#include <genesis.h>
#include "main_menu.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"

/* Opcoes do menu */
typedef enum {
    OPT_SQUAD      = 0,
    OPT_FINANCES   = 1,
    OPT_PLAY       = 2,
    OPT_TRANSFERS  = 3,
    OPT_COACHES    = 4,
    OPT_SAVE       = 5,
    OPT_COUNT      = 6
} MenuOpt;

static const char * const s_labels[OPT_COUNT] = {
    "Plantel",
    "Financas",
    "Jogar Jornada",
    "Transferencias",
    "Treinadores",
    "Guardar Jogo"
};

u8 screen_main_menu(void) {
    u8  sel    = OPT_PLAY;   /* default: jogar jornada */
    u8  redraw = 1u;
    u8  i;

    for (;;) {
        ui_wait_vblank();
        input_update();

        if (input_pressed(BTN_CANCEL)) {
            return MENU_RESULT_BACK;
        }
        if (input_pressed(BTN_CONFIRM) || input_pressed(BTN_START)) {
            return sel;
        }
        if (input_repeat(BTN_DOWN) && sel < (u8)(OPT_COUNT - 1u)) {
            sel++; redraw = 1u;
        }
        if (input_repeat(BTN_UP) && sel > 0u) {
            sel--; redraw = 1u;
        }

        if (!redraw) continue;
        redraw = 0u;

        ui_clear();

        /* Cabecalho com info da equipa e jornada */
        ui_printf(0u, 0u, UI_PAL_NORMAL, "%-20s  Jornada %2u  Div%u",
                  g_teams[g_player_team_idx].name,
                  (u16)g_round,
                  (u16)(g_division + 1u));
        ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);

        /* Titulo */
        ui_puts(15u, 3u, UI_PAL_NORMAL, "MENU PRINCIPAL");
        ui_hline(14u, 4u, 16u, UI_PAL_NORMAL);

        /* Lista de opcoes */
        for (i = 0u; i < (u8)OPT_COUNT; i++) {
            u16 row = (u16)(6u + i * 2u);
            u16 pal = (i == sel) ? UI_PAL_SELECT : UI_PAL_NORMAL;
            if (i == sel) ui_fill_row(row, UI_PAL_SELECT);
            ui_puts(14u, row, pal, s_labels[i]);
        }

        /* Dinheiro */
        ui_printf(0u, 24u, UI_PAL_NORMAL, "Dinheiro: %ld Esc", g_money);
        ui_hline(0u, 25u, UI_COLS, UI_PAL_NORMAL);
        ui_puts(0u, 26u, UI_PAL_NORMAL, "CIMA/BAIXO: navegar  A: confirmar  B: voltar");
    }
}
