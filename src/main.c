/*
 * main.c -- Entrada principal do Elifoot II Genesis
 */

#include <genesis.h>
#include "engine/ui.h"
#include "engine/input.h"
#include "engine/rng.h"
#include "engine/sram_io.h"
#include "game/data.h"
#include "screens/title.h"
#include "screens/main_menu.h"
#include "screens/squad.h"
#include "screens/finances.h"
#include "screens/play_round.h"
#include "screens/transfers.h"
#include "screens/coaches.h"
#include "screens/save_load.h"
#include "screens/cup_screen.h"
#include "screens/palmares.h"
#include "screens/standings.h"
#include "screens/stadium.h"

/* Startup screen: Novo Jogo vs Continuar */
static u8 startup_screen(void) {
    u8  sel    = 0u;
    u8  redraw = 1u;

    sram_init();

    for (;;) {
        ui_wait_vblank();
        input_update();

        if (input_repeat(BTN_DOWN) && sel < 1u) { sel = 1u; redraw = 1u; }
        if (input_repeat(BTN_UP)   && sel > 0u) { sel = 0u; redraw = 1u; }
        if (input_pressed(BTN_CONFIRM) || input_pressed(BTN_START)) return sel;

        if (!redraw) continue;
        redraw = 0u;

        ui_clear();
        ui_puts(12u,  4u, UI_PAL_NORMAL, "* ELIFOOT II *");
        ui_puts(12u,  5u, UI_PAL_NORMAL, "  Genesis Port");
        ui_hline(8u,  7u, 24u, UI_PAL_NORMAL);

        {
            u16 p0 = (sel == 0u) ? UI_PAL_SELECT : UI_PAL_NORMAL;
            u16 p1 = (sel == 1u) ? UI_PAL_SELECT : UI_PAL_NORMAL;
            if (sel == 0u) ui_fill_row(12u, UI_PAL_SELECT);
            if (sel == 1u) ui_fill_row(15u, UI_PAL_SELECT);
            ui_puts(14u, 12u, p0, "Novo Jogo");
            ui_puts(14u, 15u, p1, "Continuar Jogo");
        }

        /* Show save slot info */
        {
            u8 s;
            for (s = 0u; s < 3u; s++) {
                if (sram_slot_valid(s)) {
                    ui_printf(8u, (u16)(18u + s), UI_PAL_NORMAL,
                              "Slot %u: Temp. %u", (u16)(s + 1u),
                              (u16)sram_slot_season(s));
                } else {
                    ui_printf(8u, (u16)(18u + s), UI_PAL_NORMAL,
                              "Slot %u: (vazio)", (u16)(s + 1u));
                }
            }
        }

        ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
        ui_puts(8u, 27u, UI_PAL_NORMAL, "CIMA/BAIXO: sel  A: confirmar");
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

    /* Startup: novo jogo ou continuar */
    {
        u8 choice = startup_screen();
        if (choice == 1u) {
            /* Continuar: load from slot */
            u8 loaded = 0u;
            u8 s;
            for (s = 0u; s < 3u && !loaded; s++) {
                if (sram_slot_valid(s)) {
                    loaded = sram_load(s);
                }
            }
            if (!loaded) {
                /* No valid save -- go to new game */
                g_player_team_idx = screen_title();
                g_division = g_teams[g_player_team_idx].division;
                g_money    = g_teams[g_player_team_idx].money;
                g_round    = 1u;
            }
        } else {
            g_player_team_idx = screen_title();
            g_division = g_teams[g_player_team_idx].division;
            g_money    = g_teams[g_player_team_idx].money;
            g_round    = 1u;
        }
    }

    for (;;) {
        u8 choice = screen_main_menu();
        switch (choice) {
            case MENU_RESULT_SQUAD:     screen_squad();      break;
            case MENU_RESULT_FINANCES:  screen_finances();   break;
            case MENU_RESULT_PLAY:      screen_play_round(); break;
            case MENU_RESULT_TRANSFERS: screen_transfers();  break;
            case MENU_RESULT_COACHES:   screen_coaches();    break;
            case MENU_RESULT_CUP:       screen_cup();        break;
            case MENU_RESULT_STANDINGS: screen_standings();  break;
            case MENU_RESULT_PALMARES:  screen_palmares();   break;
            case MENU_RESULT_SAVE:      screen_save_load();  break;
            default: break;
        }
    }

    return 0;
}
