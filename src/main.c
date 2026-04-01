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
#include "screens/play_round.h"

static void brief_msg(const char *msg) {
    u16 t;
    ui_clear();
    ui_puts(8u, 13u, UI_PAL_NORMAL, msg);
    ui_puts(8u, 15u, UI_PAL_NORMAL, "B: voltar");
    for (t = 0u; t < 300u; t++) {
        ui_wait_vblank();
        input_update();
        if (input_pressed(BTN_CANCEL) || input_pressed(BTN_START)) break;
    }
}

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
            case MENU_RESULT_SQUAD:      screen_squad();             break;
            case MENU_RESULT_FINANCES:   screen_finances();          break;
            case MENU_RESULT_PLAY:       screen_play_round();        break;
            case MENU_RESULT_TRANSFERS:  brief_msg("Transferencias em breve..."); break;
            case MENU_RESULT_COACHES:    brief_msg("Treinadores em breve...");    break;
            case MENU_RESULT_SAVE:       brief_msg("Guardar em breve...");        break;
            default: break;
        }
    }

    return 0;
}
