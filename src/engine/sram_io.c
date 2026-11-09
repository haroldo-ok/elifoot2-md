/*
 * engine/sram_io.c -- Gravacao do estado completo em SRAM
 *
 * Layout: [0..3] "ELF2", [4..5] versao. Gravacao n em 0x10 + n*0x2000:
 *   [0..1] CRC-16/CCITT dos dados, [2..3] tamanho, [4..] dados.
 * Os nomes de equipas/jogadores nao sao gravados (vem da ROM; os
 * jogadores mantem o indice original).
 */

#include <genesis.h>
#include "sram_io.h"
#include "../game/data.h"
#include "../game/history.h"

#define SLOT_BASE   0x0010UL
#define SLOT_SIZE   0x2000UL
#define SLOT_DATA   4UL

static u32 s_off;
static u16 s_crc;

static u16 crc_step(u16 crc, u8 b) {
    u8 k;
    crc ^= (u16)((u16)b << 8);
    for (k = 0u; k < 8u; k++)
        crc = (crc & 0x8000u) ? (u16)((crc << 1) ^ 0x1021u) : (u16)(crc << 1);
    return crc;
}

static void w8(u8 v)  { SRAM_writeByte(s_off++, v); s_crc = crc_step(s_crc, v); }
static void w16(u16 v){ w8((u8)(v >> 8)); w8((u8)v); }
static void w32(long v){ u32 u = (u32)v; w16((u16)(u >> 16)); w16((u16)u); }
static void wbuf(const void *p, u16 n) { const u8 *b = (const u8 *)p; while (n--) w8(*b++); }

static u8  r8(void)  { u8 v = SRAM_readByte(s_off++); return v; }
static u16 r16(void) { u16 h = r8(); return (u16)((h << 8) | r8()); }
static long r32(void){ u32 h = r16(); return (long)((h << 16) | r16()); }
static void rbuf(void *p, u16 n) { u8 *b = (u8 *)p; while (n--) *b++ = r8(); }

static u32 slot_addr(u8 slot) { return SLOT_BASE + (u32)slot * SLOT_SIZE; }

void sram_init(void) {
    u8 ok;
    SRAM_enable();
    ok = SRAM_readByte(0) == 'E' && SRAM_readByte(1) == 'L' && SRAM_readByte(2) == 'F'
      && SRAM_readByte(3) == '2'
      && SRAM_readByte(4) == (u8)(SRAM_SAVE_VERSION >> 8)
      && SRAM_readByte(5) == (u8)SRAM_SAVE_VERSION;
    if (!ok) {
        u8 s;
        SRAM_writeByte(0, 'E'); SRAM_writeByte(1, 'L');
        SRAM_writeByte(2, 'F'); SRAM_writeByte(3, '2');
        SRAM_writeByte(4, (u8)(SRAM_SAVE_VERSION >> 8));
        SRAM_writeByte(5, (u8)SRAM_SAVE_VERSION);
        for (s = 0u; s < SAVE_SLOTS; s++) {      /* invalida as gravacoes */
            SRAM_writeByte(slot_addr(s) + 2UL, 0);
            SRAM_writeByte(slot_addr(s) + 3UL, 0);
        }
    }
    SRAM_disable();
}

