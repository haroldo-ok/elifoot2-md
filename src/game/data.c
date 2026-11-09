/*
 * game/data.c -- Estado global, carga de ROM e utilitarios
 *
 * teams.bin (pack_data.py): u16 count, u16 players_per_team, depois por
 * equipa 26 bytes (name[20], fg, bg, nac[3], pad) + 16 x 24 bytes
 * (name[16], pos, nac[3], pad[4]).
 */

#include <genesis.h>
#include "data.h"
#include "../engine/rng.h"
#include "../../res/resources.h"

#define BIN_TEAM_REC     26u
#define BIN_PLAYER_REC   24u
#define BIN_TEAM_BLOCK   (BIN_TEAM_REC + PLAYERS_PER_TEAM * BIN_PLAYER_REC)

Player  g_players[PLAYER_COUNT];
Team    g_teams[TEAM_COUNT];
char    g_coaches[COACH_COUNT][COACH_NAME_LEN];
Manager g_mgr[MAX_MANAGERS];
u8      g_mgr_count = 0u;
u8      g_cur_mgr   = 0u;
u8      g_season    = 1u;
u8      g_slot      = 0u;
u8      g_div_team[DIVISIONS][TEAMS_PER_DIV];
u8      g_res[DIVISIONS][TEAMS_PER_DIV][TEAMS_PER_DIV];
CupTie  g_cup[16];
u8      g_cup_n      = 0u;
u8      g_cup_phase  = 0u;
u8      g_cup_winner = TEAM_NONE;
TransferRec g_transfers[TRANSFER_HIST];
u8          g_transfer_n = 0u;
RevenueRec  g_rev[MAX_MANAGERS][2][REVENUE_HIST];
u8          g_rev_n[MAX_MANAGERS][2];

/* Forca de base inicial por divisao (observado: 1a/2a ~29-38, 4a ~10-15) */
static const u8 s_div_base[DIVISIONS]   = { 34u, 29u, 21u, 13u };
/* Bancadas iniciais por divisao (observado: 1a enche 55000, 4a 30000) */
static const u8 s_div_stands[DIVISIONS] = { 11u, 10u, 9u, 6u };
/* Socios iniciais por divisao (4a: 13250 observado) */
static const u16 s_div_socios[DIVISIONS] = { 38000u, 28000u, 20000u, 13000u };

void data_strcpy(char *dst, const char *src, u8 cap) {
    u8 i;
    for (i = 0u; i + 1u < cap && src[i]; i++) dst[i] = src[i];
    dst[i] = '\0';
}

static s16 rnd_pm(u16 r) {   /* -r .. +r */
    return (s16)rng_range((u16)(2u * r + 1u)) - (s16)r;
}

void data_new_season_tables(void) {
    u8 t, d, i, j, n[DIVISIONS];
    u16 p;
    for (d = 0u; d < DIVISIONS; d++) n[d] = 0u;
    for (t = 0u; t < (u8)TEAM_COUNT; t++) {
        Team *tm = &g_teams[t];
        tm->hw = tm->hd = tm->hl = tm->hgf = tm->hga = 0u;
        tm->aw = tm->ad = tm->al = tm->agf = tm->aga = 0u;
        tm->cup_alive = 1u;
        tm->cup_reach = 0u;
        d = tm->div;
        if (d < DIVISIONS && n[d] < TEAMS_PER_DIV) g_div_team[d][n[d]++] = t;
    }
    for (d = 0u; d < DIVISIONS; d++)
        for (i = 0u; i < TEAMS_PER_DIV; i++)
            for (j = 0u; j < TEAMS_PER_DIV; j++)
                g_res[d][i][j] = 0xFFu;
    for (p = 0u; p < PLAYER_COUNT; p++) {
        g_players[p].goals = 0u;
        g_players[p].flags &= (u8)~(PF_RAISED | PF_SUBBED);
    }
}

