/*
 * game/season.c -- Coordena??o de temporada completa
 *
 * season_run() orquestra:
 *   1. Reset de estat?sticas
 *   2. Por cada jornada:
 *      a. Menu pr?-jornada
 *      b. Simula??o de todos os jogos da jornada
 *      c. Pagamento de sal?rios (cada 4 jornadas)
 *      d. Exibi??o de resultados
 *   3. Fim: pr?mios, promo??o/rebaixamento
 *
 * SYS_doVBlankProcess() APENAS nos loops de input -- nunca na simula??o.
 * int = 16 bits: contadores de jornada usam u8.
 */

#include <genesis.h>
#include "season.h"
#include "data.h"
#include "league.h"
#include "economy.h"
#include "transfer.h"
#include "types.h"
#include "../engine/input.h"
#include "../engine/render.h"
#include "../engine/sram_io.h"
#include "../screens/main_menu.h"
#include "cup.h"
#include "../screens/coaches.h"

/* ------------------------------------------------------------------ */
/* Telas auxiliares (implementa??o inline, Fase 3 separa em screens/) */
/* ------------------------------------------------------------------ */

static void show_standings(u8 div) {
    u8  order[TEAM_COUNT];
    u8  count, i;
    u16 row = (u16)CONTENT_ROW_FIRST;

    data_sort_standings(div, order, &count);
    render_clear_content();
    render_textf(BG_A, 1u, row++, PAL_MAIN,
                 "Classificacao Div%u  Jornada %u",
                 (u16)(div+1u), (u16)g_round);
    render_hline(BG_A, 1u, row++, 38u, BOX_SIMPLE, PAL_MAIN);
    render_text(BG_A, " Pos Equipa              V  E  D  Pts",
                1u, row++, PAL_MAIN);
    render_hline(BG_A, 1u, row++, 38u, BOX_SIMPLE, PAL_MAIN);

    for (i = 0u; i < count && row <= (u16)CONTENT_ROW_LAST; i++) {
        u8    t  = order[i];
        Team *tm = &g_teams[t];
        u16  pal = (t == g_player_team_idx) ? PAL_SELECTED : PAL_MAIN;
        if (pal == PAL_SELECTED)
            render_fill_rect(BG_A, 1u, row, 38u, 1u, PAL_SELECTED, 0u);
        render_textf(BG_A, 1u, row, pal,
                     "%3u. %-16s %2u %2u %2u %3u",
                     (u16)(i+1u), tm->name,
                     (u16)tm->wins, (u16)tm->draws, (u16)tm->losses,
                     (u16)tm->points);
        row++;
    }

    render_help_bar("[A/B] Continuar", NULL);
    for (;;) {
        SYS_doVBlankProcess();
        input_update();
        if (input_pressed(BTN_CONFIRM) || input_pressed(BTN_CANCEL)) return;
    }
}

static void show_results(u8 div, u8 round_num) {
    u8   i, shown = 0u;
    u16  row = (u16)CONTENT_ROW_FIRST;

    render_clear_content();
    render_textf(BG_A, 1u, row++, PAL_MAIN,
                 "Resultados Jornada %u  Div%u",
                 (u16)round_num, (u16)(div+1u));
    render_hline(BG_A, 1u, row++, 38u, BOX_SIMPLE, PAL_MAIN);

    for (i = 0u; i < g_results_count && row <= (u16)CONTENT_ROW_LAST - 1u; i++) {
        MatchResult *r = &g_results[i];
        if (r->division != div) continue;
        {
            u16 pal = ((r->home_team == g_player_team_idx ||
                        r->away_team == g_player_team_idx)) ?
                      PAL_SELECTED : PAL_MAIN;
            if (pal == PAL_SELECTED)
                render_fill_rect(BG_A, 1u, row, 38u, 1u, PAL_SELECTED, 0u);
            render_textf(BG_A, 1u, row, pal,
                         "%-14s  %u - %u  %-14s",
                         g_teams[r->home_team].name, (u16)r->home_goals,
                         (u16)r->away_goals, g_teams[r->away_team].name);
        }
        row++;
        shown++;
    }
    if (shown == 0u) {
        render_text(BG_A, "(sem jogos nesta divisao)", 2u, row, PAL_MAIN);
    }

    render_help_bar("[A] Classificacao  [B] Continuar", NULL);
    for (;;) {
        SYS_doVBlankProcess();
        input_update();
        if (input_pressed(BTN_CONFIRM)) { show_standings(div); return; }
        if (input_pressed(BTN_CANCEL))  return;
    }
}

