#ifndef ELIFOOT_DATA_H
#define ELIFOOT_DATA_H

/*
 * game/data.h -- Arrays globais de estado do jogo e fun??es de carga
 *
 * data_init() l? os bin?rios de ROM (teams_data, coaches_data gerados
 * pelo rescomp a partir de teams.bin / coaches.bin) e preenche os
 * arrays g_players[], g_teams[], g_coaches[] em RAM.
 *
 * Or?amento de RAM:
 *   g_players:  464 ? sizeof(Player)    ? 13 KB
 *   g_teams:     29 ? sizeof(Team)      ?  2 KB
 *   g_coaches:   50 ? 20 bytes          ?  1 KB
 *   g_results:  200 ? sizeof(MatchResult) ? 1.2 KB
 *   g_cup_ties:  16 ? sizeof(CupTie)   ?  128 bytes
 *   Total estimado: ~17 KB de 64 KB dispon?veis
 */

#include <genesis.h>
#include "types.h"

/* ------------------------------------------------------------------ */
/* Arrays globais em RAM                                               */
/* ------------------------------------------------------------------ */

extern Player      g_players[TEAM_COUNT * PLAYERS_PER_TEAM];
extern Team        g_teams[TEAM_COUNT];
extern char        g_coaches[COACH_COUNT][COACH_NAME_LEN];
extern MatchResult g_results[200];
extern CupTie      g_cup_ties[16];
extern SeasonRecord g_palmares[PALMARES_COUNT];

/* ------------------------------------------------------------------ */
/* Estado de temporada global                                          */
/* ------------------------------------------------------------------ */

/* Estas vari?veis s?o declaradas extern em main.c e usadas em        */
/* render_status_bar(). Definidas aqui para centralizar o estado.     */
extern u8   g_season_num;        /* n?mero da temporada atual (1-99)  */
extern u8   g_round;             /* jornada atual (1-N)               */
extern u8   g_player_team_idx;   /* ?ndice da equipe do jogador        */
extern u8   g_division;          /* divis?o da equipe do jogador       */
extern long g_money;             /* dinheiro da equipe do jogador      */

/* N?mero de resultados armazenados em g_results[] esta rodada.       */
extern u8   g_results_count;

/* Gols marcados por jogador na temporada corrente.
 * Indice = indice global em g_players[]. */
extern u16  g_goals[TEAM_COUNT * PLAYERS_PER_TEAM];

/* N?mero de confrontos de copa activos em g_cup_ties[].              */
extern u8   g_cup_ties_count;

/* Fase actual da copa (0=eliminat?rias, 1=quartos, 2=meias, 3=final).*/
extern u8   g_cup_phase;

/* ------------------------------------------------------------------ */
/* API p?blica                                                         */
/* ------------------------------------------------------------------ */

/*
 * data_init() -- Carrega dados de ROM para RAM e inicializa estado.
 * Deve ser chamado ap?s render_init() e rng_init(), antes de qualquer
 * fun??o de jogo ou tela.
 *
 * Responsabilidades:
 *   1. L? teams_data[] (ROM) -> g_teams[] e g_players[] (RAM).
 *   2. L? coaches_data[] (ROM) -> g_coaches[] (RAM).
 *   3. Calcula strength inicial de cada jogador (baseado em n2 da equipe).
 *   4. Calcula salary inicial de cada jogador.
 *   5. Atribui forma??o padr?o (4-4-2) a todas as equipes.
 *   6. Inicializa estado de temporada (g_season_num=1, g_round=0).
 */
void data_init(void);

/*
 * data_assign_division() -- Distribui as 29 equipes nas divis?es com
 * base nos valores n1/n2 e no estado actual de promo??o/rebaixamento.
 * Chamado no in?cio de cada temporada por season_run().
 */
void data_assign_divisions(void);

/*
 * data_reset_season_stats() -- Zera wins/draws/losses/goals de todas
 * as equipes. Chamado no in?cio de cada temporada.
 */
void data_reset_season_stats(void);

/*
 * data_get_team_player_count() -- Retorna n?mero de jogadores no plantel
 * da equipe team_idx (inclui titulares e suplentes).
 */
u8 data_get_team_player_count(u8 team_idx);

/*
 * data_get_team_gk_count() -- Retorna n?mero de guarda-redes no plantel.
 * Necess?rio para validar venda de jogadores (m?nimo 1 GR).
 */
u8 data_get_team_gk_count(u8 team_idx);

/*
 * data_sort_standings() -- Ordena as equipes de uma divis?o por pontos,
 * depois saldo de gols, depois gols marcados. Resultado em out_order[]
 * (array de ?ndices em g_teams[]). div=0..3.
 * N?o chama SYS_doVBlankProcess() -- seguro para chamar em qualquer contexto.
 */
void data_sort_standings(u8 div, u8 *out_order, u8 *out_count);

#endif /* ELIFOOT_DATA_H */
