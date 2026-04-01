/*
 * engine/sram_io.c — Save/Load em SRAM para Elifoot II Genesis
 *
 * SGDK 1.70 caveats:
 *   - SRAM_readBuffer / SRAM_writeBuffer NÃO existem em SGDK 1.70.
 *     Implementar como loop de SRAM_readByte / SRAM_writeByte.
 *   - SRAM_enable() ANTES e SRAM_disable() DEPOIS de qualquer acesso.
 *     Em hardware real, SRAM_disable() é obrigatório para proteger os dados.
 *   - u32 para o offset em SRAM_readByte/writeByte (endereço no espaço
 *     de memória do Mega Drive, não apenas posição no banco).
 *   - int = 16 bits: contadores de loop usam u16, não int.
 *
 * Serialização big-endian (m68k native) para todos os campos multi-byte.
 */

#include <genesis.h>
#include "sram_io.h"
#include "../game/data.h"
#include "../game/types.h"

/* ------------------------------------------------------------------ */
/* Constantes de layout SRAM                                           */
/* ------------------------------------------------------------------ */

/* Use constants from types.h: SRAM_MAGIC_OFFSET, SRAM_VERSION_OFFSET */
#define SRAM_VERSION_OFFSET 0x0004UL
#define SRAM_SLOT0_OFFSET   SRAM_SLOT_BASE

/* Tamanho do conteúdo serializado de um slot (sem os 2 bytes de CRC) */
#define SLOT_CONTENT_SIZE   1822u

/* Tamanho total do buffer de um slot (CRC + conteúdo)                */
#define SLOT_BUFFER_SIZE    1824u

/* Layout por equipa no slot: 14 bytes                                */
#define TEAM_SAVE_SIZE      14u

/* Layout por jogador no slot: 3 bytes                                */
#define PLAYER_SAVE_SIZE    3u

/* Layout por entrada de palmares: 4 bytes                            */
#define PALMARES_SAVE_SIZE  4u

/* ------------------------------------------------------------------ */
/* I/O de baixo nível                                                  */
/* ------------------------------------------------------------------ */

static void sram_write_buf(u32 offset, const u8 *buf, u16 len) {
    u16 i;
    SRAM_enable();
    for (i = 0u; i < len; i++) {
        SRAM_writeByte(offset + (u32)i, buf[i]);
    }
    SRAM_disable();
}

static void sram_read_buf(u32 offset, u8 *buf, u16 len) {
    u16 i;
    SRAM_enable();
    for (i = 0u; i < len; i++) {
        buf[i] = SRAM_readByte(offset + (u32)i);
    }
    SRAM_disable();
}

/* ------------------------------------------------------------------ */
/* CRC-16/CCITT (polinómio 0x1021)                                    */
/* ------------------------------------------------------------------ */

static u16 crc16(const u8 *data, u16 len) {
    u16 crc = 0xFFFFu;
    u16 i;
    u8  b;
    for (i = 0u; i < len; i++) {
        crc ^= (u16)((u16)data[i] << 8);
        for (b = 0u; b < 8u; b++) {
            crc = (crc & 0x8000u) ? (u16)((crc << 1) ^ 0x1021u) : (u16)(crc << 1);
        }
    }
    return crc;
}

/* ------------------------------------------------------------------ */
/* Helpers de serialização (big-endian)                               */
/* ------------------------------------------------------------------ */

static u16 put_u8(u8 *buf, u16 pos, u8 val) {
    buf[pos] = val;
    return (u16)(pos + 1u);
}

static u16 put_u16(u8 *buf, u16 pos, u16 val) {
    buf[pos]     = (u8)(val >> 8);
    buf[pos + 1u] = (u8)(val & 0xFFu);
    return (u16)(pos + 2u);
}

static u16 put_long(u8 *buf, u16 pos, long val) {
    u32 v = (u32)val;
    buf[pos]     = (u8)(v >> 24);
    buf[pos + 1u] = (u8)(v >> 16);
    buf[pos + 2u] = (u8)(v >> 8);
    buf[pos + 3u] = (u8)(v & 0xFFu);
    return (u16)(pos + 4u);
}

static u16 get_u8(const u8 *buf, u16 pos, u8 *out) {
    *out = buf[pos];
    return (u16)(pos + 1u);
}

static u16 get_u16(const u8 *buf, u16 pos, u16 *out) {
    *out = ((u16)buf[pos] << 8) | (u16)buf[pos + 1u];
    return (u16)(pos + 2u);
}

