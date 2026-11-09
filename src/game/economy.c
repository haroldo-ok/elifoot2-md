/*
 * game/economy.c -- Ordenados, bilheteira, socios, estadio, premios
 *
 * Observado no original: ordenados pagos todas as jornadas; receita de
 * casa = publico x preco do bilhete (15183 x 10); bancadas de 5000
 * lugares a 100000; premios de Ctrl+F3. O resto e inferido.
 */

#include <genesis.h>
#include "economy.h"
#include "data.h"
#include "history.h"
#include "season.h"
#include "../engine/rng.h"

static const long s_prizes[7] = {
    1000000L, 500000L, 300000L, 500000L, 200000L, 200000L, 200000L
};
static const char * const s_prize_lbl[7] = {
    "VENCEDOR DO CAMPEONATO", "2o CLASSIFICADO", "3o CLASSIFICADO",
    "VENCEDOR DA TACA", "MELHOR ATAQUE", "MELHOR DEFESA", "MELHOR MARCADOR"
};

long economy_prize(u8 k) { return (k < 7u) ? s_prizes[k] : 0L; }
const char *economy_prize_label(u8 k) { return (k < 7u) ? s_prize_lbl[k] : ""; }

/* Publico: socios (quase todos vao) + procura que depende do adversario. */
u16 economy_attendance(u8 home, u8 away) {
    Team *h = &g_teams[home], *a = &g_teams[away];
    u32 v = ((u32)h->socios * 90UL) / 100UL;
    u16 cap = data_capacity(home);
    v += rng_range((u16)(h->socios / 4u + 1u));
    v += (u32)a->socios / 10UL;
    if (a->div < h->div) v += (u32)a->socios / 8UL;   /* visita de um grande */
    if (v > cap) v = cap;
    return (u16)v;
}

void economy_home_revenue(const LiveGame *g, u8 kind) {
    Team *h = &g_teams[g->home];
    long rev = (long)g->spectators * (long)h->ticket;
    u8 m = data_team_mgr(g->home);
    h->money += rev;
    if (m != 0xFFu) {
        /* bilhetes = publico sem os socios presentes (inferido) */
        u16 tick = (g->spectators > h->socios) ? (u16)(g->spectators - h->socios) : 0u;
        data_record_revenue(m, kind == SLOT_LEAGUE ? 0u : 1u, g->away,
                            g->hg, g->ag, tick, g->spectators, rev);
    }
}

static void socios_delta(u8 t, s16 pm) {
    Team *tm = &g_teams[t];
    s32 v = (s32)tm->socios + ((s32)tm->socios * pm) / 1000L
            + (s32)rng_range(41u) - 20L;
    if (v < 1000) v = 1000;
    if (v > 60000) v = 60000;
    tm->socios = (u16)v;
}

void economy_socios_after(const LiveGame *g) {
    s16 h = (g->hg > g->ag) ? 6 : (g->hg < g->ag) ? -6 : 0;
    socios_delta(g->home, h);
    socios_delta(g->away, (s16)-h);
}

void economy_pay_salaries(u8 t) {
    g_teams[t].money -= data_total_salaries(t);
}

void economy_stands_tick(u8 t) {
    Team *tm = &g_teams[t];
    u8 i, k = 0u;
    for (i = 0u; i < tm->nstands; i++) {
        if (tm->stand_life[i] > 1u) tm->stand_life[k++] = (u8)(tm->stand_life[i] - 1u);
        /* bancada que chega a 0 e demolida */
    }
    for (i = k; i < STANDS_MAX; i++) tm->stand_life[i] = 0u;
    tm->nstands = k;
}

u8 economy_build_stand(u8 t) {
    Team *tm = &g_teams[t];
    if (tm->nstands >= STANDS_MAX || tm->money < STAND_PRICE) return 0u;
    tm->money -= STAND_PRICE;
    tm->stand_life[tm->nstands++] = STAND_LIFE;
    return 1u;
}

void economy_ai_finances(u8 t) {
    Team *tm = &g_teams[t];
    /* A IA repoe bancadas quando o publico nao cabe e ha dinheiro. */
    u16 want = (u16)(tm->socios + tm->socios / 3u);
    if (data_capacity(t) < want && tm->money > 300000L) economy_build_stand(t);
}

static void pay(u8 t, u8 k) {
    if (t < (u8)TEAM_COUNT) g_teams[t].money += economy_prize(k);
}

void economy_pay_prizes(void) {
    u8 d;
    for (d = 0u; d < DIVISIONS; d++) {
        DivAwards *aw = &g_awards[d];
        pay(aw->first, PRIZE_FIRST);
        pay(aw->second, PRIZE_SECOND);
        pay(aw->third, PRIZE_THIRD);
        pay(aw->attack, PRIZE_ATTACK);
        pay(aw->defence, PRIZE_DEFENCE);
        if (aw->scorer != PLAYER_NONE) pay(g_players[aw->scorer].team, PRIZE_SCORER);
    }
    pay(g_cup_winner, PRIZE_CUP);
}
