/*
 * screens/coaches.c -- Gest?o de treinadores
 *
 * Strings literais fiel ao original:
 *   "TREINADORES EM JOGO"
 *   "[nome] foi despedido do [clube]"
 *   "Para o seu lugar foi escolhido [novo]"
 *   "F1  Anular treinador"  -> [A] Despedir
 *   "F2  Novo treinador"    -> [B] Contratar
 *   "Esc Fim"               -> [Start] Voltar
 */

#include <genesis.h>
#include "coaches.h"
#include "../game/data.h"
#include "../game/types.h"
#include "../engine/input.h"
#include "../engine/render.h"

/* ------------------------------------------------------------------ */
/* Estado global do boost                                              */
/* ------------------------------------------------------------------ */

u8 g_coach_boost_team   = 0xFFu;
u8 g_coach_boost_rounds = 0u;

/* ------------------------------------------------------------------ */
/* coaches_tick_boost()                                                */
/* ------------------------------------------------------------------ */

void coaches_tick_boost(void) {
    if (g_coach_boost_team == 0xFFu) return;
    if (g_coach_boost_rounds == 0u) {
        g_coach_boost_team = 0xFFu;
        return;
    }
    g_coach_boost_rounds--;
    if (g_coach_boost_rounds == 0u) {
        g_coach_boost_team = 0xFFu;
    }
}

/* ------------------------------------------------------------------ */
/* Render da lista de treinadores                                      */
/* ------------------------------------------------------------------ */

static void render_coach_list(ListNav *nav) {
    u16 row = (u16)CONTENT_ROW_FIRST;
    u8  i;

    render_clear_content();
    render_text(BG_A, "NOVO TREINADOR -- Escolha:",
                2u, row++, PAL_MAIN);
    render_hline(BG_A, 1u, row++, 38u, BOX_SIMPLE, PAL_MAIN);

    {
        u8 end = (u8)(nav->page_top + nav->page_size);
        if (end > (u8)COACH_COUNT) end = (u8)COACH_COUNT;

        for (i = nav->page_top; i < end; i++) {
            u16 pal = (i == nav->selected) ? PAL_SELECTED : PAL_MAIN;
            if (pal == PAL_SELECTED)
                render_fill_rect(BG_A, 1u, row, 38u, 1u, PAL_SELECTED, 0u);
            render_textf(BG_A, 2u, row, pal,
                         "%2u. %s", (u16)(i + 1u), g_coaches[i]);
            row++;
        }
        if (nav->page_top > 0u)
            render_text(BG_A, "^", 39u, (u16)(CONTENT_ROW_FIRST + 2u), PAL_MAIN);
        if (end < (u8)COACH_COUNT)
            render_text(BG_A, "v", 39u, (u16)(row - 1u), PAL_MAIN);
    }
    render_help_bar("[A] Contratar  [^v] Navegar  [B] Voltar", NULL);
}

/* ------------------------------------------------------------------ */
/* screen_coaches()                                                    */
/* ------------------------------------------------------------------ */