static void pre_round_menu(void) {
    render_clear_content();
    render_textf(BG_A, 2u, (u16)CONTENT_ROW_FIRST, PAL_MAIN,
                 "Temporada %u  Jornada %u",
                 (u16)g_season_num, (u16)g_round);
    render_textf(BG_A, 2u, (u16)(CONTENT_ROW_FIRST+1u), PAL_MAIN,
                 "Equipa: %-16s", g_teams[g_player_team_idx].name);
    render_textf(BG_A, 2u, (u16)(CONTENT_ROW_FIRST+2u), PAL_MAIN,
                 "Dinheiro: %ld esc.", g_money);
    render_hline(BG_A, 1u, (u16)(CONTENT_ROW_FIRST+4u), 38u, BOX_SIMPLE, PAL_MAIN);
    render_text(BG_A, "[A] Jogar Jornada",  2u, (u16)(CONTENT_ROW_FIRST+5u), PAL_MAIN);
    render_text(BG_A, "[B] Classificacao",  2u, (u16)(CONTENT_ROW_FIRST+6u), PAL_MAIN);
    render_text(BG_A, "[Start] Gravar",     2u, (u16)(CONTENT_ROW_FIRST+7u), PAL_MAIN);
    render_help_bar("[A]Jogar  [B]Class.  [Start]Gravar", NULL);

    for (;;) {
        SYS_doVBlankProcess();
        input_update();

        if (input_pressed(BTN_CONFIRM)) return;

        if (input_pressed(BTN_CANCEL)) { show_standings(g_division); return; }

        if (input_pressed(BTN_START)) {
            render_fill_rect(BG_A, 2u, (u16)(CONTENT_ROW_FIRST+9u),
                             36u, 1u, PAL_SELECTED, 0u);
            render_text(BG_A, "Gravar em slot: [A]=0  [B]=1  [C]=2",
                        2u, (u16)(CONTENT_ROW_FIRST+9u), PAL_SELECTED);
            for (;;) {
                SYS_doVBlankProcess();
                input_update();
                if (input_pressed(BTN_CONFIRM)) { sram_save(0u); break; }
                if (input_pressed(BTN_CANCEL))  { sram_save(1u); break; }
                if (input_pressed(BTN_ACTION))  { sram_save(2u); break; }
            }
            render_text(BG_A, "Gravado!                              ",
                        2u, (u16)(CONTENT_ROW_FIRST+9u), PAL_MAIN);
        }
    }
}

/* ------------------------------------------------------------------ */
/* season_run()                                                        */
/* ------------------------------------------------------------------ */

void season_run(void) {
    u8 round, total_rounds, div, d;
    LeagueRound lr;

    data_reset_season_stats();

    div          = g_teams[g_player_team_idx].division;
    total_rounds = league_total_rounds(div);

    for (round = 1u; round <= total_rounds; round++) {
        g_round = round;
        render_status_bar();

        /* Menu pr?-jornada                                           */
        pre_round_menu();

        /* Limpa buffer de resultados para esta jornada               */
        g_results_count = 0u;

        /* Simula todas as divis?es (sem vsync)                       */
        for (d = 0u; d < 2u; d++) {
            u8 cur_div    = (d == 0u) ? div : (u8)(1u - div);
            u8 cur_rounds = league_total_rounds(cur_div);
            u8 cur_round  = (round <= cur_rounds) ? round : cur_rounds;
            league_build_round(cur_div, cur_round, &lr);
            league_simulate_round(&lr);
        }

        /* Tick do boost de treinador                                 */
        coaches_tick_boost();

        /* Copa: intercalada com o campeonato.
         * Jornada 1: sorteio
         * Jornadas impares >= 3: 1a mao de uma fase
         * Jornadas pares  >= 4: 2a mao + avanco de fase              */
        if (round == 1u) {
            cup_draw();
        } else if (round >= 3u && (round & 1u) == 1u) {
            cup_simulate_leg(0u);
        } else if (round >= 4u && (round & 1u) == 0u) {
            cup_simulate_leg(1u);
            {
                u8 champion = cup_advance();
                if (champion != 0xFFu) {
                    /* Guarda campeao da taca no palmares             */
                    g_palmares[0].cup_champion = champion;
                }
            }
        }

        /* Sal?rios a cada 4 jornadas                                 */
        if ((round % 4u) == 0u) {
            economy_pay_monthly_salaries();
        }

        /* Exibe resultados                                           */
        show_results(div, round);
    }

    /* Sal?rio final se necess?rio                                    */
    if ((total_rounds % 4u) != 0u) {
        economy_pay_monthly_salaries();
    }
}