void data_init(void) {
    const u8 *ptr = teams_data;
    u16 file_teams = ((u16)ptr[0] << 8) | ptr[1];
    u8  t, i, c;
    u16 p = 0u;

    ptr += 4;
    if (file_teams > TEAM_COUNT) file_teams = TEAM_COUNT;

    for (t = 0u; t < (u8)TEAM_COUNT; t++) {
        Team *tm = &g_teams[t];
        const u8 *rec = ptr + (u16)t * BIN_TEAM_BLOCK;
        u8 d = (u8)(t / TEAMS_PER_DIV), k;
        data_strcpy(tm->name, (const char *)rec, TEAM_NAME_LEN);
        tm->fg = rec[20];
        tm->bg = rec[21];
        tm->div = d;
        tm->formation = 2u;               /* F3 4-4-2 */
        tm->coach = COACH_NONE;
        tm->manager = 0xFFu;
        tm->boost = 0u;
        tm->money = START_MONEY;
        tm->ticket = TICKET_PRICE;
        tm->nstands = (u8)(s_div_stands[d] + rng_range(2u));
        for (k = 0u; k < STANDS_MAX; k++)
            tm->stand_life[k] = (k < tm->nstands) ? (u8)(1u + rng_range(STAND_LIFE)) : 0u;
        tm->socios = (u16)(s_div_socios[d] + (u16)rng_range(s_div_socios[d] / 5u)
                           - s_div_socios[d] / 10u);
        {
            /* forca de equipa varia um pouco dentro da divisao */
            s16 team_base = (s16)s_div_base[d] + rnd_pm(2u);
            for (i = 0u; i < PLAYERS_PER_TEAM; i++, p++) {
                Player *pl = &g_players[p];
                const u8 *pr = rec + BIN_TEAM_REC + (u16)i * BIN_PLAYER_REC;
                s16 b = team_base + rnd_pm(3u);
                s16 sal;
                data_strcpy(pl->name, (const char *)pr, PLAYER_NAME_LEN);
                pl->pos = pr[16] & 3u;
                pl->nac[0] = (char)pr[17]; pl->nac[1] = (char)pr[18]; pl->nac[2] = (char)pr[19];
                pl->team = (t < (u8)file_teams) ? t : TEAM_NONE;
                if (b < 1) b = 1;
                if (b > 60) b = 60;
                pl->base = (u8)b;
                pl->str = pl->base;
                pl->flags = 0u;
                pl->goals = 0u;
                pl->_pad = 0u;
                sal = (s16)((s16)pl->base * 100 + rnd_pm(3u) * 100);
                if (sal < 500) sal = 500;
                pl->salary = (u16)sal;
            }
        }
    }

    /* Treinadores da IA (TREINAD.EF2) */
    ptr = coaches_data;
    {
        u16 n = ((u16)ptr[0] << 8) | ptr[1];
        ptr += 2;
        for (c = 0u; c < (u8)COACH_COUNT; c++) {
            if (c < n) data_strcpy(g_coaches[c], (const char *)(ptr + (u16)c * 20u), COACH_NAME_LEN);
            else g_coaches[c][0] = '\0';
        }
    }
    /* Distribui treinadores distintos ao acaso */
    {
        u8 order[COACH_COUNT];
        for (c = 0u; c < (u8)COACH_COUNT; c++) order[c] = c;
        for (c = (u8)(COACH_COUNT - 1u); c > 0u; c--) {
            u8 j = (u8)rng_range((u16)(c + 1u)), tmp = order[c];
            order[c] = order[j]; order[j] = tmp;
        }
        for (t = 0u; t < (u8)TEAM_COUNT; t++) g_teams[t].coach = order[t];
    }

    data_rosters_changed();
    g_mgr_count = 0u;
    g_cur_mgr = 0u;
    g_season = 1u;
    g_slot = 0u;
    g_cup_n = 0u;
    g_cup_phase = 0u;
    g_cup_winner = TEAM_NONE;
    g_transfer_n = 0u;
    for (i = 0u; i < MAX_MANAGERS; i++) { g_rev_n[i][0] = g_rev_n[i][1] = 0u; }
    data_new_season_tables();
}

/* ---- Plantéis em cache (refeitos so quando alguem muda de clube) ---- */
#define SQ_CAP (MAX_SQUAD + 8u)
static u16 s_sq[TEAM_COUNT][SQ_CAP];
static u8  s_sqn[TEAM_COUNT];
static u8  s_sqgk[TEAM_COUNT];
static u8  s_sq_valid = 0u;

void data_rosters_changed(void) { s_sq_valid = 0u; }

static void rebuild_rosters(void) {
    u8 t, pos;
    u16 p;
    for (t = 0u; t < (u8)TEAM_COUNT; t++) { s_sqn[t] = 0u; s_sqgk[t] = 0u; }
    for (pos = 0u; pos < 4u; pos++)
        for (p = 0u; p < PLAYER_COUNT; p++) {
            Player *pl = &g_players[p];
            t = pl->team;
            if (t >= (u8)TEAM_COUNT || pl->pos != pos || s_sqn[t] >= SQ_CAP) continue;
            s_sq[t][s_sqn[t]++] = p;
            if (pos == POS_GR) s_sqgk[t]++;
        }
    s_sq_valid = 1u;
}

u8 data_squad(u8 t, u16 *out, u8 max) {
    u8 i, n;
    if (t >= (u8)TEAM_COUNT) return 0u;
    if (!s_sq_valid) rebuild_rosters();
    n = s_sqn[t];
    if (n > max) n = max;
    for (i = 0u; i < n; i++) out[i] = s_sq[t][i];
    return n;
}

