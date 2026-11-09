/*
 * game/season.c -- Calendario, campeonato, taca e passagem de jornada
 */

#include <genesis.h>
#include "season.h"
#include "data.h"
#include "match.h"
#include "tactics.h"
#include "economy.h"
#include "history.h"
#include "../engine/rng.h"

/* ---- Calendario --------------------------------------------------- */
/* slot: 0=L1 1=L2 2=T1 3=L3 4=T2 ... 18=T9 19=L11 20=T10 21=L12 22=L13 23=L14 */
u8 season_slot_kind(u8 s) {
    if (s < 2u || s > 20u) return SLOT_LEAGUE;
    return ((s & 1u) == 0u) ? SLOT_CUP : SLOT_LEAGUE;
}

u8 season_slot_index(u8 s) {
    if (s < 2u) return s;                          /* L1, L2 */
    if (s > 20u) return (u8)(s - 10u);             /* L12..L14 = 11..13 */
    if ((s & 1u) == 0u) return (u8)((s - 2u) / 2u); /* T1..T10 = 0..9 */
    return (u8)((s - 3u) / 2u + 2u);               /* L3..L11 = 2..10 */
}

/* Metodo do circulo para 8 equipas: 7 jornadas, depois inverte casa/fora. */
void league_pair(u8 r, u8 k, u8 *home, u8 *away) {
    u8 rr = (u8)(r % 7u), a, b, swap;
    if (k == 0u) {
        a = 7u; b = rr;
        swap = (u8)(rr & 1u);
    } else {
        a = (u8)((rr + k) % 7u);
        b = (u8)((rr + 7u - k) % 7u);
        swap = (u8)(k & 1u);
    }
    if (r >= 7u) swap ^= 1u;
    if (swap) { *home = b; *away = a; } else { *home = a; *away = b; }
}

u8 league_opponent(u8 t, u8 r, u8 *home) {
    u8 d = g_teams[t].div, s = data_div_slot(t), k, h, a;
    for (k = 0u; k < 4u; k++) {
        league_pair(r, k, &h, &a);
        if (h == s) { *home = 1u; return g_div_team[d][a]; }
        if (a == s) { *home = 0u; return g_div_team[d][h]; }
    }
    *home = 1u;
    return TEAM_NONE;
}

/* ---- Taca --------------------------------------------------------- */
void cup_draw(void) {
    u8 pool[TEAM_COUNT], n = 0u, t, i;
    for (t = 0u; t < (u8)TEAM_COUNT; t++) if (g_teams[t].cup_alive) pool[n++] = t;
    for (i = (u8)(n - 1u); i > 0u; i--) {
        u8 j = (u8)rng_range((u16)(i + 1u)), x = pool[i];
        pool[i] = pool[j]; pool[j] = x;
    }
    g_cup_n = 0u;
    for (i = 0u; (u8)(i + 1u) < n && g_cup_n < 16u; i = (u8)(i + 2u)) {
        CupTie *c = &g_cup[g_cup_n++];
        c->a = pool[i]; c->b = pool[i + 1u];
        c->ga1 = c->gb1 = c->ga2 = c->gb2 = 0u;
        c->pa = c->pb = 0u;
        c->winner = TEAM_NONE;
        c->extra = 0u;
    }
}

u8 cup_tie_of(u8 t) {
    u8 i;
    for (i = 0u; i < g_cup_n; i++) if (g_cup[i].a == t || g_cup[i].b == t) return i;
    return 0xFFu;
}

u8 season_next_match(u8 t, u8 *home, u8 *slot) {
    u8 s;
    for (s = g_slot; s < SEASON_SLOTS; s++) {
        u8 idx = season_slot_index(s);
        if (season_slot_kind(s) == SLOT_LEAGUE) {
            *slot = s;
            return league_opponent(t, idx, home);
        }
        /* taca: so a fase actual e conhecida */
        if (CUP_PHASE_OF(idx) == g_cup_phase && g_teams[t].cup_alive) {
            u8 c = cup_tie_of(t);
            if (c != 0xFFu) {
                u8 a_side = (g_cup[c].a == t);
                *slot = s;
                *home = CUP_LEG_OF(idx) == 0u ? a_side : (u8)!a_side;
                return a_side ? g_cup[c].b : g_cup[c].a;
            }
        }
    }
    *home = 1u; *slot = SEASON_SLOTS;
    return TEAM_NONE;
}

/* ---- Epoca ---------------------------------------------------------- */
void season_start(void) {
    g_slot = 0u;
    g_cup_phase = 0u;
    g_cup_winner = TEAM_NONE;
    data_new_season_tables();
    cup_draw();
}

void season_setup_games(void) {
    u8 kind = season_slot_kind(g_slot), idx = season_slot_index(g_slot);
    u8 t, i;

    /* IA escolhe a tactica; humanos ja escolheram (garante 11 titulares) */
    for (t = 0u; t < (u8)TEAM_COUNT; t++) {
        if (!data_is_human(t)) tactics_ai(t);
        else {
            u16 st[16];
            if (tactics_starters(t, st) < 11u) tactics_autoselect(t, g_teams[t].formation);
        }
    }

    g_live_n = 0u;
    g_live_cup = (kind == SLOT_CUP);
    if (kind == SLOT_LEAGUE) {
        u8 d, k, h, a;
        for (d = 0u; d < DIVISIONS; d++)
            for (k = 0u; k < 4u; k++) {
                league_pair(idx, k, &h, &a);
                match_setup(g_live_n++, g_div_team[d][h], g_div_team[d][a], 1u);
            }
    } else {
        u8 leg = CUP_LEG_OF(idx);
        for (i = 0u; i < g_cup_n; i++) {
            CupTie *c = &g_cup[i];
            if (leg == 0u) match_setup(g_live_n++, c->a, c->b, 0u);
            else           match_setup(g_live_n++, c->b, c->a, 0u);
        }
    }
    for (i = 0u; i < g_live_n; i++)
        g_live[i].spectators = economy_attendance(g_live[i].home, g_live[i].away);
}

