/*
 * screens/cup_screen.c -- Ecra da Taca (eliminatorias em 2 maos)
 */

#include <genesis.h>
#include "cup_screen.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"
#include "../game/cup.h"
#include "../game/types.h"

static void wait_continue(void) {
    ui_puts(0u, 27u, UI_PAL_NORMAL, "A/B: continuar");
    for (;;) {
        ui_wait_vblank();
        input_update();
        if (input_pressed(BTN_CONFIRM) || input_pressed(BTN_CANCEL)
                || input_pressed(BTN_START)) return;
    }
}

/* Show all ties for the current phase */
static void show_ties(u8 leg) {
    u8 i;

    ui_clear();
    ui_printf(5u, 0u, UI_PAL_NORMAL, "TACA -- %s -- %s MAO",
              cup_phase_name(g_cup_phase),
              leg == 0u ? "1a" : "2a");
    ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);

    for (i = 0u; i < g_cup_ties_count && i < 12u; i++) {
        CupTie *ct  = &g_cup_ties[i];
        u16     row = (u16)(3u + i * 2u);
        u16     pal = UI_PAL_NORMAL;
        u8      is_player = (ct->team_a == g_player_team_idx
                          || ct->team_b == g_player_team_idx);

        if (is_player) { pal = UI_PAL_SELECT; ui_fill_row(row, UI_PAL_SELECT); }

        if (leg == 0u) {
            ui_printf(0u, row, pal, "%-13s vs %-13s",
                      g_teams[ct->team_a].name,
                      g_teams[ct->team_b].name);
        } else {
            /* Show leg 1 result and leg 2 result */
            ui_printf(0u, row, pal, "%-11s %u-%u  (1:%u-%u)  %u-%u %-11s",
                      g_teams[ct->team_a].name,
                      (u16)ct->goals_a_leg2, (u16)ct->goals_b_leg2,
                      (u16)ct->goals_a_leg1, (u16)ct->goals_b_leg1,
                      (u16)ct->goals_a_leg1 + ct->goals_a_leg2,
                      (u16)ct->goals_b_leg1 + ct->goals_b_leg2,
                      g_teams[ct->team_b].name);
        }
    }

    ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
    wait_continue();
}

void screen_cup(void) {
    /* Draw if not yet done for this phase */
    if (g_cup_ties_count == 0u) {
        u8 n = cup_draw();
        if (n == 0u) {
            ui_clear();
            ui_puts(8u, 13u, UI_PAL_NORMAL, "Nenhuma equipa na Taca.");
            wait_continue();
            return;
        }
    }

    /* Check if player's team is still active */
    if (!cup_team_active(g_player_team_idx)) {
        ui_clear();
        ui_puts(6u, 10u, UI_PAL_NORMAL, "A sua equipa foi eliminada da Taca.");
        ui_printf(6u, 12u, UI_PAL_NORMAL, "Fase: %s", cup_phase_name(g_cup_phase));
        wait_continue();
        return;
    }

    /* Simulate "showing" message */
    ui_clear();
    ui_printf(8u, 12u, UI_PAL_NORMAL, "TACA -- %s",
              cup_phase_name(g_cup_phase));
    ui_puts(8u, 14u, UI_PAL_NORMAL, "A simular 1a mao...");
    ui_wait_vblank(); ui_wait_vblank();

    /* Leg 1 */
    cup_simulate_leg(0u);
    show_ties(0u);

    /* Leg 2 */
    ui_clear();
    ui_puts(8u, 13u, UI_PAL_NORMAL, "A simular 2a mao...");
    ui_wait_vblank(); ui_wait_vblank();

    cup_simulate_leg(1u);
    show_ties(1u);

    /* Advance to next phase */
    {
        u8 winner = cup_advance();
        if (winner != 0xFFu) {
            /* Cup final decided */
            ui_clear();
            ui_puts(8u, 5u, UI_PAL_NORMAL, "*** VENCEDOR DA TACA ***");
            ui_hline(6u, 7u, 28u, UI_PAL_NORMAL);
            ui_puts(12u, 10u, UI_PAL_SELECT, g_teams[winner].name);
            ui_fill_row(10u, UI_PAL_SELECT);
            if (winner == g_player_team_idx) {
                ui_puts(8u, 14u, UI_PAL_NORMAL, "PARABENS! Ganhou a Taca!");
            }
            wait_continue();
        }
    }
}
