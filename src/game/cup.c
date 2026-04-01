/*
 * game/cup.c — Copa com eliminatórias em 2 mãos
 *
 * m68k / SGDK 1.70 caveats:
 *   - int = 16 bits: índices e contadores usam u8/u16.
 *   - NÃO chama SYS_doVBlankProcess() nos loops de simulação.
 *   - Penáltis simulados com rng_range() — sem vsync.
 *
 * Algoritmo de sorteio (potes):
 *   Pote A: equipas com n1 par  (0, 2, 4, 6, ...)
 *   Pote B: equipas com n1 ímpar (1, 3, 5, ...)
 *   Cada par: 1 equipa do pote A vs 1 equipa do pote B.
 *   Se desequilibrado, completa com restantes do pote maior.
 *   Equipas sorteadas aleatoriamente dentro de cada pote.
 */

#include <genesis.h>
#include "cup.h"
#include "data.h"
#include "match.h"
#include "../engine/rng.h"

/* ------------------------------------------------------------------ */
/* Nomes de fase (strings literais do original)                       */
/* ------------------------------------------------------------------ */

static const char * const s_phase_names[] = {
    "1a ELIMINATORIA",
    "2a ELIMINATORIA",
    "QUARTOS DE FINAL",
    "MEIAS FINAIS",
    "FINAL DA TACA",
};

const char *cup_phase_name(u8 phase) {
    if (phase > 4u) return "COPA";
    return s_phase_names[phase];
}

/* ------------------------------------------------------------------ */
/* Utilitários internos                                                */
/* ------------------------------------------------------------------ */

