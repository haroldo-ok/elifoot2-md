/*
 * screens/coaches.c -- Gestao de treinadores
 */

#include <genesis.h>
#include "coaches.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"
#include "../game/types.h"

/* ------------------------------------------------------------------ */
u8 g_coach_boost_team   = 0xFFu;
u8 g_coach_boost_rounds = 0u;

void coaches_tick_boost(void) {
    if (g_coach_boost_team == 0xFFu) return;
    if (g_coach_boost_rounds > 0u) {
        g_coach_boost_rounds--;
    }
    if (g_coach_boost_rounds == 0u) {
        g_coach_boost_team = 0xFFu;
    }
}

/* ------------------------------------------------------------------ */
static void wait_any(void) {
    ui_puts(0u, 27u, UI_PAL_NORMAL, "A/B: continuar");
    for (;;) {
        ui_wait_vblank();
        input_update();
        if (input_pressed(BTN_CONFIRM) || input_pressed(BTN_CANCEL)
                || input_pressed(BTN_START)) return;
    }
}

/* ------------------------------------------------------------------ */
/* Sub-ecra: escolha de novo treinador                                 */
/* ------------------------------------------------------------------ */
static u8 pick_coach(void) {
    /* Returns selected coach index, or 0xFF if cancelled */
    u8  sel    = 0u;
    u8  top    = 0u;
    u8  redraw = 1u;

    for (;;) {
        ui_wait_vblank();
        input_update();

        if (input_pressed(BTN_CANCEL) || input_pressed(BTN_START))
            return 0xFFu;

        if (input_repeat(BTN_DOWN) && sel < (u8)(COACH_COUNT - 1u)) {
            sel++;
            if (sel >= (u8)(top + 18u)) top++;
            redraw = 1u;
        }
        if (input_repeat(BTN_UP) && sel > 0u) {
            sel--;
            if (sel < top) top = sel;
            redraw = 1u;
        }
        if (input_pressed(BTN_CONFIRM)) return sel;

        if (!redraw) continue;
        redraw = 0u;

        ui_clear();
        ui_puts(8u, 0u, UI_PAL_NORMAL, "ESCOLHA NOVO TREINADOR");
        ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);

        {
            u8 i;
            for (i = 0u; i < 18u; i++) {
                u8  idx = (u8)(top + i);
                u16 row = (u16)(3u + i);
                u16 pal;
                if (idx >= (u8)COACH_COUNT) break;
                pal = (idx == sel) ? UI_PAL_SELECT : UI_PAL_NORMAL;
                if (idx == sel) ui_fill_row(row, UI_PAL_SELECT);
                ui_printf(2u, row, pal, "%2u. %s",
                          (u16)(idx + 1u), g_coaches[idx]);
            }
        }

        ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
        ui_puts(0u, 27u, UI_PAL_NORMAL, "A: contratar  B: voltar");
    }
}

/* ------------------------------------------------------------------ */
void screen_coaches(void) {
    Team *team = &g_teams[g_player_team_idx];

    for (;;) {
        u8 redraw = 1u;

        for (;;) {
            ui_wait_vblank();
            input_update();

            if (input_pressed(BTN_START) || input_pressed(BTN_CANCEL))
                return;

            /* A = novo treinador */
            if (input_pressed(BTN_CONFIRM)) {
                u8 new_coach = pick_coach();
                if (new_coach != 0xFFu) {
                    u8 old_coach = team->coach_idx;
                    team->coach_idx      = new_coach;
                    g_coach_boost_team   = g_player_team_idx;
                    g_coach_boost_rounds = (u8)COACH_BOOST_ROUNDS;

                    ui_clear();
                    ui_puts(5u, 0u, UI_PAL_NORMAL, "CHICOTADA PSICOLOGICA!");
                    ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);
                    ui_printf(2u,  4u, UI_PAL_NORMAL, "%s foi despedido.",
                              g_coaches[old_coach]);
                    ui_printf(2u,  6u, UI_PAL_NORMAL, "Para o seu lugar foi escolhido");
                    ui_printf(2u,  7u, UI_PAL_NORMAL, "%s.", g_coaches[new_coach]);
                    ui_printf(2u, 10u, UI_PAL_NORMAL,
                              "+%u de forca por %u jornadas!",
                              (u16)COACH_BOOST_STRENGTH,
                              (u16)COACH_BOOST_ROUNDS);
                    wait_any();
                    redraw = 1u;
                }
                break;
            }

            if (!redraw) continue;
            redraw = 0u;

            ui_clear();
            ui_puts(10u, 0u, UI_PAL_NORMAL, "TREINADORES EM JOGO");
            ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);

            ui_printf(2u, 4u, UI_PAL_NORMAL, "Equipa:           %s", team->name);
            ui_printf(2u, 6u, UI_PAL_NORMAL, "Treinador actual: %s",
                      g_coaches[team->coach_idx]);

            if (g_coach_boost_team == g_player_team_idx
                    && g_coach_boost_rounds > 0u) {
                ui_printf(2u, 9u, UI_PAL_NORMAL,
                          "Boost activo: +%u forca (%u jornadas restantes)",
                          (u16)COACH_BOOST_STRENGTH,
                          (u16)g_coach_boost_rounds);
            } else {
                ui_puts(2u, 9u, UI_PAL_NORMAL, "Sem boost activo.");
            }

            ui_puts(2u, 13u, UI_PAL_NORMAL, "[A] Contratar novo treinador");
            ui_puts(2u, 15u, UI_PAL_NORMAL, "[B] Voltar ao menu");

            ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
            ui_puts(0u, 27u, UI_PAL_NORMAL, "A: novo treinador  B: voltar");
        }
    }
}