u8 data_squad_count(u8 t) {
    if (t >= (u8)TEAM_COUNT) return 0u;
    if (!s_sq_valid) rebuild_rosters();
    return s_sqn[t];
}

u8 data_gk_count(u8 t) {
    if (t >= (u8)TEAM_COUNT) return 0u;
    if (!s_sq_valid) rebuild_rosters();
    return s_sqgk[t];
}

static u8 better(u8 a, u8 b) {   /* 1 se a fica a frente de b */
    Team *ta = &g_teams[a], *tb = &g_teams[b];
    s16 da, db;
    if (TEAM_PTS(ta) != TEAM_PTS(tb)) return TEAM_PTS(ta) > TEAM_PTS(tb);
    da = (s16)TEAM_GF(ta) - (s16)TEAM_GA(ta);
    db = (s16)TEAM_GF(tb) - (s16)TEAM_GA(tb);
    if (da != db) return da > db;
    if (TEAM_GF(ta) != TEAM_GF(tb)) return TEAM_GF(ta) > TEAM_GF(tb);
    return a < b;
}

void data_sort_div(u8 div, u8 *out) {
    u8 i, j;
    for (i = 0u; i < TEAMS_PER_DIV; i++) out[i] = g_div_team[div][i];
    for (i = 1u; i < TEAMS_PER_DIV; i++) {
        u8 v = out[i];
        j = i;
        while (j > 0u && better(v, out[j - 1u])) { out[j] = out[j - 1u]; j--; }
        out[j] = v;
    }
}

u8 data_team_place(u8 t) {
    u8 order[TEAMS_PER_DIV], i;
    data_sort_div(g_teams[t].div, order);
    for (i = 0u; i < TEAMS_PER_DIV; i++) if (order[i] == t) return (u8)(i + 1u);
    return TEAMS_PER_DIV;
}

u8 data_div_slot(u8 t) {
    u8 i, d = g_teams[t].div;
    for (i = 0u; i < TEAMS_PER_DIV; i++) if (g_div_team[d][i] == t) return i;
    return 0u;
}

u8 data_team_mgr(u8 t) {
    u8 m;
    for (m = 0u; m < g_mgr_count; m++) if (g_mgr[m].team == t) return m;
    return 0xFFu;
}

u8 data_is_human(u8 t) { return data_team_mgr(t) != 0xFFu; }

long data_price(u16 p) {
    long v = (long)g_players[p].str * 3000L;
    return v - v / 20L + (long)rng_range((u16)(v / 1000L + 1u)) * 100L;
}

u16 data_min_salary(u16 p) {
    u16 v = (u16)(g_players[p].str * 70u);
    v = (u16)(((v + 50u) / 100u) * 100u);
    return (v < 100u) ? 100u : v;
}

long data_total_salaries(u8 t) {
    long s = 0L; u8 i;
    if (t >= (u8)TEAM_COUNT) return 0L;
    if (!s_sq_valid) rebuild_rosters();
    for (i = 0u; i < s_sqn[t]; i++) s += g_players[s_sq[t][i]].salary;
    return s;
}

u16 data_capacity(u8 t) { return (u16)(g_teams[t].nstands * STAND_SEATS); }

void data_record_transfer(u16 p, u8 from, u8 to) {
    TransferRec *r;
    u8 i;
    if (g_transfer_n >= TRANSFER_HIST) {
        for (i = 0u; i + 1u < TRANSFER_HIST; i++) g_transfers[i] = g_transfers[i + 1u];
        g_transfer_n = (u8)(TRANSFER_HIST - 1u);
    }
    r = &g_transfers[g_transfer_n++];
    r->player = p; r->from = from; r->to = to;
    r->str = g_players[p].str; r->_pad = 0u;
    r->salary = g_players[p].salary;
}

void data_record_revenue(u8 m, u8 kind, u8 opp, u8 gh, u8 ga,
                         u16 tickets, u16 spect, long rev) {
    RevenueRec *r;
    u8 i;
    if (m >= MAX_MANAGERS || kind > 1u) return;
    if (g_rev_n[m][kind] >= REVENUE_HIST) {
        for (i = 0u; i + 1u < REVENUE_HIST; i++) g_rev[m][kind][i] = g_rev[m][kind][i + 1u];
        g_rev_n[m][kind] = (u8)(REVENUE_HIST - 1u);
    }
    r = &g_rev[m][kind][g_rev_n[m][kind]++];
    r->opp = opp; r->gh = gh; r->ga = ga; r->_pad = 0u;
    r->tickets = tickets; r->spectators = spect; r->revenue = rev;
}
