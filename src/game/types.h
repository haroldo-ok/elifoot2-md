#ifndef ELIFOOT_TYPES_H
#define ELIFOOT_TYPES_H

/*
 * game/types.h -- Constantes e estruturas do Elifoot II (Mega Drive)
 *
 * Valores confirmados a correr o executavel original (ver
 * documentation/original_observado.md): 32 equipas, 4 divisoes de 8,
 * 14 jornadas, vitoria = 2 pontos, taca de 32 em 5 fases de 2 maos.
 *
 * m68k: int = 32 bits com o gcc do SGDK, mas o codigo usa u8/u16/long
 * explicitos. Structs com campos u16/long em offsets pares.
 */

#include <genesis.h>

#define TEAM_COUNT        32u
#define DIVISIONS          4u
#define TEAMS_PER_DIV      8u
#define PLAYERS_PER_TEAM  16u              /* plantel inicial no ficheiro */
#define PLAYER_COUNT     512u              /* TEAM_COUNT * 16             */
#define COACH_COUNT       50u
#define MAX_MANAGERS       4u

#define TEAM_NAME_LEN     16u              /* inclui NUL */
#define PLAYER_NAME_LEN   16u
#define COACH_NAME_LEN    20u
#define MGR_NAME_LEN      13u

#define LEAGUE_ROUNDS     14u              /* ida e volta entre 8 equipas */
#define CUP_PHASES         5u
#define CUP_LEGS          10u
#define SEASON_SLOTS      24u              /* 14 jornadas + 10 maos da taca */
#define PROMOTED           2u              /* sobem/descem por divisao (inferido) */

#define WIN_POINTS         2u
#define START_MONEY   320000L
#define TICKET_PRICE      10u
#define STAND_SEATS     5000u
#define STAND_PRICE   100000L
#define STAND_LIFE        48u              /* jornadas de vida de uma bancada nova (inferido) */
#define STANDS_MAX        16u
#define MIN_SQUAD         14u
#define MAX_SQUAD         22u
#define MAX_SUBS           3u

#define TEAM_NONE       0xFFu
#define PLAYER_NONE   0xFFFFu
#define COACH_NONE      0xFFu

/* Posicoes (codificacao de teams.bin) */
#define POS_GR 0u
#define POS_DF 1u
#define POS_MD 2u
#define POS_AV 3u

/* Player.flags */
#define PF_FIELD   0x01u    /* titular                               */
#define PF_RAISED  0x02u    /* teve aumento esta epoca ('*')          */
#define PF_SUBBED  0x04u    /* ja foi substituido neste jogo          */

/* Formacoes F1..F10 (confirmadas no binario) */
#define FORM_COUNT 10u

typedef struct {
    char name[PLAYER_NAME_LEN];   /*  0 */
    char nac[3];                  /* 16 */
    u8   team;                    /* 19: TEAM_NONE = livre             */
    u8   pos;                     /* 20 */
    u8   base;                    /* 21: forca de base                 */
    u8   str;                     /* 22: forca actual (forma)          */
    u8   flags;                   /* 23 */
    u8   goals;                   /* 24: golos na epoca                */
    u8   _pad;                    /* 25 */
    u16  salary;                  /* 26: ordenado por jornada          */
} Player;                         /* 28 bytes */

typedef struct {
    char name[TEAM_NAME_LEN];     /*  0 */
    u8   fg, bg;                  /* 16: cores do clube (N1, N2)        */
    u8   div;                     /* 18: 0..3                           */
    u8   formation;               /* 19 */
    u8   coach;                   /* 20: treinador da IA, COACH_NONE    */
    u8   manager;                 /* 21: treinador humano, 0xFF = IA    */
    /* campeonato: casa e fora */
    u8   hw, hd, hl, hgf, hga;    /* 22 */
    u8   aw, ad, al, agf, aga;    /* 27 */
    u8   nstands;                 /* 32 */
    u8   cup_alive;               /* 33 */
    u8   cup_reach;               /* 34: fase onde saiu (0..4), 5 = venceu */
    u8   boost;                   /* 35: jornadas de chicotada restantes */
    u8   stand_life[STANDS_MAX];  /* 36 */
    u16  socios;                  /* 52 */
    u16  ticket;                  /* 54 */
    long money;                   /* 56 */
} Team;                           /* 60 bytes */

typedef struct {
    char name[MGR_NAME_LEN];
    u8   team;
} Manager;

/* Confronto da taca. a joga em casa na 1a mao. */
typedef struct {
    u8 a, b;
    u8 ga1, gb1;      /* 1a mao (a em casa)            */
    u8 ga2, gb2;      /* 2a mao (b em casa)            */
    u8 pa, pb;        /* penaltis (se houve)           */
    u8 winner;        /* TEAM_NONE ate decidir          */
    u8 extra;         /* 1 se houve prolongamento       */
} CupTie;

#define TEAM_W(t)   ((u8)((t)->hw + (t)->aw))
#define TEAM_D(t)   ((u8)((t)->hd + (t)->ad))
#define TEAM_L(t)   ((u8)((t)->hl + (t)->al))
#define TEAM_GF(t)  ((u8)((t)->hgf + (t)->agf))
#define TEAM_GA(t)  ((u8)((t)->hga + (t)->aga))
#define TEAM_J(t)   ((u8)(TEAM_W(t) + TEAM_D(t) + TEAM_L(t)))
#define TEAM_PTS(t) ((u16)((u16)TEAM_W(t) * WIN_POINTS + (u16)TEAM_D(t)))

#endif /* ELIFOOT_TYPES_H */
