#ifndef ELIFOOT_TYPES_H
#define ELIFOOT_TYPES_H

/*
 * game/types.h — Tipos e estruturas do Elifoot II Genesis
 *
 * CRÍTICO — m68k / SGDK 1.70:
 *   int  = 16 bits  → NUNCA usar int para valores monetários ou contagens > 32767
 *   long = 32 bits  → usar para salários, receitas, transferências
 *   sizeof(int) = 2, sizeof(long) = 4, sizeof(void*) = 4
 *
 * As estruturas PlayerRecord / TeamRecord / CoachRecord definem o layout
 * BINÁRIO de teams.bin e coaches.bin gerados por pack_data.py.
 * Qualquer alteração aqui exige regenerar os binários.
 *
 * PlayerRecord e TeamRecord usam padding explícito (_pad) para garantir
 * alinhamento de word no m68k sem depender de padding implícito do GCC,
 * que varia com flags de compilação.
 */

#include <genesis.h>

/* ------------------------------------------------------------------ */
/* Constantes                                                          */
/* ------------------------------------------------------------------ */

#define TEAM_COUNT         29u
#define PLAYERS_PER_TEAM   16u
#define COACH_COUNT        50u

#define TEAM_NAME_LEN      20u   /* inclui NUL                         */
#define PLAYER_NAME_LEN    16u   /* inclui NUL                         */
#define COACH_NAME_LEN     20u   /* inclui NUL                         */

#define MAX_DIVISIONS       4u
#define TEAMS_PER_DIV_1    16u
#define TEAMS_PER_DIV_2    13u   /* 29 - 16 = 13 (simplificado)        */

#define MIN_SQUAD_SIZE     14u   /* mínimo de jogadores no plantel      */
#define MIN_GK_COUNT        1u   /* mínimo de guarda-redes no plantel   */

/* Slots de save em SRAM                                               */
#define SAVE_SLOT_COUNT     3u
#define SRAM_MAGIC_OFFSET   0x0000u
#define SRAM_MAGIC_0        'E'
#define SRAM_MAGIC_1        'L'
#define SRAM_MAGIC_2        'F'
#define SRAM_MAGIC_3        '2'
#define SRAM_VERSION        0x0001u
#define SRAM_SLOT_BASE      0x0006u
#define SRAM_SLOT_SIZE      0x0800u   /* 2048 bytes por slot            */

/* ------------------------------------------------------------------ */
/* Enumerações                                                         */
/* ------------------------------------------------------------------ */

/*
 * Position — posição de campo do jogador.
 * Valores correspondem ao encoding em teams.bin (0–3).
 */
typedef enum {
    POS_GR = 0,   /* Guarda-redes                                      */
    POS_DF = 1,   /* Defensor                                          */
    POS_MD = 2,   /* Médio                                             */
    POS_AV = 3,   /* Avançado                                          */
} Position;

/*
 * Formation — índice de formação tática (0–9).
 * O nome e o array de multiplicadores estão em game/data.c.
 */
typedef enum {
    FORM_4_4_2  = 0,
    FORM_4_3_3  = 1,
    FORM_3_5_2  = 2,
    FORM_5_3_2  = 3,
    FORM_4_5_1  = 4,
    FORM_3_4_3  = 5,
    FORM_5_4_1  = 6,
    FORM_4_2_4  = 7,
    FORM_3_3_4  = 8,
    FORM_5_2_3  = 9,
    FORM_COUNT  = 10,
} Formation;

/*
 * GameScreen — estados da máquina de telas.
 */
typedef enum {
    SCREEN_TITLE,
    SCREEN_MAIN_MENU,
    SCREEN_SQUAD,
    SCREEN_RESULTS,
    SCREEN_STANDINGS,
    SCREEN_CALENDAR,
    SCREEN_FINANCES_SALARIES,
    SCREEN_FINANCES_REVENUES,
    SCREEN_TRANSFERS,
    SCREEN_PALMARES,
    SCREEN_STADIUM,
    SCREEN_COACHES,
    SCREEN_SAVE,
    SCREEN_CUP_DRAW,
    SCREEN_SEASON_END,
} GameScreen;

/* ------------------------------------------------------------------ */
/* Estruturas binárias (layout de ROM — teams.bin / coaches.bin)      */
/* ------------------------------------------------------------------ */

/*
 * PlayerRecord — 24 bytes, layout fixo, big-endian.
 * Corresponde ao formato em pack_data.py → PLAYER_REC_FMT.
 */
