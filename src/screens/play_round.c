/*
 * screens/play_round.c -- Simula e exibe uma jornada
 */

#include <genesis.h>
#include "play_round.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"
#include "../game/league.h"
#include "../game/economy.h"
#include "../engine/rng.h"
#include "../game/transfer.h"
#include "../screens/coaches.h"
#include "../game/types.h"

static void wait_confirm(void) {
    ui_puts(0u, 27u, UI_PAL_NORMAL, "A/B: continuar");
    for (;;) {
        ui_wait_vblank();
        input_update();
        if (input_pressed(BTN_CONFIRM) || input_pressed(BTN_CANCEL)
                || input_pressed(BTN_START)) return;
    }
}

static void show_round_results(const LeagueRound *rnd, u8 div) {
    u8 i;
    ui_clear();
    ui_printf(0u, 0u, UI_PAL_NORMAL, "Jornada %u -- Div%u   Resultados:",
              (u16)g_round, (u16)(div + 1u));
    ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);

    for (i = 0u; i < rnd->count && i < 8u; i++) {
        u8  h = rnd->home[i];
        u8  a = rnd->away[i];
        u16 row = (u16)(3u + i * 2u);
        /* Find result in g_results */
        u8  r;
        u8  hg = 0u, ag = 0u;
        u16 pal = UI_PAL_NORMAL;

        for (r = 0u; r < g_results_count; r++) {
            MatchResult *mr = &g_results[r];
            if (mr->round == g_round && mr->home_team == h && mr->away_team == a) {
                hg = mr->home_goals;
                ag = mr->away_goals;
                break;
            }
        }

        if (h == g_player_team_idx || a == g_player_team_idx) {
            pal = UI_PAL_SELECT;
            ui_fill_row(row, UI_PAL_SELECT);
        }

        ui_printf(0u, row, pal, "%-13s %u - %-u  %s",
                  g_teams[h].name, (u16)hg, (u16)ag, g_teams[a].name);
    }

    ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
    wait_confirm();
}

static void show_standings_brief(u8 div) {
    u8  order[29];
    u8  count, i;
    u8  top = (count > 10u) ? 10u : count;

    data_sort_standings(div, order, &count);
    top = (count > 10u) ? 10u : count;

    ui_clear();
    ui_printf(0u, 0u, UI_PAL_NORMAL, "Classificacao -- Div%u  Jornada %u",
              (u16)(div + 1u), (u16)g_round);
    ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);
    ui_puts( 0u, 2u, UI_PAL_NORMAL, "#  Equipa             J  V  E  D  Pts");
    ui_hline(0u, 3u, UI_COLS, UI_PAL_NORMAL);

    for (i = 0u; i < top; i++) {
        u8     t   = order[i];
        Team  *tm  = &g_teams[t];
        u16    row = (u16)(4u + i);
        u8     j   = (u8)(tm->wins + tm->draws + tm->losses);
        u16    pts = TEAM_POINTS(tm->wins, tm->draws);
        u16    pal = (t == g_player_team_idx) ? UI_PAL_SELECT : UI_PAL_NORMAL;

        if (t == g_player_team_idx) ui_fill_row(row, UI_PAL_SELECT);
        ui_printf(0u, row, pal, "%2u %-18s %2u %2u %2u %2u  %3u",
                  (u16)(i + 1u), tm->name, (u16)j,
                  (u16)tm->wins, (u16)tm->draws, (u16)tm->losses, pts);
    }

    ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
    wait_confirm();
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
/* Posicoes (usado localmente)                                         */
/* ------------------------------------------------------------------ */

static const char s_pos_pr[4][3] = { "GR", "DF", "MD", "AV" };

/* ------------------------------------------------------------------ */
/* salary_request() -- pede aumento a 1 jogador aleatorio por jornada */
/*                                                                     */
/* Fiel ao original:                                                   */
/*   "[Nome], [pos], [forca] de forca, pede aumento para [val]."      */
/*   Forcado a aceitar se plantel <= 14 ou so tem 1 GR.               */
/*   Se recusado: 50% vai a leilao, 50% "Entao adeus" (sai do clube). */
/* ------------------------------------------------------------------ */