/* Shuffle de um array de u8 de comprimento n (Fisher-Yates) */
static void shuffle_u8(u8 *arr, u8 n) {
    u8 i, j, tmp;
    if (n < 2u) return;
    for (i = (u8)(n - 1u); i > 0u; i--) {
        j = (u8)(rng_range((u16)(i + 1u)));
        tmp    = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

/* Simula série de penáltis entre team_a e team_b.
 * Retorna índice do vencedor (team_a ou team_b). */
static u8 simulate_penalties(u8 team_a, u8 team_b) {
    u8  score_a = 0u, score_b = 0u;
    u8  k;
    u16 str_a = match_team_strength(team_a);
    u16 str_b = match_team_strength(team_b);
    u16 total = str_a + str_b;
    if (total == 0u) total = 1u;
    u16 prob_a = (u16)(((u32)str_a * 100UL) / (u32)total);

    /* 5 penáltis cada; converte ~75% */
    for (k = 0u; k < 5u; k++) {
        if (rng_range(100u) < 75u && rng_range(100u) < prob_a)   score_a++;
        if (rng_range(100u) < 75u && rng_range(100u) < (u16)(100u - prob_a)) score_b++;
    }
    /* Morte súbita se empatados após 5 */
    while (score_a == score_b) {
        if (rng_range(100u) < 75u && rng_range(100u) < prob_a)   score_a++;
        if (rng_range(100u) < 75u && rng_range(100u) < (u16)(100u - prob_a)) score_b++;
    }
    return (score_a > score_b) ? team_a : team_b;
}

/* ------------------------------------------------------------------ */
/* cup_draw()                                                          */
/* ------------------------------------------------------------------ */

u8 cup_draw(void) {
    /*
     * Selecciona as 16 participantes:
     *   Primeiro: equipas da Div1 (até 16)
     *   Completar com equipas da Div2 com n1 > 0 (se Div1 < 16)
     *
     * Separa em dois potes pelo valor de n1 (par / ímpar).
     * Faz o sorteio e cria os pares em g_cup_ties[].
     */
    u8 participants[16];
    u8 np = 0u;
    u8 pote_a[8], pote_b[8];
    u8 na = 0u, nb = 0u;
    u8 t, i, pairs;

    /* Recolhe participantes */
    for (t = 0u; t < (u8)TEAM_COUNT && np < 16u; t++) {
        if (g_teams[t].division == 0u) {
            participants[np++] = t;
            g_teams[t].cup_active = 1u;
        }
    }
    for (t = 0u; t < (u8)TEAM_COUNT && np < 16u; t++) {
        if (g_teams[t].division != 0u && g_teams[t].n1 > 0u) {
            participants[np++] = t;
            g_teams[t].cup_active = 1u;
        }
    }
    /* Completa com quaisquer equipas da Div2 se ainda falta */
    for (t = 0u; t < (u8)TEAM_COUNT && np < 16u; t++) {
        if (g_teams[t].division != 0u && g_teams[t].cup_active != 1u) {
            participants[np++] = t;
            g_teams[t].cup_active = 1u;
        }
    }

    /* Marca equipas não participantes como eliminadas */
    for (t = 0u; t < (u8)TEAM_COUNT; t++) {
        u8 found = 0u;
        for (i = 0u; i < np; i++) {
            if (participants[i] == t) { found = 1u; break; }
        }
        if (!found) g_teams[t].cup_active = 0u;
    }

    /* Separa em potes: n1 par → A, ímpar → B */
    for (i = 0u; i < np; i++) {
        u8 team = participants[i];
        if ((g_teams[team].n1 & 1u) == 0u) {
            if (na < 8u) pote_a[na++] = team;
        } else {
            if (nb < 8u) pote_b[nb++] = team;
        }
    }
    /* Equaliza potes se desequilibrados */
    while (na > nb && nb < 8u && na > 0u) { pote_b[nb++] = pote_a[--na]; }
    while (nb > na && na < 8u && nb > 0u) { pote_a[na++] = pote_b[--nb]; }

    /* Shuffle de cada pote */
    shuffle_u8(pote_a, na);
    shuffle_u8(pote_b, nb);

    /* Cria confrontos */
    pairs = (na < nb) ? na : nb;
    g_cup_ties_count = 0u;
    for (i = 0u; i < pairs && i < 8u; i++) {
        g_cup_ties[i].team_a       = pote_a[i];
        g_cup_ties[i].team_b       = pote_b[i];
        g_cup_ties[i].goals_a_leg1 = 0u;
        g_cup_ties[i].goals_b_leg1 = 0u;
        g_cup_ties[i].goals_a_leg2 = 0u;
        g_cup_ties[i].goals_b_leg2 = 0u;
        g_cup_ties[i].phase        = g_cup_phase;
        g_cup_ties_count++;
    }
    return g_cup_ties_count;
}

/* ------------------------------------------------------------------ */
/* cup_simulate_leg()                                                  */
/* ------------------------------------------------------------------ */

void cup_simulate_leg(u8 leg) {
    u8 i;
    /* NÃO chama SYS_doVBlankProcess() */
    for (i = 0u; i < g_cup_ties_count; i++) {
        CupTie    *tie  = &g_cup_ties[i];
        MatchResult res;

        if (leg == 0u) {
            /* 1ª mão: team_a em casa */
            res = match_simulate(tie->team_a, tie->team_b);
            tie->goals_a_leg1 = res.home_goals;
            tie->goals_b_leg1 = res.away_goals;
        } else {
            /* 2ª mão: team_b em casa (invertido) */
            res = match_simulate(tie->team_b, tie->team_a);
            tie->goals_b_leg2 = res.home_goals;
            tie->goals_a_leg2 = res.away_goals;
        }
    }
}

/* ------------------------------------------------------------------ */
/* cup_advance()                                                       */
/* ------------------------------------------------------------------ */

u8 cup_advance(void) {
    /*
     * Determina vencedores de cada confronto pelo agregado.
     * Em empate: quem marcou mais golos fora na 2ª mão avança.
     * Se ainda empate: penáltis.
     *
     * Cria confrontos da fase seguinte em g_cup_ties[].
     * Se era a Final (fase 4): retorna o campeão.
     */
    u8  winners[8];
    u8  nw = 0u;
    u8  i;
    u8  cup_winner = 0xFFu;

    for (i = 0u; i < g_cup_ties_count; i++) {
        CupTie *tie = &g_cup_ties[i];
        u8  agg_a = (u8)(tie->goals_a_leg1 + tie->goals_a_leg2);
        u8  agg_b = (u8)(tie->goals_b_leg1 + tie->goals_b_leg2);
        u8  winner;

        if (agg_a > agg_b) {
            winner = tie->team_a;
        } else if (agg_b > agg_a) {
            winner = tie->team_b;
        } else {
            /* Regra de golos fora: team_b jogou fora na 1ª mão */
            /* Golos de team_b fora (leg1) vs team_a fora (leg2) */
            if (tie->goals_b_leg1 > tie->goals_a_leg2) {
                winner = tie->team_b;
            } else if (tie->goals_a_leg2 > tie->goals_b_leg1) {
                winner = tie->team_a;
            } else {
                winner = simulate_penalties(tie->team_a, tie->team_b);
            }
        }
        winners[nw++] = winner;

        /* Marca perdedor como eliminado */
        u8 loser = (winner == tie->team_a) ? tie->team_b : tie->team_a;
        g_teams[loser].cup_active = 0u;
    }

    /* Final: apenas 1 confronto → retorna campeão */
    if (g_cup_phase >= 4u) {
        if (nw > 0u) {
            cup_winner = winners[0];
            g_cup_ties_count = 0u;
        }
        return cup_winner;
    }

    /* Avança para a próxima fase */
    g_cup_phase++;
    g_cup_ties_count = 0u;

    /* Shuffle dos vencedores para criar novos confrontos aleatórios */
    shuffle_u8(winners, nw);

    for (i = 0u; i + 1u < nw; i += 2u) {
        CupTie *tie            = &g_cup_ties[g_cup_ties_count];
        tie->team_a            = winners[i];
        tie->team_b            = winners[i + 1u];
        tie->goals_a_leg1      = 0u;
        tie->goals_b_leg1      = 0u;
        tie->goals_a_leg2      = 0u;
        tie->goals_b_leg2      = 0u;
        tie->phase             = g_cup_phase;
        g_cup_ties_count++;
    }

    return 0xFFu;   /* copa ainda em curso */
}

/* ------------------------------------------------------------------ */
/* cup_team_active()                                                   */
/* ------------------------------------------------------------------ */

u8 cup_team_active(u8 team_idx) {
    if (team_idx >= (u8)TEAM_COUNT) return 0u;
    return g_teams[team_idx].cup_active;
}