/* ------------------------------------------------------------------ */
static void write_state(const char *name) {
    u8 t, i, j, d, m, k;
    u16 p;
    char nm[SAVE_NAME_LEN];
    for (i = 0u; i < SAVE_NAME_LEN; i++) nm[i] = 0;
    data_strcpy(nm, name, SAVE_NAME_LEN);
    wbuf(nm, SAVE_NAME_LEN);
    w8(g_season); w8(g_slot); w8(g_mgr_count);
    for (m = 0u; m < MAX_MANAGERS; m++) { wbuf(g_mgr[m].name, MGR_NAME_LEN); w8(g_mgr[m].team); }
    for (t = 0u; t < (u8)TEAM_COUNT; t++) {
        Team *tm = &g_teams[t];
        w8(tm->div); w8(tm->formation); w8(tm->coach); w32(tm->money);
        w8(tm->hw); w8(tm->hd); w8(tm->hl); w8(tm->hgf); w8(tm->hga);
        w8(tm->aw); w8(tm->ad); w8(tm->al); w8(tm->agf); w8(tm->aga);
        w8(tm->nstands); wbuf(tm->stand_life, STANDS_MAX);
        w16(tm->socios); w16(tm->ticket);
        w8(tm->cup_alive); w8(tm->cup_reach); w8(tm->boost);
    }
    for (p = 0u; p < PLAYER_COUNT; p++) {
        Player *pl = &g_players[p];
        w8(pl->team); w8(pl->base); w8(pl->str); w8(pl->flags); w8(pl->goals); w16(pl->salary);
    }
    for (d = 0u; d < DIVISIONS; d++) {
        wbuf(g_div_team[d], TEAMS_PER_DIV);
        for (i = 0u; i < TEAMS_PER_DIV; i++) wbuf(g_res[d][i], TEAMS_PER_DIV);
    }
    w8(g_cup_n); w8(g_cup_phase); w8(g_cup_winner);
    for (i = 0u; i < 16u; i++) wbuf(&g_cup[i], sizeof(CupTie));
    w8(g_transfer_n);
    for (i = 0u; i < TRANSFER_HIST; i++) {
        TransferRec *r = &g_transfers[i];
        w16(r->player); w8(r->from); w8(r->to); w8(r->str); w16(r->salary);
    }
    for (m = 0u; m < MAX_MANAGERS; m++)
        for (k = 0u; k < 2u; k++) {
            w8(g_rev_n[m][k]);
            for (j = 0u; j < REVENUE_HIST; j++) {
                RevenueRec *r = &g_rev[m][k][j];
                w8(r->opp); w8(r->gh); w8(r->ga);
                w16(r->tickets); w16(r->spectators); w32(r->revenue);
            }
        }
    for (t = 0u; t < (u8)TEAM_COUNT; t++) wbuf(&g_honours[t], sizeof(TeamHonours));
    w8(g_winners_n);
    for (i = 0u; i < WINNERS_MAX; i++) wbuf(&g_winners[i], sizeof(WinnerRec));
    for (m = 0u; m < MAX_MANAGERS; m++) wbuf(g_hist_mgr_name[m], MGR_NAME_LEN);
    w8(g_awards_valid);
    for (d = 0u; d < DIVISIONS; d++) {
        DivAwards *a = &g_awards[d];
        w8(a->first); w8(a->second); w8(a->third);
        w8(a->attack); w8(a->attack_goals); w8(a->defence); w8(a->defence_goals);
        w8(a->scorer_goals); w16(a->scorer);
    }
}

