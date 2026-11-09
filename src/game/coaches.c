/*
 * game/coaches.c -- Treinadores da IA (TREINAD.EF2) e treinadores humanos
 *
 * Strings do original: "CHICOTADAS PSICOLOGICAS", "foi despedido do",
 * "Para o seu lugar foi escolhido", "Troca com", ": quer ir treinar o",
 * "TREINADORES EM JOGO" (= humanos). Probabilidades sao inferidas.
 */

#include <genesis.h>
#include "coaches.h"
#include "data.h"
#include "season.h"
#include "../engine/rng.h"

Sacking g_sack[SACKINGS_MAX];
u8      g_sack_n = 0u;

u8 coaches_free(void) {
    u8 used[COACH_COUNT], c, t, n = 0u, cand[COACH_COUNT];
    for (c = 0u; c < (u8)COACH_COUNT; c++) used[c] = 0u;
    for (t = 0u; t < (u8)TEAM_COUNT; t++)
        if (g_teams[t].coach < (u8)COACH_COUNT) used[g_teams[t].coach] = 1u;
    for (c = 0u; c < (u8)COACH_COUNT; c++)
        if (!used[c] && g_coaches[c][0]) cand[n++] = c;
    return n ? cand[rng_range(n)] : COACH_NONE;
}

void coaches_ai_sackings(void) {
    u8 d;
    g_sack_n = 0u;
    if (g_slot < 4u) return;
    for (d = 0u; d < DIVISIONS && g_sack_n < SACKINGS_MAX; d++) {
        u8 order[TEAMS_PER_DIV], k;
        data_sort_div(d, order);
        for (k = 6u; k < TEAMS_PER_DIV && g_sack_n < SACKINGS_MAX; k++) {
            u8 t = order[k];
            Sacking *s;
            if (data_is_human(t) || g_teams[t].boost) continue;
            if (rng_range(100u) >= 12u) continue;
            s = &g_sack[g_sack_n++];
            s->team = t;
            s->old_c = g_teams[t].coach;
            s->swap = TEAM_NONE;
            if (rng_range(100u) < 30u) {
                /* troca de treinadores com outro clube da IA */
                u8 o = (u8)rng_range(TEAM_COUNT);
                if (o != t && !data_is_human(o) && g_teams[o].coach < (u8)COACH_COUNT) {
                    s->swap = o;
                    s->new_c = g_teams[o].coach;
                    g_teams[o].coach = s->old_c;
                    g_teams[o].boost = 3u;
                } else s->new_c = coaches_free();
            } else s->new_c = coaches_free();
            g_teams[t].coach = s->new_c;
            g_teams[t].boost = 3u;
        }
    }
}

/* ---- Humanos -------------------------------------------------------- */
u8 managers_add(const char *name) {
    u8 cand[TEAM_COUNT], n = 0u, t, m, d;
    if (g_mgr_count >= MAX_MANAGERS) return 0xFFu;
    for (d = (u8)(DIVISIONS - 1u); ; d--) {
        for (t = 0u; t < (u8)TEAM_COUNT; t++)
            if (g_teams[t].div == d && !data_is_human(t)) cand[n++] = t;
        if (n || d == 0u) break;
    }
    if (!n) return 0xFFu;
    m = g_mgr_count++;
    data_strcpy(g_mgr[m].name, name, MGR_NAME_LEN);
    g_mgr[m].team = cand[rng_range(n)];
    g_teams[g_mgr[m].team].manager = m;
    g_teams[g_mgr[m].team].coach = COACH_NONE;
    return m;
}

void managers_remove(u8 m) {
    u8 i;
    if (m >= g_mgr_count) return;
    g_teams[g_mgr[m].team].manager = 0xFFu;
    if (g_teams[g_mgr[m].team].coach == COACH_NONE)
        g_teams[g_mgr[m].team].coach = coaches_free();
    for (i = m; i + 1u < g_mgr_count; i++) {
        g_mgr[i] = g_mgr[i + 1u];
        g_rev_n[i][0] = g_rev_n[i + 1u][0];
        g_rev_n[i][1] = g_rev_n[i + 1u][1];
    }
    g_mgr_count--;
    for (i = 0u; i < (u8)TEAM_COUNT; i++) g_teams[i].manager = data_team_mgr(i);
}

void managers_move(u8 m, u8 t) {
    u8 old = g_mgr[m].team;
    u8 c = g_teams[t].coach;
    g_teams[old].manager = 0xFFu;
    g_teams[old].coach = (c < (u8)COACH_COUNT) ? c : coaches_free();
    g_teams[t].manager = m;
    g_teams[t].coach = COACH_NONE;
    g_mgr[m].team = t;
    g_rev_n[m][0] = g_rev_n[m][1] = 0u;
}

u8 managers_job_offer(u8 m) {
    u8 t = g_mgr[m].team, place = data_team_place(t), d = g_teams[t].div;
    u8 cand[TEAM_COUNT], n = 0u, o;
    if (d == 0u) return TEAM_NONE;
    if (!(place <= 3u || g_teams[t].cup_reach >= 4u)) return TEAM_NONE;
    if (rng_range(100u) >= 50u) return TEAM_NONE;
    for (o = 0u; o < (u8)TEAM_COUNT; o++)
        if (g_teams[o].div < d && !data_is_human(o)) cand[n++] = o;
    return n ? cand[rng_range(n)] : TEAM_NONE;
}
