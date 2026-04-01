/*
 * game/match.c -- Simula??o de partidas do Elifoot II Genesis
 *
 * Algoritmo fiel ao comportamento original do Elifoot II (Turbo Pascal):
 *   1. Calcular for?a m?dia dos 11 titulares de cada equipe.
 *   2. Equipa da casa recebe b?nus de +5 pontos de for?a.
 *   3. N?mero total de oportunidades de golo: 3 ou 4 (aleat?rio).
 *   4. Cada oportunidade ? atribu?da ? equipa mais forte com
 *      probabilidade proporcional ? for?a relativa.
 *   5. N?o todos os eventos s?o gols -- aplicar taxa de convers?o.
 *
 * m68k caveats respeitados:
 *   - int = 16 bits: todas as for?as e probabilidades usam u16.
 *   - Multiplica??es de u16?u16 podem exceder 65535 -- usar u32 quando
 *     necess?rio (ex: home_prob = (str_h * 100) / total).
 *   - N?O chama SYS_doVBlankProcess() -- seguro em loops de simula??o.
 */

#include <genesis.h>
#include "match.h"
#include "data.h"
#include "../engine/rng.h"
#include "../screens/coaches.h"

/* ------------------------------------------------------------------ */
/* match_team_strength()                                               */
/* ------------------------------------------------------------------ */

u16 match_team_strength(u8 team_idx) {
    Team *t = &g_teams[team_idx];
    u16   total = 0u;
    u16   count = 0u;
    u16   i;

    for (i = 0u; i < (u16)t->player_count; i++) {
        Player *pl = &g_players[(u16)t->player_start + i];
        if (pl->on_field) {
            total += (u16)pl->strength;
            count++;
        }
    }
    if (count == 0u) return 1u;
    {
        u16 base = total / count;
        /* Chicotada psicologica: +COACH_BOOST_STRENGTH se boost activo */
        if (g_coach_boost_team == team_idx && g_coach_boost_rounds > 0u) {
            base += (u16)COACH_BOOST_STRENGTH;
        }
        return base;
    }
}

/* ------------------------------------------------------------------ */
/* match_simulate()                                                    */
/* ------------------------------------------------------------------ */

MatchResult match_simulate(u8 home, u8 away) {
    MatchResult r;
    u16  str_h, str_a, total;
    u16  home_prob;         /* probabilidade de golo para casa (0-100) */
    u16  opportunities;     /* n?mero de oportunidades de golo         */
    u16  i;
    u8   hg = 0u, ag = 0u;

    r.home_team = home;
    r.away_team = away;
    r.round     = g_round;
    r.division  = g_teams[home].division;

    str_h = match_team_strength(home) + 5u;  /* b?nus de casa          */
    str_a = match_team_strength(away);
    total = str_h + str_a;
    if (total == 0u) total = 1u;

    /* Probabilidade proporcional ? for?a -- usar u32 para evitar overflow:
     * str_h pode ser at? ~105 (100+5), * 100 = 10500 -- cabe em u16,
     * mas ser expl?cito ? mais seguro no m68k.                        */
    home_prob = (u16)(((u32)str_h * 100UL) / (u32)total);

    /* 3 ou 4 oportunidades de golo por partida (em m?dia ~2.5)        */
    opportunities = 3u + rng_range(2u);

    for (i = 0u; i < opportunities; i++) {
        /*
         * Taxa de convers?o: ~70% das oportunidades s?o gols.
         * Fiel ao original: nem toda oportunidade resulta em golo.
         * Usar rng_range(10) < 7 = 70% de probabilidade.
         */
        if (rng_range(10u) >= 7u) continue;  /* oportunidade desperdi?ada */

        if (rng_range(100u) < home_prob) {
            hg++;
        } else {
            ag++;
        }
    }

    r.home_goals = hg;
    r.away_goals = ag;

    /* ---- Atualiza estat?sticas das equipas ---- */
    g_teams[home].goals_for     += hg;
    g_teams[home].goals_against += ag;
    g_teams[away].goals_for     += ag;
    g_teams[away].goals_against += hg;

    if (hg > ag) {
        g_teams[home].wins++;
        g_teams[away].losses++;
        g_teams[home].points += 3u;
    } else if (hg < ag) {
        g_teams[away].wins++;
        g_teams[home].losses++;
        g_teams[away].points += 3u;
    } else {
        g_teams[home].draws++;
        g_teams[away].draws++;
        g_teams[home].points += 1u;
        g_teams[away].points += 1u;
    }

    /* Guarda resultado no array global (sem overflow -- max 200)       */
    if (g_results_count < 200u) {
        g_results[g_results_count] = r;
        g_results_count++;
    }

    return r;
}
