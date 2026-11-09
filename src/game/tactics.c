/*
 * game/tactics.c -- Formacoes F1..F10 e escalacao
 *
 * Ordem e nomes confirmados no binario (0x11578):
 * F1 3-4-3, F2 4-3-3, F3 4-4-2, F4 4-5-1, F5 5-2-3,
 * F6 5-3-2, F7 5-4-1, F8 5-5-0, F9 6-3-1, F10 6-4-0.
 * F3 escolheu os melhores por posicao no original (observado).
 */

#include <genesis.h>
#include "tactics.h"
#include "data.h"
#include "../engine/rng.h"

static const char s_names[FORM_COUNT][6] = {
    "3-4-3", "4-3-3", "4-4-2", "4-5-1", "5-2-3",
    "5-3-2", "5-4-1", "5-5-0", "6-3-1", "6-4-0"
};
static const u8 s_need[FORM_COUNT][4] = {
    {1,3,4,3}, {1,4,3,3}, {1,4,4,2}, {1,4,5,1}, {1,5,2,3},
    {1,5,3,2}, {1,5,4,1}, {1,5,5,0}, {1,6,3,1}, {1,6,4,0}
};

const char *tactics_name(u8 f) { return s_names[(f < FORM_COUNT) ? f : 2u]; }
u8 tactics_need(u8 f, u8 pos) { return s_need[(f < FORM_COUNT) ? f : 2u][pos & 3u]; }

/* Escalacao sobre uma lista pequena (o plantel), para nao varrer 512. */
static void select_list(const u16 *sq, u8 n, u8 form) {
    u8 pos, i, on = 0u;
    for (i = 0u; i < n; i++) g_players[sq[i]].flags &= (u8)~(PF_FIELD | PF_SUBBED);
    for (pos = 0u; pos < 4u; pos++) {
        u8 k, need = tactics_need(form, pos);
        for (k = 0u; k < need; k++) {
            u8 best = 0xFFu, bs = 0u;
            for (i = 0u; i < n; i++) {
                Player *pl = &g_players[sq[i]];
                if (pl->pos == pos && !(pl->flags & PF_FIELD) && (best == 0xFFu || pl->str > bs)) {
                    best = i; bs = pl->str;
                }
            }
            if (best == 0xFFu) break;
            g_players[sq[best]].flags |= PF_FIELD;
            on++;
        }
    }
    /* Faltam jogadores numa posicao: completa ate 11 com os melhores. */
    while (on < 11u) {
        u8 best = 0xFFu, bs = 0u;
        for (i = 0u; i < n; i++) {
            Player *pl = &g_players[sq[i]];
            if (!(pl->flags & PF_FIELD) && (best == 0xFFu || pl->str > bs)) { best = i; bs = pl->str; }
        }
        if (best == 0xFFu) break;
        g_players[sq[best]].flags |= PF_FIELD;
        on++;
    }
}

void tactics_autoselect(u8 t, u8 form) {
    u16 sq[MAX_SQUAD + 8];
    u8 n = data_squad(t, sq, (u8)(MAX_SQUAD + 8u));
    if (form >= FORM_COUNT) form = 2u;
    g_teams[t].formation = form;
    select_list(sq, n, form);
}

static void rating_list(const u16 *sq, u8 n, u16 *att, u16 *def) {
    u16 a = 0u, d = 0u;
    u8 i;
    for (i = 0u; i < n; i++) {
        Player *pl = &g_players[sq[i]];
        if (!(pl->flags & PF_FIELD)) continue;
        switch (pl->pos) {
            case POS_GR: d = (u16)(d + pl->str * 3u); break;
            case POS_DF: d = (u16)(d + pl->str * 2u); break;
            case POS_MD: d = (u16)(d + pl->str); a = (u16)(a + pl->str); break;
            default:     a = (u16)(a + pl->str * 2u); break;
        }
    }
    *att = a ? a : 1u;
    *def = d ? d : 1u;
}

void tactics_rating(u8 t, u16 *att, u16 *def) {
    u16 sq[MAX_SQUAD + 8];
    u8 n = data_squad(t, sq, (u8)(MAX_SQUAD + 8u));
    rating_list(sq, n, att, def);
    if (g_teams[t].boost) {          /* chicotada psicologica */
        *att = (u16)(*att + *att / 10u);
        *def = (u16)(*def + *def / 10u);
    }
}

void tactics_ai(u8 t) {
    /* A IA experimenta 4 formacoes equilibradas e fica com a melhor soma. */
    static const u8 cand[4] = { 1u, 2u, 3u, 5u };
    u16 sq[MAX_SQUAD + 8];
    u8 n = data_squad(t, sq, (u8)(MAX_SQUAD + 8u));
    u8 i, best = 2u;
    u16 bestv = 0u;
    for (i = 0u; i < 4u; i++) {
        u16 a, d, v;
        select_list(sq, n, cand[i]);
        rating_list(sq, n, &a, &d);
        v = (u16)(a + d / 2u + (u16)rng_range(8u));
        if (v > bestv) { bestv = v; best = cand[i]; }
    }
    g_teams[t].formation = best;
    select_list(sq, n, best);
}

u8 tactics_starters(u8 t, u16 *out) {
    u16 sq[MAX_SQUAD + 8];
    u8 n = data_squad(t, sq, (u8)(MAX_SQUAD + 8u)), i, k = 0u;
    for (i = 0u; i < n; i++) if (g_players[sq[i]].flags & PF_FIELD) out[k++] = sq[i];
    return k;
}

u8 tactics_bench(u8 t, u16 *out) {
    u16 sq[MAX_SQUAD + 8];
    u8 n = data_squad(t, sq, (u8)(MAX_SQUAD + 8u)), i, k = 0u;
    for (i = 0u; i < n; i++) if (!(g_players[sq[i]].flags & PF_FIELD)) out[k++] = sq[i];
    return k;
}
