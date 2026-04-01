#ifndef ELIFOOT_DATA_H
#define ELIFOOT_DATA_H

/*
 * game/data.h — Arrays globais de estado do jogo e funções de carga
 *
 * data_init() lê os binários de ROM (teams_data, coaches_data gerados
 * pelo rescomp a partir de teams.bin / coaches.bin) e preenche os
 * arrays g_players[], g_teams[], g_coaches[] em RAM.
 *
 * Orçamento de RAM:
 *   g_players:  464 × sizeof(Player)    ≈ 13 KB
 *   g_teams:     29 × sizeof(Team)      ≈  2 KB
 *   g_coaches:   50 × 20 bytes          ≈  1 KB
 *   g_results:  200 × sizeof(MatchResult) ≈ 1.2 KB
 *   g_cup_ties:  16 × sizeof(CupTie)   ≈  128 bytes
 *   Total estimado: ~17 KB de 64 KB disponíveis
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

/* Estas variáveis são declaradas extern em main.c e usadas em        */
/* render_status_bar(). Definidas aqui para centralizar o estado.     */
extern u8   g_season_num;        /* número da temporada atual (1–99)  */
extern u8   g_round;             /* jornada atual (1–N)               */
extern u8   g_player_team_idx;   /* índice da equipe do jogador        */
extern u8   g_division;          /* divisão da equipe do jogador       */
extern long g_money;             /* dinheiro da equipe do jogador      */

/* Número de resultados armazenados em g_results[] esta rodada.       */
extern u8   g_results_count;

/* Número de confrontos de copa activos em g_cup_ties[].              */
extern u8   g_cup_ties_count;

/* Fase actual da copa (0=eliminatórias, 1=quartos, 2=meias, 3=final).*/
extern u8   g_cup_phase;

/* ------------------------------------------------------------------ */
/* API pública                                                         */
/* ------------------------------------------------------------------ */

/*
 * data_init() — Carrega dados de ROM para RAM e inicializa estado.
 * Deve ser chamado após render_init() e rng_init(), antes de qualquer
 * função de jogo ou tela.
 *
 * Responsabilidades:
 *   1. Lê teams_data[] (ROM) → g_teams[] e g_players[] (RAM).
 *   2. Lê coaches_data[] (ROM) → g_coaches[] (RAM).
 *   3. Calcula strength inicial de cada jogador (baseado em n2 da equipe).
 *   4. Calcula salary inicial de cada jogador.
 *   5. Atribui formação padrão (4-4-2) a todas as equipes.
 *   6. Inicializa estado de temporada (g_season_num=1, g_round=0).
 */
void data_init(void);

/*
 * data_assign_division() — Distribui as 29 equipes nas divisões com
 * base nos valores n1/n2 e no estado actual de promoção/rebaixamento.
 * Chamado no início de cada temporada por season_run().
 */
void data_assign_divisions(void);

/*
 * data_reset_season_stats() — Zera wins/draws/losses/goals de todas
 * as equipes. Chamado no início de cada temporada.
 */
void data_reset_season_stats(void);

/*
 * data_get_team_player_count() — Retorna número de jogadores no plantel
 * da equipe team_idx (inclui titulares e suplentes).
 */
u8 data_get_team_player_count(u8 team_idx);

/*
 * data_get_team_gk_count() — Retorna número de guarda-redes no plantel.
 * Necessário para validar venda de jogadores (mínimo 1 GR).
 */
u8 data_get_team_gk_count(u8 team_idx);

/*
 * data_sort_standings() — Ordena as equipes de uma divisão por pontos,
 * depois saldo de gols, depois gols marcados. Resultado em out_order[]
 * (array de índices em g_teams[]). div=0..3.
 * Não chama SYS_doVBlankProcess() — seguro para chamar em qualquer contexto.
 */
void data_sort_standings(u8 div, u8 *out_order, u8 *out_count);

#endif /* ELIFOOT_DATA_H */