static u16 get_long(const u8 *buf, u16 pos, long *out) {
    u32 v = ((u32)buf[pos]     << 24)
          | ((u32)buf[pos + 1u] << 16)
          | ((u32)buf[pos + 2u] << 8)
          |  (u32)buf[pos + 3u];
    *out = (long)v;
    return (u16)(pos + 4u);
}

/* ------------------------------------------------------------------ */
/* sram_init()                                                         */
/* ------------------------------------------------------------------ */

void sram_init(void) {
    u8  magic[4];
    u16 version;

    sram_read_buf(SRAM_MAGIC_OFFSET, magic, 4u);

    if (magic[0] == (u8)SRAM_MAGIC_0 &&
        magic[1] == (u8)SRAM_MAGIC_1 &&
        magic[2] == (u8)SRAM_MAGIC_2 &&
        magic[3] == (u8)SRAM_MAGIC_3) {
        /* Magic OK — verifica versão                                  */
        sram_read_buf(SRAM_VERSION_OFFSET, magic, 2u);
        version = ((u16)magic[0] << 8) | (u16)magic[1];
        if (version == (u16)SRAM_SAVE_VERSION) return;
    }

    /* SRAM virgem ou versão errada — inicializa cabeçalho             */
    {
        u8 hdr[6];
        hdr[0] = (u8)SRAM_MAGIC_0;
        hdr[1] = (u8)SRAM_MAGIC_1;
        hdr[2] = (u8)SRAM_MAGIC_2;
        hdr[3] = (u8)SRAM_MAGIC_3;
        hdr[4] = (u8)((u16)SRAM_SAVE_VERSION >> 8);
        hdr[5] = (u8)((u16)SRAM_SAVE_VERSION & 0xFFu);
        sram_write_buf(SRAM_MAGIC_OFFSET, hdr, 6u);
    }
}

/* ------------------------------------------------------------------ */
/* sram_save()                                                         */
/* ------------------------------------------------------------------ */

u8 sram_save(u8 slot) {
    /*
     * Buffer estático para o slot — 1824 bytes.
     * Estático para não consumir stack do m68k (stack é limitado).
     * ATENÇÃO: não é reentrante, mas save nunca é chamado de IRQ.
     */
    static u8 buf[SLOT_BUFFER_SIZE];
    u16  pos = 2u;   /* primeiros 2 bytes = CRC (preenchido no fim)    */
    u32  slot_offset;
    u16  crc;
    u8   t, p, i;

    if ((u16)slot >= (u16)SAVE_SLOT_COUNT) return 0u;
    slot_offset = SRAM_SLOT0_OFFSET + (u32)slot * (u32)SRAM_SLOT_SIZE;

    /* Cabeçalho do slot                                               */
    pos = put_u8(buf, pos, g_season_num);
    pos = put_u8(buf, pos, g_round);
    pos = put_u8(buf, pos, g_player_team_idx);
    pos = put_u8(buf, pos, g_cup_phase);

    /* 29 equipas × 14 bytes                                           */
    for (t = 0u; t < (u8)TEAM_COUNT; t++) {
        Team *team = &g_teams[t];
        pos = put_u8 (buf, pos, team->division);
        pos = put_long(buf, pos, team->money);
        pos = put_u8 (buf, pos, team->wins);
        pos = put_u8 (buf, pos, team->draws);
        pos = put_u8 (buf, pos, team->losses);
        pos = put_u8 (buf, pos, team->goals_for);
        pos = put_u8 (buf, pos, team->goals_against);
        pos = put_u16(buf, pos, team->points);
        pos = put_u16(buf, pos, team->stadium_cap);
    }

    /* 464 jogadores × 3 bytes: strength + salary_lo + salary_hi      */
    /* Salary comprimido para 16 bits (max 65535 escudos)              */
    for (p = 0u; p < (u16)(TEAM_COUNT * PLAYERS_PER_TEAM); p++) {
        Player *pl  = &g_players[p];
        u16    sal16 = (pl->salary > 65535L) ? 65535u : (u16)pl->salary;
        pos = put_u8 (buf, pos, pl->strength);
        pos = put_u16(buf, pos, sal16);
    }

    /* Palmares × 4 bytes                                              */
    for (i = 0u; i < (u8)PALMARES_COUNT; i++) {
        pos = put_u8(buf, pos, g_palmares[i].season_num);
        pos = put_u8(buf, pos, g_palmares[i].div1_champion);
        pos = put_u8(buf, pos, g_palmares[i].cup_champion);
        pos = put_u8(buf, pos, g_palmares[i].player_pos);
    }

    /* CRC sobre os bytes 2..pos-1                                     */
    crc    = crc16(buf + 2u, (u16)(pos - 2u));
    buf[0] = (u8)(crc >> 8);
    buf[1] = (u8)(crc & 0xFFu);

    sram_write_buf(slot_offset, buf, pos);
    return 1u;
}