static void salary_request(void) {
    Team   *team = &g_teams[g_player_team_idx];
    u8      idx, i;
    Player *pl;
    long    new_sal;
    u8      total_gr = 0u;
    u8      total_pl = team->player_count;
    u8      forced;
    u16     pidx;

    /* 30% chance por jornada */
    if (rng_range(10u) >= 3u) return;
    if (total_pl == 0u) return;

    /* Escolher jogador aleatorio */
    idx  = (u8)(rng_range((u16)total_pl));
    pidx = (u16)(team->player_start + idx);
    pl   = &g_players[pidx];

    /* Aumento pedido: forca/4 a forca/2 escudos por mes (aprox 15-30%) */
    new_sal = pl->salary
            + (long)(pl->strength) * 50L
            + (long)rng_range(300u);
    if (new_sal < pl->salary + 500L) new_sal = pl->salary + 500L;

    /* Contar GRs no plantel */
    for (i = 0u; i < total_pl; i++) {
        if (g_players[(u16)(team->player_start + i)].pos == 0u)
            total_gr++;
    }

    /* Forcado a aceitar se: so tem 1 GR e este o pede, ou plantel <= 14 */
    forced = ((pl->pos == 0u && total_gr <= 1u) || total_pl <= 14u);

    /* Mostrar pedido */
    ui_clear();
    ui_puts(11u, 0u, UI_PAL_NORMAL, "PEDIDO DE ORDENADO");
    ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);

    ui_printf(2u, 4u, UI_PAL_NORMAL, "%s,", pl->name);
    ui_printf(2u, 5u, UI_PAL_NORMAL, "%s, %u de forca,",
              s_pos_pr[pl->pos < 4u ? pl->pos : 0u],
              (u16)pl->strength);
    ui_puts(2u, 6u, UI_PAL_NORMAL, "pede que lhe seja aumentado");
    ui_printf(2u, 7u, UI_PAL_NORMAL, "o ordenado para %ld Esc.", new_sal);
    ui_hline(0u, 9u, UI_COLS, UI_PAL_NORMAL);
    ui_printf(2u, 10u, UI_PAL_NORMAL, "Ordenado actual: %ld Esc.", pl->salary);

    if (forced) {
        /* Mensagem de aceitacao forcada */
        if (pl->pos == 0u && total_gr <= 1u) {
            ui_puts(2u, 13u, UI_PAL_NORMAL,
                    "Como so possui um guarda-redes");
            ui_puts(2u, 14u, UI_PAL_NORMAL,
                    "no plantel e obrigado a aceitar.");
        } else {
            ui_puts(2u, 13u, UI_PAL_NORMAL,
                    "Como so tem 14 jogadores");
            ui_puts(2u, 14u, UI_PAL_NORMAL,
                    "no plantel e obrigado a aceitar.");
        }
        pl->salary = new_sal;
        ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
        ui_puts(0u, 27u, UI_PAL_NORMAL, "A/B: continuar");
        for (;;) {
            ui_wait_vblank(); input_update();
            if (input_pressed(BTN_CONFIRM) || input_pressed(BTN_CANCEL)) break;
        }
        return;
    }

    /* Escolha: aceitar ou recusar */
    ui_puts(2u, 13u, UI_PAL_NORMAL, "Aceita o aumento?");
    ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
    ui_puts(0u, 27u, UI_PAL_NORMAL, "A: Sim   B: Nao");

    for (;;) {
        ui_wait_vblank();
        input_update();

        if (input_pressed(BTN_CONFIRM)) {
            /* Aceite */
            pl->salary = new_sal;
            ui_puts(2u, 16u, UI_PAL_NORMAL, "Aumento aceite.");
            ui_printf(2u, 17u, UI_PAL_NORMAL,
                      "Novo ordenado: %ld Esc.", new_sal);
            ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
            ui_puts(0u, 27u, UI_PAL_NORMAL, "A/B: continuar");
            for (;;) {
                ui_wait_vblank(); input_update();
                if (input_pressed(BTN_CONFIRM) || input_pressed(BTN_CANCEL)) break;
            }
            return;
        }

        if (input_pressed(BTN_CANCEL)) {
            /* Recusado */
            if (rng_range(2u) == 1u) {
                /* Vai a leilao */
                u8 winner;
                ui_printf(2u, 16u, UI_PAL_NORMAL,
                          "Caso nao aceite, %s", pl->name);
                ui_printf(2u, 17u, UI_PAL_NORMAL,
                          "pode decidir por-se em leilao");
                ui_printf(2u, 18u, UI_PAL_NORMAL,
                          "pelo preco de %ld Esc.", new_sal);
                ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
                ui_puts(0u, 27u, UI_PAL_NORMAL, "A/B: continuar");
                for (;;) {
                    ui_wait_vblank(); input_update();
                    if (input_pressed(BTN_CONFIRM) || input_pressed(BTN_CANCEL)) break;
                }
                winner = transfer_auction(pidx, g_player_team_idx);
                if (winner != 0xFFu) {
                    ui_clear();
                    ui_printf(4u, 12u, UI_PAL_NORMAL,
                              "%s transferido para %s.",
                              pl->name, g_teams[winner].name);
                } else {
                    ui_clear();
                    ui_puts(4u, 12u, UI_PAL_NORMAL,
                            "Nao houve ofertas.");
                    ui_puts(4u, 13u, UI_PAL_NORMAL, "Entao adeus.");
                    /* Player leaves: mark as not on field, salary = 0 */
                    pl->on_field = 0u;
                    pl->salary   = 0L;
                }
            } else {
                /* Aceita sem leilao mas "Entao adeus" -- sai mesmo assim */
                ui_puts(2u, 16u, UI_PAL_NORMAL, "Entao adeus.");
                pl->on_field = 0u;
                pl->salary   = 0L;
            }
            ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
            ui_puts(0u, 27u, UI_PAL_NORMAL, "A/B: continuar");
            for (;;) {
                ui_wait_vblank(); input_update();
                if (input_pressed(BTN_CONFIRM) || input_pressed(BTN_CANCEL)) break;
            }
            return;
        }
    }
}


