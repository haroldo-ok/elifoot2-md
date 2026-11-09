/*
 * game/match.c -- Simulacao minuto a minuto (inferida; o binario nao
 * expoe a formula). Ataque = 2xAV + MD; defesa = 3xGR + 2xDF + MD.
 * Golos esperados por equipa ~ 3.75 * ataque / (ataque + defesa adv.),
 * casa +15%. Dois clubes iguais em 4-4-2 dao ~1.3 golos cada.
 */

#include <genesis.h>
#include "match.h"
#include "tactics.h"
#include "data.h"
#include "../engine/rng.h"

LiveGame g_live[16];
u8       g_live_n   = 0u;
u8       g_live_cup = 0u;

static u16 goal_threshold(u16 att, u16 def, u8 home) {
    /* lam100 = golos esperados * 100 em 90 min */
    u32 lam100 = (375UL * att) / ((u32)att + def);
    if (home) lam100 = (lam100 * 115UL) / 100UL;
    /* por minuto, em 1/10000: lam100 * 10000 / (90 * 100) */
    return (u16)((lam100 * 10UL) / 9UL);
}

void match_recompute(u8 i) {
    LiveGame *g = &g_live[i];
    u16 ah, dh, aa, da;
    tactics_rating(g->home, &ah, &dh);
    tactics_rating(g->away, &aa, &da);
    g->thr_h = goal_threshold(ah, da, 1u);
    g->thr_a = goal_threshold(aa, dh, 0u);
}

void match_setup(u8 i, u8 home, u8 away, u8 league) {
    LiveGame *g = &g_live[i];
    (void)league;
    g->home = home; g->away = away;
    g->hg = g->ag = 0u;
    g->last_team = TEAM_NONE;
    g->last_min = 0u;
    g->last_player = PLAYER_NONE;
    g->spectators = 0u;
    g->extra = g->pens = 0u;
    g->ph = g->pa = 0u;
    match_recompute(i);
}

/* Marcador: titulares, AV peso 6, MD 3, DF 1, GR 0. */
static u16 pick_scorer(u8 t) {
    u16 st[16];
    u8 n = tactics_starters(t, st), i;
    u16 total = 0u, r;
    for (i = 0u; i < n; i++) {
        u8 pos = g_players[st[i]].pos;
        total = (u16)(total + (pos == POS_AV ? 6u : pos == POS_MD ? 3u : pos == POS_DF ? 1u : 0u));
    }
    if (total == 0u) return n ? st[0] : PLAYER_NONE;
    r = rng_range(total);
    for (i = 0u; i < n; i++) {
        u8 pos = g_players[st[i]].pos;
        u16 w = (u16)(pos == POS_AV ? 6u : pos == POS_MD ? 3u : pos == POS_DF ? 1u : 0u);
        if (r < w) return st[i];
        r = (u16)(r - w);
    }
    return st[0];
}

static void goal(LiveGame *g, u8 home, u8 m, u8 league) {
    u8 t = home ? g->home : g->away;
    u16 p = pick_scorer(t);
    if (home) g->hg++; else g->ag++;
    g->last_team = t;
    g->last_min = m;
    g->last_player = p;
    if (league && p != PLAYER_NONE && g_players[p].goals < 255u) g_players[p].goals++;
}

u8 match_minute(u8 i, u8 m, u8 league) {
    LiveGame *g = &g_live[i];
    u8 scored = 0u;
    if (rng_range(10000u) < g->thr_h) { goal(g, 1u, m, league); scored = 1u; }
    if (rng_range(10000u) < g->thr_a) { goal(g, 0u, m, league); scored = 1u; }
    return scored;
}

u8 match_penalties(u8 i, PenaltyCb cb) {
    LiveGame *g = &g_live[i];
    u8 k = 0u;
    g->pens = 1u;
    g->ph = g->pa = 0u;
    for (;;) {
        u8 sh = (rng_range(100u) < 75u), sa = (rng_range(100u) < 75u);
        g->ph = (u8)(g->ph + sh);
        if (cb) cb(i, k, 1u, sh);
        g->pa = (u8)(g->pa + sa);
        if (cb) cb(i, k, 0u, sa);
        k++;
        if (k >= 5u && g->ph != g->pa) break;
        if (k < 5u) {
            /* ja decidido antes dos 5? */
            u8 left = (u8)(5u - k);
            if (g->ph > (u8)(g->pa + left) || g->pa > (u8)(g->ph + left)) break;
        }
        if (k > 30u) { g->ph++; break; }
    }
    return (g->ph > g->pa) ? g->home : g->away;
}
