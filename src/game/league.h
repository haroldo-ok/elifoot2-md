#ifndef ELIFOOT_LEAGUE_H
#define ELIFOOT_LEAGUE_H

#include <genesis.h>
#include "types.h"

/*
 * game/league.h -- Campeonato: calendariza??o de jogos e classifica??o
 *
 * O campeonato usa um sistema de todos-contra-todos (round-robin) dentro
 * de cada divis?o. Com 16 equipas na Div1, s?o 15 jornadas de 8 jogos
 * cada (ida) ou 30 jornadas (ida e volta). Para simplificar a RAM,
 * implementamos apenas a volta simples (15 jornadas).
 *
 * Com 13 equipas na Div2, s?o 13 jornadas (algoritmo de round-robin
 * com bye para n?mero ?mpar de equipas).
 */

#define LEAGUE_MAX_GAMES_PER_ROUND  8u   /* Div1: 16/2=8 jogos/jornada */
#define LEAGUE_ROUNDS_DIV1         15u   /* 16 equipas, volta simples  */
#define LEAGUE_ROUNDS_DIV2         13u   /* 13 equipas, volta simples  */

/*
 * LeagueRound -- jogos de uma jornada de uma divis?o.
 * home[i] e away[i] s?o ?ndices em g_teams[].
 * count = n?mero de jogos (at? 8).
 */
typedef struct {
    u8 home[LEAGUE_MAX_GAMES_PER_ROUND];
    u8 away[LEAGUE_MAX_GAMES_PER_ROUND];
    u8 count;
} LeagueRound;

/*
 * league_build_round() -- Constr?i os pares de jogos para a jornada
 * round_num (1-based) da divis?o div, usando algoritmo de round-robin
 * de Berger. Preenche a estrutura round_out.
 * N?O chama SYS_doVBlankProcess().
 */
void league_build_round(u8 div, u8 round_num, LeagueRound *round_out);

/*
 * league_simulate_round() -- Simula todos os jogos de uma jornada.
 * Para cada jogo: chama match_simulate(), depois economy_ticket_revenue().
 * N?O chama SYS_doVBlankProcess() -- ? seguro chamar em loop.
 */
void league_simulate_round(const LeagueRound *round);

/*
 * league_total_rounds() -- N?mero total de jornadas para a divis?o div.
 */
u8 league_total_rounds(u8 div);

#endif /* ELIFOOT_LEAGUE_H */
