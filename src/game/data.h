#ifndef ELIFOOT_DATA_H
#define ELIFOOT_DATA_H

#include <genesis.h>
#include "types.h"

/* ---- Estado global ------------------------------------------------- */
extern Player  g_players[PLAYER_COUNT];
extern Team    g_teams[TEAM_COUNT];
extern char    g_coaches[COACH_COUNT][COACH_NAME_LEN];
extern Manager g_mgr[MAX_MANAGERS];
extern u8      g_mgr_count;
extern u8      g_cur_mgr;          /* treinador humano a jogar agora   */

extern u8      g_season;           /* epoca (1..)                     */
extern u8      g_slot;             /* jornada da epoca (0..23)        */

/* Equipas de cada divisao (ordem fixa na epoca, usada no calendario). */
extern u8      g_div_team[DIVISIONS][TEAMS_PER_DIV];
/* Resultados [div][casa][fora] = golos casa<<4 | golos fora, 0xFF = por jogar.
 * Indices = posicao em g_div_team. */
extern u8      g_res[DIVISIONS][TEAMS_PER_DIV][TEAMS_PER_DIV];

/* Taca */
extern CupTie  g_cup[16];
extern u8      g_cup_n;            /* confrontos da fase actual       */
extern u8      g_cup_phase;        /* 0..4, 5 = acabou                */
extern u8      g_cup_winner;

/* Historico de transferencias (Ctrl+F4) */
typedef struct {
    u16 player;
    u8  from, to;
    u8  str, _pad;
    u16 salary;
} TransferRec;
#define TRANSFER_HIST 20u
extern TransferRec g_transfers[TRANSFER_HIST];
extern u8          g_transfer_n;

/* Ultimas receitas (Ctrl+F5) por equipa humana: [mgr][0=camp,1=taca] */
typedef struct {
    u8   opp, gh, ga, _pad;
    u16  tickets, spectators;
    long revenue;
} RevenueRec;
#define REVENUE_HIST 8u
extern RevenueRec g_rev[MAX_MANAGERS][2][REVENUE_HIST];
extern u8         g_rev_n[MAX_MANAGERS][2];

/* ---- Funcoes ------------------------------------------------------- */
void data_init(void);                  /* carrega ROM, estado inicial */
void data_new_season_tables(void);     /* zera estatisticas e g_res    */

/* Lista o plantel de t (ordenado por posicao). Devolve o numero. */
u8   data_squad(u8 t, u16 *out, u8 max);
u8   data_squad_count(u8 t);
/* Chamar sempre que um jogador muda de clube (invalida a cache). */
void data_rosters_changed(void);
u8   data_gk_count(u8 t);

/* Classificacao da divisao: out[8] indices de equipa, do 1o ao ultimo. */
void data_sort_div(u8 div, u8 *out);
u8   data_team_place(u8 t);            /* 1..8 */
u8   data_div_slot(u8 t);              /* indice de t em g_div_team    */

/* Treinador humano de t ou 0xFF */
u8   data_team_mgr(u8 t);
u8   data_is_human(u8 t);

/* Valores de mercado (observados: preco ~ forca*3000, minimo ~ forca*70) */
long data_price(u16 p);
u16  data_min_salary(u16 p);

long data_total_salaries(u8 t);
u16  data_capacity(u8 t);

void data_record_transfer(u16 p, u8 from, u8 to);
void data_record_revenue(u8 mgr, u8 kind, u8 opp, u8 gh, u8 ga,
                         u16 tickets, u16 spect, long rev);

/* Copia segura de string */
void data_strcpy(char *dst, const char *src, u8 cap);

#endif /* ELIFOOT_DATA_H */
