/*
 * screens/finances.c -- Ecra de financas
 */

#include <genesis.h>
#include "finances.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"
#include "stadium.h"

void screen_finances(void) {
    Team *team = &g_teams[g_player_team_idx];
    u8 i;
    long total_salary = 0L;

    /* Calcular folha salarial */
    for (i = 0u; i < team->player_count; i++) {
        total_salary += g_players[(u16)(team->player_start + i)].salary;
    }

    ui_clear();
    ui_puts(14u, 0u, UI_PAL_NORMAL, "FINANCAS");
    ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);

    ui_printf( 2u,  3u, UI_PAL_NORMAL, "Equipa:          %-18s", team->name);
    ui_printf( 2u,  5u, UI_PAL_NORMAL, "Dinheiro:        %ld Esc", g_money);
    ui_printf( 2u,  7u, UI_PAL_NORMAL, "Folha salarial:  %ld Esc/mes", total_salary);
    ui_printf( 2u,  9u, UI_PAL_NORMAL, "Estadio:         %u lugares", (u16)team->stadium_cap);
    ui_printf( 2u, 11u, UI_PAL_NORMAL, "Bilhete:         %u Esc", (u16)team->ticket_price);
    ui_printf( 2u, 13u, UI_PAL_NORMAL, "Jornada:         %u", (u16)g_round);

    ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
    ui_puts(0u, 27u, UI_PAL_NORMAL, "C: ver estadio   B: voltar");

    /* Espera B para sair ou C para estadio */
    for (;;) {
        ui_wait_vblank();
        input_update();
        if (input_pressed(BTN_CANCEL) || input_pressed(BTN_START)) return;
        if (input_pressed(BTN_ACTION)) { screen_stadium(); return; }
    }
}
