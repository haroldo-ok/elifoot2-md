/*
 * main.c -- Entrada principal do Elifoot II Genesis
 */

#include <genesis.h>
#include "engine/ui.h"
#include "engine/input.h"
#include "engine/rng.h"
#include "game/data.h"
#include "screens/title.h"
#include "screens/main_menu.h"
#include "screens/squad.h"
#include "screens/finances.h"

int main(int hardReset) {
    (void)hardReset;

    JOY_init();
    VDP_init();
    ui_init();
    input_init();
    rng_init(GET_VCOUNTER);
    data_init();

    g_player_team_idx = screen_title();
    g_division = g_teams[g_player_team_idx].division;
    g_money    = g_teams[g_player_team_idx].money;
    g_round    = 1u;

    for (;;) {
        u8 choice = screen_main_menu();

        switch (choice) {
            case MENU_RESULT_SQUAD:
                screen_squad();
                break;
            case MENU_RESULT_FINANCES:
                screen_finances();
                break;
            case MENU_RESULT_PLAY:
                /* TODO: simulate_round() */
                ui_clear();
                ui_puts(10u, 13u, UI_PAL_NORMAL, "Simulacao em breve...");
                { u16 t = 0u; while (t < 180u) { ui_wait_vblank(); input_update(); t++; if (input_pressed(BTN_CANCEL)) break; } }
                break;
            case MENU_RESULT_TRANSFERS:
                ui_clear();
                ui_puts(10u, 13u, UI_PAL_NORMAL, "Transferencias em breve...");
                { u16 t = 0u; while (t < 180u) { ui_wait_vblank(); input_update(); t++; if (input_pressed(BTN_CANCEL)) break; } }
                break;
            case MENU_RESULT_COACHES:
                ui_clear();
                ui_puts(10u, 13u, UI_PAL_NORMAL, "Treinadores em breve...");
                { u16 t = 0u; while (t < 180u) { ui_wait_vblank(); input_update(); t++; if (input_pressed(BTN_CANCEL)) break; } }
                break;
            case MENU_RESULT_SAVE:
                ui_clear();
                ui_puts(10u, 13u, UI_PAL_NORMAL, "Guardar em breve...");
                { u16 t = 0u; while (t < 180u) { ui_wait_vblank(); input_update(); t++; if (input_pressed(BTN_CANCEL)) break; } }
                break;
            default:
                break;
        }
    }

    return 0;
}
