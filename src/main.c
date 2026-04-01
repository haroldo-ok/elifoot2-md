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

int main(int hardReset) {
    (void)hardReset;

    JOY_init();
    VDP_init();
    ui_init();
    input_init();
    rng_init(GET_VCOUNTER);
    data_init();

    /* Seleccao de equipa */
    g_player_team_idx = screen_title();
    /* Update game state for chosen team */
    g_division = g_teams[g_player_team_idx].division;
    g_money    = g_teams[g_player_team_idx].money;
    g_round    = 1u;

    /* Loop principal do jogo */
    for (;;) {
        u8 choice = screen_main_menu();

        switch (choice) {
            case MENU_RESULT_SQUAD:
                /* TODO: screen_squad() */
                break;
            case MENU_RESULT_FINANCES:
                /* TODO: screen_finances() */
                break;
            case MENU_RESULT_PLAY:
                /* TODO: simulate_round() */
                break;
            case MENU_RESULT_TRANSFERS:
                /* TODO: screen_transfers() */
                break;
            case MENU_RESULT_COACHES:
                /* TODO: screen_coaches() */
                break;
            case MENU_RESULT_SAVE:
                /* TODO: sram_save() */
                break;
            default:
                break;
        }
    }

    return 0;
}
