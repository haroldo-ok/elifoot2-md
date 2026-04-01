/*
 * screens/main_menu.c -- Menu principal pre-jornada completo
 *
 * Substituicao do stub. Integra todas as telas da Fase 3.
 *
 * Layout (fiel ao original -- menu de opcoes numeradas):
 *
 *   ELIFOOT II  Jornada:XX  DivX  $XXXXXXX   <- status bar (WINDOW)
 *   ==========================================
 *   SAO PAULO  --  Jornada 12 de 15
 *   ==========================================
 *   [A] Jogar Jornada
 *   [B] Plantel / Formacao
 *   [C] Financas
 *   [X] Transferencias
 *   [Y] Palmares
 *   [Start] Gravar / Carregar
 *   ==========================================
 *   Dinheiro: 74500   Ordenados: 12000/mes
 *
 * Botoes:
 *   A      = jogar jornada (retorna imediatamente)
 *   B      = plantel/formacao
 *   C      = financas
 *   X      = transferencias
 *   Y      = palmares
 *   Start  = gravar/carregar (3 slots)
 */

#include <genesis.h>
#include "main_menu.h"
#include "squad.h"
#include "finances.h"
#include "transfers.h"
#include "palmares.h"
#include "coaches.h"
#include "../game/data.h"
#include "../game/types.h"
#include "../engine/input.h"
#include "../engine/render.h"
#include "../engine/sram_io.h"

/* ------------------------------------------------------------------ */
/* Sub-tela de save/load                                               */
/* ------------------------------------------------------------------ */

static void screen_save_load(void) {
    u8  i;
    u16 row;

    for (;;) {
        render_clear_content();
        row = (u16)CONTENT_ROW_FIRST;

        render_text(BG_A, "GRAVAR / CARREGAR JOGO",
                    9u, row++, PAL_MAIN);
        render_hline(BG_A, 1u, row++, 38u, BOX_DOUBLE, PAL_MAIN);
        row++;

        for (i = 0u; i < (u8)SAVE_SLOT_COUNT; i++) {
            u8 valid = sram_slot_valid(i);
            if (valid) {
                u8 season = sram_slot_season(i);
                render_textf(BG_A, 3u, row++, PAL_MAIN,
                             "Slot %u: Temporada %u  (dados validos)",
                             (u16)i, (u16)season);
            } else {
                render_textf(BG_A, 3u, row++, PAL_MAIN,
                             "Slot %u: (vazio)", (u16)i);
            }
        }

        row++;
        render_text(BG_A, "[A] Gravar em slot 0",  3u, row++, PAL_MAIN);
        render_text(BG_A, "[B] Gravar em slot 1",  3u, row++, PAL_MAIN);
        render_text(BG_A, "[C] Gravar em slot 2",  3u, row++, PAL_MAIN);
        row++;
        render_text(BG_A, "[X] Carregar slot 0",   3u, row++, PAL_MAIN);
        render_text(BG_A, "[Y] Carregar slot 1",   3u, row++, PAL_MAIN);
        render_text(BG_A, "[Z] Carregar slot 2",   3u, row++, PAL_MAIN);
        row++;
        render_text(BG_A, "[Start] Voltar",         3u, row, PAL_MAIN);

        render_help_bar("[A/B/C]Gravar  [X/Y/Z]Carregar  [Start]Sair", NULL);

        for (;;) {
            SYS_doVBlankProcess();
            input_update();

            if (input_pressed(BTN_START)) return;

            /* Gravar                                                  */
            if (input_pressed(BTN_CONFIRM)) { sram_save(0u); break; }
            if (input_pressed(BTN_CANCEL))  { sram_save(1u); break; }
            if (input_pressed(BTN_ACTION))  { sram_save(2u); break; }

            /* Carregar                                                */
            if (input_pressed(BTN_FORMATION)) {
                if (sram_slot_valid(0u)) { sram_load(0u); return; }
                break;
            }
            if (input_pressed(BTN_INFO)) {
                if (sram_slot_valid(1u)) { sram_load(1u); return; }
                break;
            }
            if (input_pressed(BTN_SQUAD)) {
                if (sram_slot_valid(2u)) { sram_load(2u); return; }
                break;
            }
        }

        /* Mostra confirmacao                                          */
        render_text(BG_A, "Operacao concluida!                   ",
                    3u, (u16)(CONTENT_ROW_LAST - 1u), PAL_MAIN);
        {
            u8 t;
            for (t = 0u; t < 90u; t++) SYS_doVBlankProcess();
        }
    }
}