/* ------------------------------------------------------------------ */
/* screen_season_end()                                                 */
/* ------------------------------------------------------------------ */

void screen_season_end(void) {
    u8  order[TEAM_COUNT], d1order[TEAM_COUNT];
    u8  count, d1count;
    u8  player_pos = 0u, i;
    u16 row;

    data_sort_standings(g_division, order, &count);
    for (i = 0u; i < count; i++) {
        if (order[i] == g_player_team_idx) { player_pos = (u8)(i+1u); break; }
    }

    economy_pay_prizes();
    g_money = g_teams[g_player_team_idx].money;

    /* Palmares                                                       */
    for (i = (u8)(PALMARES_COUNT-1u); i > 0u; i--)
        g_palmares[i] = g_palmares[i-1u];
    data_sort_standings(0u, d1order, &d1count);
    g_palmares[0].season_num    = g_season_num;
    g_palmares[0].div1_champion = (d1count > 0u) ? d1order[0] : 0u;
    g_palmares[0].cup_champion  = 0u;
    g_palmares[0].player_div    = g_division;
    g_palmares[0].player_pos    = player_pos;

    /* Promo??o / rebaixamento                                        */
    {
        u8 old_div = g_division;
        data_assign_divisions();
        g_division = g_teams[g_player_team_idx].division;

        render_clear_content();
        row = (u16)CONTENT_ROW_FIRST;
        render_textf(BG_A, 2u, row++, PAL_MAIN,
                     "=== FIM DA TEMPORADA %u ===", (u16)g_season_num);
        row++;
        render_textf(BG_A, 2u, row++, PAL_MAIN,
                     "Equipa : %-16s", g_teams[g_player_team_idx].name);
        render_textf(BG_A, 2u, row++, PAL_MAIN,
                     "Posicao: %u de %u", (u16)player_pos, (u16)count);
        render_textf(BG_A, 2u, row++, PAL_MAIN,
                     "Dinheiro: %ld esc.", g_money);
        row++;

        if (g_division < old_div)
            render_text(BG_A, "** PROMOVIDO! **", 2u, row++, PAL_MAIN);
        else if (g_division > old_div)
            render_text(BG_A, "** REBAIXADO **",  2u, row++, PAL_MAIN);
        else
            render_text(BG_A, "Permanece na mesma divisao.", 2u, row++, PAL_MAIN);

        row++;
        if (d1count > 0u)
            render_textf(BG_A, 2u, row++, PAL_MAIN,
                         "Campeao Div1: %s", g_teams[d1order[0]].name);
    }

    render_help_bar("[A] Nova Temporada  [Start] Gravar", NULL);

    for (;;) {
        SYS_doVBlankProcess();
        input_update();
        if (input_pressed(BTN_START)) {
            sram_save(0u);
            render_text(BG_A, "Gravado!", 2u, (u16)CONTENT_ROW_LAST, PAL_MAIN);
        }
        if (input_pressed(BTN_CONFIRM)) break;
    }

    g_season_num++;
    g_round    = 1u;
    g_division = g_teams[g_player_team_idx].division;
}
