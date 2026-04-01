/*
 * game/league.c -- Campeonato: calendariza??o round-robin e simula??o
 *
 * Algoritmo de Berger para round-robin com n equipas:
 *   - Fixa a equipa 0 na posi??o 0 da "roda".
 *   - Nas restantes n-1 posi??es, roda os ?ndices 1..n-1.
 *   - Na jornada j (0-based): emparelha posi??o k com posi??o (n-1-k).
 *   - Alterna casa/fora nas jornadas pares/?mpares para o fixo.
 *
 * Para n?mero ?mpar de equipas (Div2: 13), uma equipa fica de fora
 * em cada jornada (bye) -- ignorada na simula??o.
 */

#include <genesis.h>
#include "league.h"
#include "match.h"
#include "economy.h"
#include "data.h"

/* ------------------------------------------------------------------ */
/* Constantes                                                          */
/* ------------------------------------------------------------------ */

#define MAX_DIV_TEAMS  16u   /* m?ximo de equipas em qualquer divis?o  */

/* ------------------------------------------------------------------ */
/* league_total_rounds()                                               */
/* ------------------------------------------------------------------ */

u8 league_total_rounds(u8 div) {
    return (div == 0u) ? (u8)LEAGUE_ROUNDS_DIV1 : (u8)LEAGUE_ROUNDS_DIV2;
}

/* ------------------------------------------------------------------ */
/* league_build_round()                                                */
/* ------------------------------------------------------------------ */

void league_build_round(u8 div, u8 round_num, LeagueRound *round_out) {
    /*
     * Algoritmo de Berger (round-robin de roda):
     *
     * Seja n = n?mero de equipas (par: n, ?mpar: n+1 com um "dummy").
     * As n equipas s?o colocadas num c?rculo com n/2 pares opostos.
     * A equipa 0 ? fixa; as restantes rodam 1 posi??o por jornada.
     *
     * Para a jornada r (1-based):
     *   wheel[0] = 0 (fixo)
     *   wheel[k] = ((r - 1 + k - 1) % (n - 1)) + 1,  k = 1..n-1
     *
     * Emparelhamentos da jornada r:
     *   Para k = 0 .. n/2 - 1:
     *     par: wheel[k] vs wheel[n-1-k]
     *     Casa alternada: se (r + k) ? par, wheel[k] ? casa.
     */

    u8  div_teams[MAX_DIV_TEAMS + 1u];  /* +1 para o dummy ?mpar       */
    u8  order[MAX_DIV_TEAMS];
    u8  n_real, n, i;
    u8  wheel[MAX_DIV_TEAMS + 1u];
    u8  r;

    round_out->count = 0u;

    /* Recolhe equipas da divis?o                                      */
    n_real = 0u;
    for (i = 0u; i < (u8)TEAM_COUNT; i++) {
        if (g_teams[i].division == div && n_real < MAX_DIV_TEAMS) {
            div_teams[n_real++] = i;
        }
    }
    if (n_real < 2u) return;

    /* Ajusta para par (adiciona dummy = 0xFF se ?mpar)               */
    n = n_real;
    if (n & 1u) {
        div_teams[n] = 0xFFu;  /* dummy = bye                         */
        n++;
    }

    /* Constr?i a roda para a jornada round_num (1-based)             */
    /* wheel[0] = ?ndice global da equipa 0 (fixa)                    */
    /* wheel[k] = ?ndice global da equipa na posi??o k                */
    r = round_num;   /* 1-based                                        */

    wheel[0] = div_teams[0];
    for (i = 1u; i < n; i++) {
        u8 pos = (u8)(((u16)(r - 1u) + (u16)(i - 1u)) % (u16)(n - 1u) + 1u);
        wheel[i] = div_teams[pos];
    }

    /* Emparelhamentos                                                 */
    for (i = 0u; i < n / 2u; i++) {
        u8 a = wheel[i];
        u8 b = wheel[n - 1u - i];

        /* Ignora pares com o dummy                                   */
        if (a == 0xFFu || b == 0xFFu) continue;

        /* Alterna quem ? casa: posi??o par -> a ? casa, ?mpar -> b ? casa */
        if (((u16)r + (u16)i) % 2u == 0u) {
            round_out->home[round_out->count] = a;
            round_out->away[round_out->count] = b;
        } else {
            round_out->home[round_out->count] = b;
            round_out->away[round_out->count] = a;
        }
        round_out->count++;
    }
    (void)order;
}

/* ------------------------------------------------------------------ */
/* league_simulate_round()                                             */
/* ------------------------------------------------------------------ */

void league_simulate_round(const LeagueRound *round) {
    u8   i;
    long revenue;

    /* N?O chama SYS_doVBlankProcess() -- loop puro de c?lculo.        */
    for (i = 0u; i < round->count; i++) {
        u8 home = round->home[i];
        u8 away = round->away[i];

        /* Simula o jogo (atualiza g_teams[] internamente)             */
        match_simulate(home, away);

        /* Receita de bilheteria para a equipa da casa                */
        revenue = economy_ticket_revenue(home);
        g_teams[home].money += revenue;
    }

    /* Sincroniza g_money com a equipa do jogador ap?s a jornada       */
    g_money = g_teams[g_player_team_idx].money;
}