u8 season_cup_tied(u8 i) {
    CupTie *c = &g_cup[i];
    LiveGame *g = &g_live[i];
    /* 2a mao: b em casa */
    u8 agg_a = (u8)(c->ga1 + g->ag), agg_b = (u8)(c->gb1 + g->hg);
    return agg_a == agg_b;
}

static void apply_league(LiveGame *g) {
    Team *h = &g_teams[g->home], *a = &g_teams[g->away];
    u8 d = h->div;
    h->hgf = (u8)(h->hgf + g->hg); h->hga = (u8)(h->hga + g->ag);
    a->agf = (u8)(a->agf + g->ag); a->aga = (u8)(a->aga + g->hg);
    if (g->hg > g->ag)      { h->hw++; a->al++; }
    else if (g->hg < g->ag) { h->hl++; a->aw++; }
    else                    { h->hd++; a->ad++; }
    g_res[d][data_div_slot(g->home)][data_div_slot(g->away)] =
        (u8)(((g->hg > 15u ? 15u : g->hg) << 4) | (g->ag > 15u ? 15u : g->ag));
}

static void cup_resolve(void) {
    u8 i, phase = g_cup_phase;
    for (i = 0u; i < g_cup_n; i++) {
        CupTie *c = &g_cup[i];
        u8 loser;
        if (c->winner == TEAM_NONE) {
            u8 aa = (u8)(c->ga1 + c->ga2), ab = (u8)(c->gb1 + c->gb2);
            c->winner = (aa >= ab) ? c->a : c->b;
        }
        loser = (c->winner == c->a) ? c->b : c->a;
        g_teams[loser].cup_alive = 0u;
        g_teams[loser].cup_reach = phase;
        if (g_teams[c->winner].cup_reach < (u8)(phase + 1u))
            g_teams[c->winner].cup_reach = (u8)(phase + 1u);
    }
    if (phase >= (u8)(CUP_PHASES - 1u)) {
        g_cup_winner = g_cup[0].winner;
        g_teams[g_cup_winner].cup_reach = 5u;
        g_cup_phase = CUP_PHASES;
    } else {
        g_cup_phase++;
        cup_draw();
    }
}

void season_apply_results(void) {
    u8 i;
    u8 kind = season_slot_kind(g_slot), idx = season_slot_index(g_slot);
    for (i = 0u; i < g_live_n; i++) {
        LiveGame *g = &g_live[i];
        economy_home_revenue(g, kind);
        if (kind == SLOT_LEAGUE) {
            apply_league(g);
        } else {
            CupTie *c = &g_cup[i];
            if (CUP_LEG_OF(idx) == 0u) { c->ga1 = g->hg; c->gb1 = g->ag; }
            else {
                c->gb2 = g->hg; c->ga2 = g->ag;
                c->extra = g->extra;
                if (g->pens) {
                    c->pb = g->ph; c->pa = g->pa;
                    c->winner = (g->ph > g->pa) ? c->b : c->a;
                }
            }
        }
        economy_socios_after(g);
    }
    if (kind == SLOT_CUP && CUP_LEG_OF(idx) == 1u) cup_resolve();
}

void season_end_slot(void) {
    u8 t;
    u16 p;
    for (t = 0u; t < (u8)TEAM_COUNT; t++) {
        economy_pay_salaries(t);
        economy_stands_tick(t);
        if (g_teams[t].boost) g_teams[t].boost--;
        if (!data_is_human(t)) economy_ai_finances(t);
    }
    /* Forma: a forca varia +-3 a volta da base (observado no original) */
    for (p = 0u; p < PLAYER_COUNT; p++) {
        Player *pl = &g_players[p];
        s16 v = (s16)pl->base + (s16)rng_range(7u) - 3;
        if (v < 1) v = 1;
        if (v > 99) v = 99;
        pl->str = (u8)v;
        pl->flags &= (u8)~PF_SUBBED;
    }
    g_slot++;
}

void season_close(void) {
    history_compute_awards();
    economy_pay_prizes();
    history_record_season();
}

void season_promote(void) {
    u8 d, i, order[DIVISIONS][TEAMS_PER_DIV];
    u16 p;
    for (d = 0u; d < DIVISIONS; d++) data_sort_div(d, order[d]);
    for (d = 0u; d + 1u < DIVISIONS; d++)
        for (i = 0u; i < PROMOTED; i++) {
            g_teams[order[d][TEAMS_PER_DIV - 1u - i]].div = (u8)(d + 1u);
            g_teams[order[d + 1u][i]].div = d;
        }
    /* Evolucao das forcas de base no defeso */
    for (p = 0u; p < PLAYER_COUNT; p++) {
        Player *pl = &g_players[p];
        s16 v = (s16)pl->base + (s16)rng_range(5u) - 2;
        if (v < 1) v = 1;
        if (v > 60) v = 60;
        pl->base = (u8)v;
        pl->str = pl->base;
    }
    if (g_season < 250u) g_season++;
    season_start();
}