static void read_state(void) {
    u8 t, i, j, d, m, k;
    u16 p;
    char nm[SAVE_NAME_LEN];
    rbuf(nm, SAVE_NAME_LEN);
    g_season = r8(); g_slot = r8(); g_mgr_count = r8();
    if (g_mgr_count > MAX_MANAGERS) g_mgr_count = MAX_MANAGERS;
    for (m = 0u; m < MAX_MANAGERS; m++) { rbuf(g_mgr[m].name, MGR_NAME_LEN); g_mgr[m].team = r8(); }
    for (t = 0u; t < (u8)TEAM_COUNT; t++) {
        Team *tm = &g_teams[t];
        tm->div = r8(); tm->formation = r8(); tm->coach = r8(); tm->money = r32();
        tm->hw = r8(); tm->hd = r8(); tm->hl = r8(); tm->hgf = r8(); tm->hga = r8();
        tm->aw = r8(); tm->ad = r8(); tm->al = r8(); tm->agf = r8(); tm->aga = r8();
        tm->nstands = r8(); rbuf(tm->stand_life, STANDS_MAX);
        tm->socios = r16(); tm->ticket = r16();
        tm->cup_alive = r8(); tm->cup_reach = r8(); tm->boost = r8();
    }
    for (p = 0u; p < PLAYER_COUNT; p++) {
        Player *pl = &g_players[p];
        pl->team = r8(); pl->base = r8(); pl->str = r8(); pl->flags = r8(); pl->goals = r8();
        pl->salary = r16();
    }
    for (d = 0u; d < DIVISIONS; d++) {
        rbuf(g_div_team[d], TEAMS_PER_DIV);
        for (i = 0u; i < TEAMS_PER_DIV; i++) rbuf(g_res[d][i], TEAMS_PER_DIV);
    }
    g_cup_n = r8(); g_cup_phase = r8(); g_cup_winner = r8();
    for (i = 0u; i < 16u; i++) rbuf(&g_cup[i], sizeof(CupTie));
    g_transfer_n = r8();
    for (i = 0u; i < TRANSFER_HIST; i++) {
        TransferRec *r = &g_transfers[i];
        r->player = r16(); r->from = r8(); r->to = r8(); r->str = r8(); r->salary = r16();
    }
    for (m = 0u; m < MAX_MANAGERS; m++)
        for (k = 0u; k < 2u; k++) {
            g_rev_n[m][k] = r8();
            for (j = 0u; j < REVENUE_HIST; j++) {
                RevenueRec *r = &g_rev[m][k][j];
                r->opp = r8(); r->gh = r8(); r->ga = r8();
                r->tickets = r16(); r->spectators = r16(); r->revenue = r32();
            }
        }
    for (t = 0u; t < (u8)TEAM_COUNT; t++) rbuf(&g_honours[t], sizeof(TeamHonours));
    g_winners_n = r8();
    for (i = 0u; i < WINNERS_MAX; i++) rbuf(&g_winners[i], sizeof(WinnerRec));
    for (m = 0u; m < MAX_MANAGERS; m++) rbuf(g_hist_mgr_name[m], MGR_NAME_LEN);
    g_awards_valid = r8();
    for (d = 0u; d < DIVISIONS; d++) {
        DivAwards *a = &g_awards[d];
        a->first = r8(); a->second = r8(); a->third = r8();
        a->attack = r8(); a->attack_goals = r8(); a->defence = r8(); a->defence_goals = r8();
        a->scorer_goals = r8(); a->scorer = r16();
    }
    for (t = 0u; t < (u8)TEAM_COUNT; t++) g_teams[t].manager = data_team_mgr(t);
    data_rosters_changed();
    g_cur_mgr = 0u;
}

/* ------------------------------------------------------------------ */
u8 sram_save(u8 slot, const char *name) {
    u32 base;
    u16 len;
    if (slot >= SAVE_SLOTS) return 0u;
    base = slot_addr(slot);
    SRAM_enable();
    s_off = base + SLOT_DATA;
    s_crc = 0xFFFFu;
    write_state(name);
    len = (u16)(s_off - base - SLOT_DATA);
    SRAM_writeByte(base + 0UL, (u8)(s_crc >> 8));
    SRAM_writeByte(base + 1UL, (u8)s_crc);
    SRAM_writeByte(base + 2UL, (u8)(len >> 8));
    SRAM_writeByte(base + 3UL, (u8)len);
    SRAM_disable();
    return 1u;
}

u8 sram_slot_valid(u8 slot) {
    u32 base;
    u16 len, stored, crc = 0xFFFFu, i;
    if (slot >= SAVE_SLOTS) return 0u;
    base = slot_addr(slot);
    SRAM_enable();
    stored = (u16)(((u16)SRAM_readByte(base) << 8) | SRAM_readByte(base + 1UL));
    len    = (u16)(((u16)SRAM_readByte(base + 2UL) << 8) | SRAM_readByte(base + 3UL));
    if (len == 0u || len > (u16)(SLOT_SIZE - SLOT_DATA)) { SRAM_disable(); return 0u; }
    for (i = 0u; i < len; i++) crc = crc_step(crc, SRAM_readByte(base + SLOT_DATA + i));
    SRAM_disable();
    return crc == stored;
}

u8 sram_load(u8 slot) {
    if (!sram_slot_valid(slot)) return 0u;
    SRAM_enable();
    s_off = slot_addr(slot) + SLOT_DATA;
    read_state();
    SRAM_disable();
    return 1u;
}

u8 sram_slot_info(u8 slot, char *name, u8 *season) {
    u32 a;
    u8 i;
    if (!sram_slot_valid(slot)) return 0u;
    a = slot_addr(slot) + SLOT_DATA;
    SRAM_enable();
    for (i = 0u; i < SAVE_NAME_LEN; i++) name[i] = (char)SRAM_readByte(a + i);
    name[SAVE_NAME_LEN - 1u] = '\0';
    *season = SRAM_readByte(a + SAVE_NAME_LEN);
    SRAM_disable();
    return 1u;
}
