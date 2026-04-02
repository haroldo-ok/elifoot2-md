/*
 * screens/stadium.c -- Estadio: construir bancadas e configurar bilhete
 */

#include <genesis.h>
#include "stadium.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"

/* Custo e lugares por bancada (fiel ao original) */
#define STAND_COST    50000L   /* 50 000 Esc por bancada */
#define STAND_PLACES  2500u    /* 2 500 lugares por bancada */

void screen_stadium(void) {
    Team *team  = &g_teams[g_player_team_idx];
    u8    redraw = 1u;

    for (;;) {
        ui_wait_vblank();
        input_update();

        if (input_pressed(BTN_CANCEL) || input_pressed(BTN_START)) return;

        /* A: construir bancada */
        if (input_pressed(BTN_CONFIRM)) {
            if (g_money < STAND_COST) {
                ui_clear();
                ui_puts(6u, 12u, UI_PAL_NORMAL, "Dinheiro insuficiente!");
                ui_puts(6u, 14u, UI_PAL_NORMAL, "B: voltar");
                { u16 t; for(t=0u;t<90u;t++){ ui_wait_vblank(); input_update(); if(input_pressed(BTN_CANCEL)) break; } }
            } else {
                g_money -= STAND_COST;
                team->stadium_cap = (u16)(team->stadium_cap + STAND_PLACES);
                ui_clear();
                ui_puts(6u, 11u, UI_PAL_NORMAL, "Bancada construida!");
                ui_printf(6u, 13u, UI_PAL_NORMAL,
                          "Capacidade: %u lugares.", (u16)team->stadium_cap);
                ui_puts(6u, 15u, UI_PAL_NORMAL, "A/B: continuar");
                { u16 t; for(t=0u;t<120u;t++){ ui_wait_vblank(); input_update(); if(input_pressed(BTN_CONFIRM)||input_pressed(BTN_CANCEL)) break; } }
            }
            redraw = 1u;
            continue;
        }

        /* C: aumentar preco do bilhete (+100) */
        if (input_pressed(BTN_ACTION)) {
            if (team->ticket_price < 5000u)
                team->ticket_price = (u16)(team->ticket_price + 100u);
            redraw = 1u;
        }

        /* X: diminuir preco do bilhete (-100) */
        if (input_pressed(BUTTON_X)) {
            if (team->ticket_price >= 200u)
                team->ticket_price = (u16)(team->ticket_price - 100u);
            redraw = 1u;
        }

        if (!redraw) continue;
        redraw = 0u;

        ui_clear();
        ui_puts(13u, 0u, UI_PAL_NORMAL, "SITUACAO DO ESTADIO");
        ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);

        ui_printf(2u,  4u, UI_PAL_NORMAL, "Equipa:          %s", team->name);
        ui_printf(2u,  6u, UI_PAL_NORMAL, "Capacidade:      %u lugares",
                  (u16)team->stadium_cap);
        ui_printf(2u,  8u, UI_PAL_NORMAL, "Preco do bilhete: %u Esc",
                  (u16)team->ticket_price);
        ui_printf(2u, 10u, UI_PAL_NORMAL, "Dinheiro:        %ld Esc", g_money);
        ui_hline(2u, 12u, 36u, UI_PAL_NORMAL);
        ui_printf(2u, 14u, UI_PAL_NORMAL,
                  "Construir bancada: %u lug. por %ld Esc",
                  (u16)STAND_PLACES, STAND_COST);

        ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
        ui_puts(0u, 27u, UI_PAL_NORMAL, "A:bancada C:bil+ X:bil-  B:sair");
    }
}