void screen_coaches(void) {
    Team   *team     = &g_teams[g_player_team_idx];
    u8      page_sz  = (u8)(CONTENT_ROW_LAST - CONTENT_ROW_FIRST - 3u);
    u16     row;

    for (;;) {
        /* ---- Tela principal ---- */
        render_clear_content();
        row = (u16)CONTENT_ROW_FIRST;

        render_text(BG_A, "TREINADORES EM JOGO",
                    10u, row++, PAL_MAIN);
        render_hline(BG_A, 1u, row++, 38u, BOX_DOUBLE, PAL_MAIN);
        row++;

        render_textf(BG_A, 2u, row++, PAL_MAIN,
                     "Equipa  : %-20s", team->name);
        render_textf(BG_A, 2u, row++, PAL_MAIN,
                     "Treinador actual: %-20s",
                     g_coaches[team->coach_idx]);

        if (g_coach_boost_team == g_player_team_idx &&
            g_coach_boost_rounds > 0u) {
            render_textf(BG_A, 2u, row++, PAL_MAIN,
                         "Boost activo: +%u forca (%u jornadas)",
                         (u16)COACH_BOOST_STRENGTH,
                         (u16)g_coach_boost_rounds);
        }
        row++;
        render_text(BG_A, "[A] Novo treinador",  3u, row++, PAL_MAIN);
        render_text(BG_A, "[B] Despedir actual", 3u, row++, PAL_MAIN);
        render_text(BG_A, "[Start] Voltar",      3u, row,   PAL_MAIN);

        render_help_bar("[A]Novo  [B]Despedir  [Start]Voltar", NULL);

        /* ---- Input ---- */
        for (;;) {
            SYS_doVBlankProcess();
            input_update();

            if (input_pressed(BTN_START)) return;

            /* Despedir treinador actual */
            if (input_pressed(BTN_CANCEL)) {
                render_clear_content();
                row = (u16)CONTENT_ROW_FIRST;
                render_textf(BG_A, 2u, row++, PAL_MAIN,
                             "%s foi despedido do %s",
                             g_coaches[team->coach_idx],
                             team->name);
                render_text(BG_A, "Para o seu lugar foi escolhido...",
                            2u, row++, PAL_MAIN);
                render_text(BG_A, "(nenhum -- contrate um novo)",
                            2u, row, PAL_MAIN);
                render_help_bar("[A/B] Continuar", NULL);
                for (;;) {
                    SYS_doVBlankProcess();
                    input_update();
                    if (input_pressed(BTN_CONFIRM) || input_pressed(BTN_CANCEL))
                        break;
                }
                break;  /* regressa ao menu principal de coaches */
            }

            /* Contratar novo treinador */
            if (input_pressed(BTN_CONFIRM)) {
                ListNav nav;
                u8      needs_redraw = 1u;
                u8      old_coach    = team->coach_idx;

                list_nav_init(&nav, (u8)COACH_COUNT, page_sz);

                for (;;) {
                    SYS_doVBlankProcess();
                    input_update();

                    if (input_pressed(BTN_CANCEL)) break;

                    if (list_nav_update(&nav)) needs_redraw = 1u;

                    if (input_pressed(BTN_CONFIRM)) {
                        u8 new_coach = nav.selected;

                        /* Aplica chicotada psicol?gica */
                        render_clear_content();
                        row = (u16)CONTENT_ROW_FIRST;
                        render_textf(BG_A, 2u, row++, PAL_MAIN,
                                     "%s foi despedido do %s",
                                     g_coaches[old_coach], team->name);
                        render_textf(BG_A, 2u, row++, PAL_MAIN,
                                     "Para o seu lugar foi escolhido");
                        render_textf(BG_A, 2u, row++, PAL_MAIN,
                                     "%s", g_coaches[new_coach]);
                        row++;
                        render_text(BG_A,
                                    "CHICOTADA PSICOLOGICA!",
                                    9u, row++, PAL_MAIN);
                        render_textf(BG_A, 2u, row, PAL_MAIN,
                                     "+%u de forca por %u jornadas",
                                     (u16)COACH_BOOST_STRENGTH,
                                     (u16)COACH_BOOST_ROUNDS);

                        team->coach_idx         = new_coach;
                        g_coach_boost_team      = g_player_team_idx;
                        g_coach_boost_rounds    = (u8)COACH_BOOST_ROUNDS;

                        render_help_bar("[A/B] Continuar", NULL);
                        for (;;) {
                            SYS_doVBlankProcess();
                            input_update();
                            if (input_pressed(BTN_CONFIRM) ||
                                input_pressed(BTN_CANCEL)) break;
                        }
                        goto done_hiring;
                    }

                    if (needs_redraw) {
                        needs_redraw = 0u;
                        render_coach_list(&nav);
                    }
                }
                done_hiring:
                break;
            }
        }
    }
}
