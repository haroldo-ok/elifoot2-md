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

    /* Mostrar equipa seleccionada enquanto o menu principal nao esta pronto */
    ui_clear();
    ui_puts(10u, 12u, UI_PAL_NORMAL, "Equipa seleccionada:");
    ui_puts(10u, 14u, UI_PAL_NORMAL, g_teams[g_player_team_idx].name);
    ui_puts( 8u, 18u, UI_PAL_NORMAL, "Menu principal em breve...");

    for (;;) {
        ui_wait_vblank();
    }

    return 0;
}