typedef struct {
    char  name[PLAYER_NAME_LEN];  /* offset  0: NUL-terminated ASCII   */
    u8    pos;                    /* offset 16: Position enum (0–3)     */
    char  nac[3];                 /* offset 17: ex "POR" (sem NUL)      */
    u8    _pad[4];                /* offset 20: reservado, sempre 0     */
    /* sizeof = 24 bytes */
} PlayerRecord;

/*
 * TeamRecord — 26 + 16×24 = 410 bytes, layout fixo, big-endian.
 * Corresponde ao formato em pack_data.py → TEAM_REC_FMT.
 */
typedef struct {
    char          name[TEAM_NAME_LEN];           /* offset  0           */
    u8            n1;                            /* offset 20           */
    u8            n2;                            /* offset 21           */
    char          nac[3];                        /* offset 22           */
    u8            _pad;                          /* offset 25           */
    PlayerRecord  players[PLAYERS_PER_TEAM];     /* offset 26           */
    /* sizeof = 26 + 16×24 = 410 bytes */
} TeamRecord;

/*
 * CoachRecord — 20 bytes, layout fixo.
 */
typedef struct {
    char  name[COACH_NAME_LEN];   /* NUL-terminated ASCII              */
    /* sizeof = 20 bytes */
} CoachRecord;

/* ------------------------------------------------------------------ */
/* Estruturas de estado em RAM                                         */
/* ------------------------------------------------------------------ */

/*
 * Player — estado em runtime de um jogador (em RAM, não em ROM).
 * Carregado de TeamRecord.players[] e enriquecido com atributos
 * calculados em runtime (strength, salary, on_field).
 */
typedef struct {
    char  name[PLAYER_NAME_LEN];  /* nome transliterado                */
    u8    pos;                    /* Position                          */
    u8    nat;                    /* índice de nacionalidade           */
    u8    strength;               /* força 1–99 (calculado em data_init)*/
    u8    on_field;               /* 1=titular, 0=banco                */
    long  salary;                 /* salário mensal (32 bits)           */
} Player;

/*
 * Team — estado em runtime de uma equipe.
 */
typedef struct {
    char  name[TEAM_NAME_LEN];
    u8    n1;                 /* pote de sorteio da copa              */
    u8    n2;                 /* fator de orçamento/força inicial     */
    u8    division;           /* 0=Div1 … 3=Div4                      */
    u8    formation;          /* Formation enum (0–9)                  */
    u8    coach_idx;          /* índice em g_coaches[] (0–49)         */
    u8    player_start;       /* índice do 1º jogador em g_players[]  */
    u8    player_count;       /* tamanho do plantel (sempre 16 no ROM)*/

    /* Estatísticas da temporada                                       */
    u8    wins;
    u8    draws;
    u8    losses;
    u8    goals_for;
    u8    goals_against;
    u16   points;

    /* Finanças (32 bits — podem exceder 32767 facilmente)            */
    long  money;
    long  salary_total;

    /* Estádio                                                         */
    u16   stadium_cap;
    u16   ticket_price;

    /* Copa                                                            */
    u8    cup_active;    /* 1=ainda na copa desta temporada            */
} Team;

/*
 * MatchResult — resultado de uma partida.
 */
typedef struct {
    u8  home_team;
    u8  away_team;
    u8  home_goals;
    u8  away_goals;
    u8  round;
    u8  division;
} MatchResult;

/*
 * CupTie — confronto de copa em 2 mãos.
 */
typedef struct {
    u8  team_a;
    u8  team_b;
    u8  goals_a_leg1;
    u8  goals_b_leg1;
    u8  goals_a_leg2;
    u8  goals_b_leg2;
    u8  phase;     /* 0=eliminatórias, 1=quartos, 2=meias, 3=final    */
    u8  _pad;
} CupTie;

/*
 * SeasonRecord — registo de uma temporada para o palmares.
 */
typedef struct {
    u8  season_num;
    u8  div1_champion;   /* índice da equipe campeã da Div1            */
    u8  cup_champion;    /* índice da equipe campeã da copa            */
    u8  player_div;      /* divisão do jogador nessa temporada         */
    u8  player_pos;      /* posição final do jogador na classificação  */
    u8  _pad[3];
} SeasonRecord;

#define PALMARES_COUNT  5u   /* máximo de temporadas no palmares       */

/* ------------------------------------------------------------------ */
/* Macros utilitárias                                                  */
/* ------------------------------------------------------------------ */

/* Calcula pontos de classificação a partir de wins/draws.             */
#define TEAM_POINTS(w, d)  ((u16)((u16)(w) * 3u + (u16)(d)))

/* Saldo de gols.                                                      */
#define GOAL_DIFF(gf, ga)  ((s16)((s16)(gf) - (s16)(ga)))

#endif /* ELIFOOT_TYPES_H */