/* ------------------------------------------------------------------ */
/* sram_load()                                                         */
/* ------------------------------------------------------------------ */

u8 sram_load(u8 slot) {
    static u8 buf[SLOT_BUFFER_SIZE];
    u32  slot_offset;
    u16  stored_crc, calc_crc;
    u16  pos = 2u;
    u8   t, p, i;

    if ((u16)slot >= (u16)SAVE_SLOT_COUNT) return 0u;
    slot_offset = SRAM_SLOT0_OFFSET + (u32)slot * (u32)SRAM_SLOT_SIZE;

    sram_read_buf(slot_offset, buf, (u16)SLOT_BUFFER_SIZE);

    stored_crc = ((u16)buf[0] << 8) | (u16)buf[1];
    calc_crc   = crc16(buf + 2u, (u16)(SLOT_BUFFER_SIZE - 2u));
    if (stored_crc != calc_crc) return 0u;

    /* Cabeçalho                                                       */
    pos = get_u8(buf, pos, &g_season_num);
    pos = get_u8(buf, pos, &g_round);
    pos = get_u8(buf, pos, &g_player_team_idx);
    pos = get_u8(buf, pos, &g_cup_phase);

    /* 29 equipas                                                      */
    for (t = 0u; t < (u8)TEAM_COUNT; t++) {
        Team *team = &g_teams[t];
        pos = get_u8 (buf, pos, &team->division);
        pos = get_long(buf, pos, &team->money);
        pos = get_u8 (buf, pos, &team->wins);
        pos = get_u8 (buf, pos, &team->draws);
        pos = get_u8 (buf, pos, &team->losses);
        pos = get_u8 (buf, pos, &team->goals_for);
        pos = get_u8 (buf, pos, &team->goals_against);
        pos = get_u16(buf, pos, &team->points);
        pos = get_u16(buf, pos, &team->stadium_cap);
    }

    /* 464 jogadores                                                   */
    for (p = 0u; p < (u16)(TEAM_COUNT * PLAYERS_PER_TEAM); p++) {
        Player *pl = &g_players[p];
        u16     sal16;
        pos       = get_u8 (buf, pos, &pl->strength);
        pos       = get_u16(buf, pos, &sal16);
        pl->salary = (long)sal16;
    }

    /* Palmares                                                        */
    for (i = 0u; i < (u8)PALMARES_COUNT; i++) {
        pos = get_u8(buf, pos, &g_palmares[i].season_num);
        pos = get_u8(buf, pos, &g_palmares[i].div1_champion);
        pos = get_u8(buf, pos, &g_palmares[i].cup_champion);
        pos = get_u8(buf, pos, &g_palmares[i].player_pos);
    }

    /* Sincroniza estado global                                        */
    g_division = g_teams[g_player_team_idx].division;
    g_money    = g_teams[g_player_team_idx].money;
    return 1u;
}

/* ------------------------------------------------------------------ */
/* sram_slot_valid()                                                   */
/* ------------------------------------------------------------------ */

u8 sram_slot_valid(u8 slot) {
    static u8 buf[SLOT_BUFFER_SIZE];
    u32  slot_offset;
    u16  stored_crc, calc_crc;

    if ((u16)slot >= (u16)SAVE_SLOT_COUNT) return 0u;
    slot_offset = SRAM_SLOT0_OFFSET + (u32)slot * (u32)SRAM_SLOT_SIZE;
    sram_read_buf(slot_offset, buf, (u16)SLOT_BUFFER_SIZE);
    stored_crc = ((u16)buf[0] << 8) | (u16)buf[1];
    calc_crc   = crc16(buf + 2u, (u16)(SLOT_BUFFER_SIZE - 2u));
    return (stored_crc == calc_crc) ? 1u : 0u;
}

/* ------------------------------------------------------------------ */
/* sram_slot_season()                                                  */
/* ------------------------------------------------------------------ */

u8 sram_slot_season(u8 slot) {
    u8  hdr[3];
    u32 slot_offset;
    if ((u16)slot >= (u16)SAVE_SLOT_COUNT) return 0u;
    if (!sram_slot_valid(slot)) return 0u;
    slot_offset = SRAM_SLOT0_OFFSET + (u32)slot * (u32)SRAM_SLOT_SIZE;
    sram_read_buf(slot_offset + 2UL, hdr, 1u);   /* byte 2 = season_num */
    return hdr[0];
}
