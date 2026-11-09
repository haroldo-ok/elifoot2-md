/*
 * game/market.c -- Leiloes, transferencias e pedidos de aumento
 *
 * Observado: ofertas da IA ficam 2-2.8x acima do ordenado minimo
 * (2700 -> 6200, 2600 -> 7100, 2000 -> 4800, 2200 -> 5400).
 * Pedido de aumento: "pede ... 1600 ... leilao pelo preco de 18000"
 * para um jogador de forca 15 (preco ~ forca x 1200).
 */

#include <genesis.h>
#include "market.h"
#include "data.h"
#include "../engine/rng.h"

u8 market_can_lose(u8 t, u16 p) {
    if (data_squad_count(t) <= MIN_SQUAD) return 0u;
    if (g_players[p].pos == POS_GR && data_gk_count(t) <= 1u) return 0u;
    return 1u;
}

u16 market_pick_auction(void) {
    u8 tries, debtor = TEAM_NONE, t0;
    /* Clubes da IA com dividas vendem primeiro (inferido). */
    for (t0 = 0u; t0 < (u8)TEAM_COUNT; t0++)
        if (g_teams[t0].money < 0L && !data_is_human(t0) && data_squad_count(t0) > 15u) { debtor = t0; break; }
    if (debtor != TEAM_NONE && rng_range(2u)) {
        u16 sq[MAX_SQUAD + 8];
        u8 n = data_squad(debtor, sq, (u8)(MAX_SQUAD + 8u));
        u16 p = sq[rng_range(n)];
        if (market_can_lose(debtor, p)) return p;
    }
    for (tries = 0u; tries < 40u; tries++) {
        u16 p = rng_range(PLAYER_COUNT);
        u8 t = g_players[p].team;
        if (t >= (u8)TEAM_COUNT || data_is_human(t)) continue;
        if (data_squad_count(t) <= 15u) continue;
        if (!market_can_lose(t, p)) continue;
        return p;
    }
    return PLAYER_NONE;
}

u8 market_ai_bid(u16 p, long price, u8 exclude, u16 *salary) {
    u8 t, best = TEAM_NONE;
    u16 best_sal = 0u, minsal = data_min_salary(p);
    long fair = (long)g_players[p].str * 3600L;   /* acima disto a IA nao paga */
    if (price > fair) return TEAM_NONE;
    for (t = 0u; t < (u8)TEAM_COUNT; t++) {
        u16 s;
        if (t == exclude || t == g_players[p].team || data_is_human(t)) continue;
        if (g_teams[t].money < price + 100000L) continue;
        if (data_squad_count(t) >= MAX_SQUAD) continue;
        if (rng_range(100u) >= 20u) continue;          /* nem todos se interessam */
        s = (u16)(((u32)minsal * (20u + rng_range(9u))) / 10u);
        s = (u16)((s / 100u) * 100u);
        if (s > best_sal) { best_sal = s; best = t; }
    }
    *salary = best_sal;
    return best;
}

void market_transfer(u16 p, u8 to, u16 salary, long price) {
    Player *pl = &g_players[p];
    u8 from = pl->team;
    if (from < (u8)TEAM_COUNT) g_teams[from].money += price;
    g_teams[to].money -= price;
    pl->team = to;
    data_rosters_changed();
    pl->salary = salary;
    pl->flags &= (u8)~(PF_FIELD | PF_SUBBED);
    data_record_transfer(p, from, to);
}

void market_release(u16 p) {
    u8 tries;
    for (tries = 0u; tries < 50u; tries++) {
        u8 t = (u8)rng_range(TEAM_COUNT);
        if (t == g_players[p].team || data_is_human(t)) continue;
        if (data_squad_count(t) >= MAX_SQUAD) continue;
        market_transfer(p, t, data_min_salary(p), 0L);
        return;
    }
    g_players[p].team = TEAM_NONE;
    data_rosters_changed();
}

u16 market_demand(u16 p) {
    return data_min_salary(p);
}

u8 market_salary_request(u8 t, u16 *out_p, u16 *new_salary, long *auction_price) {
    u16 sq[MAX_SQUAD + 8];
    u8 n = data_squad(t, sq, (u8)(MAX_SQUAD + 8u)), i, k = 0u;
    u16 cand[MAX_SQUAD + 8];
    if (rng_range(100u) >= 35u) return 0u;
    /* jogadores mal pagos para a forca actual e sem aumento esta epoca */
    for (i = 0u; i < n; i++) {
        Player *pl = &g_players[sq[i]];
        if (pl->flags & PF_RAISED) continue;
        if ((u16)pl->salary < (u16)(pl->str * 100u)) cand[k++] = sq[i];
    }
    if (k == 0u) return 0u;
    *out_p = cand[rng_range(k)];
    {
        Player *pl = &g_players[*out_p];
        u16 s = (u16)(pl->str * 100u + 100u * rng_range(3u));
        if (s <= pl->salary) s = (u16)(pl->salary + 100u);
        *new_salary = (u16)((s / 100u) * 100u);
        *auction_price = (long)pl->str * 1200L;
    }
    return 1u;
}