/* ------------------------------------------------------------------ */
/* screen_main_menu()                                                  */
/* ------------------------------------------------------------------ */

void screen_main_menu(void) {
    Team  *team = &g_teams[g_player_team_idx];
    u8     total_rounds;
    u8     needs_redraw = 1u;

    /* Numero total de jornadas na divisao do jogador                  */
    total_rounds = (team->division == 0u) ? 15u : 13u;

    for (;;) {
        SYS_doVBlankProcess();
        input_update();

        /* A = Jogar jornada -> retorna ao season_run()               */
        if (input_pressed(BTN_CONFIRM)) return;

        /* B = Plantel / formacao                                      */
        if (input_pressed(BTN_CANCEL)) {
            screen_squad();
            needs_redraw = 1u;
            render_status_bar();
        }

        /* C = Financas                                                */
        if (input_pressed(BTN_ACTION)) {
            screen_finances();
            needs_redraw = 1u;
            render_status_bar();
        }

        /* X = Transferencias                                          */
        if (input_pressed(BTN_FORMATION)) {
            screen_transfers();
            needs_redraw = 1u;
            render_status_bar();
        }

        /* Y = Palmares                                                */
        if (input_pressed(BTN_INFO)) {
            screen_palmares();
            needs_redraw = 1u;
            render_status_bar();
        }

        /* Z = Treinadores                                             */
        if (input_pressed(BTN_SQUAD)) {
            screen_coaches();
            needs_redraw = 1u;
            render_status_bar();
        }

        /* Start = Gravar / carregar                                   */
        if (input_pressed(BTN_START)) {
            screen_save_load();
            needs_redraw = 1u;
            render_status_bar();
        }

        if (!needs_redraw) continue;
        needs_redraw = 0u;

        render_clear_content();
        {
            u16 row = (u16)CONTENT_ROW_FIRST;

            /* Titulo                                                  */
            render_textf(BG_A, 1u, row++, PAL_MAIN,
                         "%-18s  Jornada %u de %u",
                         team->name,
                         (u16)g_round,
                         (u16)total_rounds);
            render_hline(BG_A, 1u, row++, 38u, BOX_DOUBLE, PAL_MAIN);
            row++;

            /* Opcoes do menu                                          */
            render_text(BG_A, "[A]     Jogar Jornada",
                        2u, row++, PAL_MAIN);
            render_text(BG_A, "[B]     Plantel / Formacao",
                        2u, row++, PAL_MAIN);
            render_text(BG_A, "[C]     Financas e Ordenados",
                        2u, row++, PAL_MAIN);
            render_text(BG_A, "[X]     Transferencias",
                        2u, row++, PAL_MAIN);
            render_text(BG_A, "[Y]     Palmares",
                        2u, row++, PAL_MAIN);
            render_text(BG_A, "[Z]     Treinadores",
                        2u, row++, PAL_MAIN);
            render_text(BG_A, "[Start] Gravar / Carregar",
                        2u, row++, PAL_MAIN);
            row++;

            /* Resumo financeiro                                       */
            render_hline(BG_A, 1u, row++, 38u, BOX_SIMPLE, PAL_MAIN);
            render_textf(BG_A, 1u, row++, PAL_MAIN,
                         "Dinheiro : %ld esc.", team->money);
            render_textf(BG_A, 1u, row++, PAL_MAIN,
                         "Ordenados: %ld esc./mes", team->salary_total);
            render_textf(BG_A, 1u, row++, PAL_MAIN,
                         "Resultado: %uV %uE %uD  Pts:%u",
                         (u16)team->wins, (u16)team->draws,
                         (u16)team->losses, (u16)team->points);
        }

        render_help_bar(
            "[A]Jogar [B]Plan. [C]Fin. [X]Trans. [Z]Train.",
            NULL);
    }
}