void screen_play_round(void) {
    LeagueRound rnd;
    u8 total_rounds = league_total_rounds(g_division);

    if (g_round > total_rounds) {
        /* ---- FIM DE TEMPORADA ---- */
        u8  order[29];
        u8  count, i;
        u8  player_pos = 0xFFu;

        data_sort_standings(g_division, order, &count);

        /* Find player position */
        for (i = 0u; i < count; i++) {
            if (order[i] == g_player_team_idx) { player_pos = i; break; }
        }

        ui_clear();
        ui_printf(8u, 0u, UI_PAL_NORMAL, "FIM DE TEMPORADA -- Div%u",
                  (u16)(g_division + 1u));
        ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);
        ui_puts(0u, 2u, UI_PAL_NORMAL, "#  Equipa             Pts");
        ui_hline(0u, 3u, UI_COLS, UI_PAL_NORMAL);

        for (i = 0u; i < count && i < 10u; i++) {
            u8    t   = order[i];
            Team *tm  = &g_teams[t];
            u16   pts = TEAM_POINTS(tm->wins, tm->draws);
            u16   pal = (t == g_player_team_idx) ? UI_PAL_SELECT : UI_PAL_NORMAL;
            u16   row = (u16)(4u + i);
            if (t == g_player_team_idx) ui_fill_row(row, UI_PAL_SELECT);
            ui_printf(0u, row, pal, "%2u %-18s %3u",
                      (u16)(i + 1u), tm->name, pts);
        }

        /* Promotion/relegation message */
        if (player_pos != 0xFFu) {
            u16 msg_row = 16u;
            if (g_division > 0u && player_pos <= 1u) {
                ui_puts(4u, msg_row, UI_PAL_NORMAL, "SUBIDA DE DIVISAO!");
                g_division--;
                g_teams[g_player_team_idx].division = g_division;
            } else if (g_division < 3u && player_pos >= (u8)(count - 2u)) {
                ui_puts(4u, msg_row, UI_PAL_NORMAL, "DESCIDA DE DIVISAO!");
                g_division++;
                g_teams[g_player_team_idx].division = g_division;
            } else {
                ui_printf(4u, msg_row, UI_PAL_NORMAL,
                          "Posicao final: %u de %u", (u16)(player_pos + 1u), (u16)count);
            }
        }

        /* Pay end-of-season prizes */
        economy_pay_prizes();

        wait_confirm();

        /* Record in palmares */
        {
            u8 slot = (u8)(g_season_num % (u8)PALMARES_COUNT);
            SeasonRecord *sr = &g_palmares[slot];
            sr->season_num  = g_season_num;
            sr->player_div  = g_division;
            sr->player_pos  = player_pos;
            /* Find Div1 champion */
            {
                u8 d1order[29]; u8 d1count;
                data_sort_standings(0u, d1order, &d1count);
                sr->div1_champion = d1count > 0u ? d1order[0] : 0xFFu;
            }
            sr->cup_champion = 0xFFu; /* updated by cup logic */
        }

        /* Reset for new season */
        {
            u8 t;
            for (t = 0u; t < (u8)TEAM_COUNT; t++) {
                g_teams[t].wins        = 0u;
                g_teams[t].draws       = 0u;
                g_teams[t].losses      = 0u;
                g_teams[t].goals_for   = 0u;
                g_teams[t].goals_against = 0u;
                g_teams[t].points      = 0u;
            }
        }
        g_results_count = 0u;
        g_round = 1u;
        /* Reset player goals for new season */
        {
            u16 pi;
            u16 total_pl = (u16)((u16)TEAM_COUNT * (u16)PLAYERS_PER_TEAM);
            for (pi = 0u; pi < total_pl; pi++)
                g_goals[pi] = 0u;
        }
        g_season_num++;
        return;
    }

    /* Show "simulating" message */
    ui_clear();
    ui_printf(8u, 12u, UI_PAL_NORMAL, "Simulando jornada %u...", (u16)g_round);
    ui_wait_vblank();
    ui_wait_vblank();

    /* Salary request before round */
    salary_request();

    /* Tick coach boost before simulation */
    coaches_tick_boost();

    /* Build and simulate */
    league_build_round(g_division, g_round, &rnd);
    league_simulate_round(&rnd);

    /* Pay salaries every 4 rounds */
    if ((g_round % 4u) == 0u) {
        economy_pay_monthly_salaries();
    }

    /* Show results then standings */
    show_round_results(&rnd, g_division);
    show_standings_brief(g_division);

    g_round++;
}
