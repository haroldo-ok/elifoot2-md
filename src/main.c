/*
 * main.c -- Entrada principal do Elifoot II Genesis
 */

#include <genesis.h>
#include "engine/ui.h"
#include "engine/input.h"
#include "engine/rng.h"
#include "game/data.h"
#include "screens/title.h"

int main(int hardReset) {
    (void)hardReset;

    JOY_init();
    VDP_init();

    ui_init();
    input_init();
    rng_init(GET_VCOUNTER);
    data_init();

    g_player_team_idx = screen_title();

    /* TODO: avancar para o menu principal */
    for (;;) {
        ui_wait_vblank();
    }

    return 0;
}
