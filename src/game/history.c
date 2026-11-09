/*
 * game/history.c -- Palmares, ultimos vencedores, premios da temporada
 */

#include <genesis.h>
#include "history.h"
#include "data.h"

TeamHonours g_honours[TEAM_COUNT];
WinnerRec   g_winners[WINNERS_MAX];
u8          g_winners_n = 0u;
DivAwards   g_awards[DIVISIONS];
u8          g_awards_valid = 0u;
char        g_hist_mgr_name[MAX_MANAGERS][MGR_NAME_LEN];

void history_reset(void) {
    u8 t, k;
    for (t = 0u; t < (u8)TEAM_COUNT; t++) {
        u8 *p = (u8 *)&g_honours[t];
        for (k = 0u; k < (u8)sizeof(TeamHonours); k++) p[k] = 0u;
    }
    for (k = 0u; k < MAX_MANAGERS; k++) g_hist_mgr_name[k][0] = '\0';
    g_winners_n = 0u;
    g_awards_valid = 0u;
}

u8 history_coach_of(u8 t) {
    u8 m = data_team_mgr(t);
    if (m != 0xFFu) {
        data_strcpy(g_hist_mgr_name[m], g_mgr[m].name, MGR_NAME_LEN);
        return COACH_HUMAN(m);
    }
    return g_teams[t].coach;
}

const char *history_coach_name(u8 id) {
    if (id >= 200u && (u8)(id - 200u) < MAX_MANAGERS) return g_hist_mgr_name[id - 200u];
    if (id < COACH_COUNT) return g_coaches[id];
    return "---";
}

void history_compute_awards(void) {
    u8 d;
    for (d = 0u; d < DIVISIONS; d++) {
        DivAwards *aw = &g_awards[d];
        u8 order[TEAMS_PER_DIV], i;
        u16 p;
        data_sort_div(d, order);
        aw->first = order[0]; aw->second = order[1]; aw->third = order[2];
        aw->attack = aw->defence = order[0];
        aw->attack_goals = TEAM_GF(&g_teams[order[0]]);
        aw->defence_goals = TEAM_GA(&g_teams[order[0]]);
        for (i = 1u; i < TEAMS_PER_DIV; i++) {
            Team *tm = &g_teams[order[i]];
            if (TEAM_GF(tm) > aw->attack_goals)  { aw->attack = order[i];  aw->attack_goals = TEAM_GF(tm); }
            if (TEAM_GA(tm) < aw->defence_goals) { aw->defence = order[i]; aw->defence_goals = TEAM_GA(tm); }
        }
        aw->scorer = PLAYER_NONE;
        aw->scorer_goals = 0u;
        for (p = 0u; p < PLAYER_COUNT; p++) {
            Player *pl = &g_players[p];
            if (pl->team >= (u8)TEAM_COUNT || g_teams[pl->team].div != d) continue;
            if (pl->goals > aw->scorer_goals) { aw->scorer = p; aw->scorer_goals = pl->goals; }
        }
    }
    g_awards_valid = 1u;
}

void history_record_season(void) {
    u8 t, d, i;
    WinnerRec *w;
    for (t = 0u; t < (u8)TEAM_COUNT; t++) {
        TeamHonours *h = &g_honours[t];
        u8 r = g_teams[t].cup_reach;
        d = g_teams[t].div;
        if (h->seasons[d] < 255u) h->seasons[d]++;
        if (r == 5u)      { if (h->cup_wins < 255u) h->cup_wins++; if (h->cup_finals < 255u) h->cup_finals++; }
        else if (r == 4u) { if (h->cup_finals   < 255u) h->cup_finals++; }
        else if (r == 3u) { if (h->cup_semis    < 255u) h->cup_semis++; }
        else if (r == 2u) { if (h->cup_quarters < 255u) h->cup_quarters++; }
        else if (r == 1u) { if (h->cup_r2       < 255u) h->cup_r2++; }
    }
    if (g_winners_n >= WINNERS_MAX) {
        for (i = 0u; i + 1u < WINNERS_MAX; i++) g_winners[i] = g_winners[i + 1u];
        g_winners_n = (u8)(WINNERS_MAX - 1u);
    }
    w = &g_winners[g_winners_n++];
    w->season = g_season;
    for (d = 0u; d < DIVISIONS; d++) {
        u8 c = g_awards[d].first;
        w->champ[d] = c;
        w->champ_coach[d] = history_coach_of(c);
        if (g_honours[c].titles[d] < 255u) g_honours[c].titles[d]++;
    }
    w->cup = g_cup_winner;
    w->cup_coach = (g_cup_winner < (u8)TEAM_COUNT) ? history_coach_of(g_cup_winner) : 0xFFu;
    w->_pad = 0u;
}

u8 history_coach_titles(u8 id, u8 cup) {
    u8 i, d, n = 0u;
    for (i = 0u; i < g_winners_n; i++) {
        if (cup) { if (g_winners[i].cup_coach == id) n++; }
        else for (d = 0u; d < DIVISIONS; d++) if (g_winners[i].champ_coach[d] == id) n++;
    }
    return n;
}
