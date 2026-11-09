#ifndef ELIFOOT_SEASON_H
#define ELIFOOT_SEASON_H

#include <genesis.h>
#include "types.h"

/*
 * game/season.h -- Calendario da epoca, campeonato e taca (sem UI)
 *
 * Uma epoca tem 24 jornadas: 14 do campeonato e 10 maos da taca, pela
 * ordem L1 L2 T1 L3 T2 ... L11 T10 L12 L13 L14 (a 1a mao da taca veio
 * depois da 2a jornada no original; o resto e inferido).
 */

#define SLOT_LEAGUE 0u
#define SLOT_CUP    1u

u8 season_slot_kind(u8 slot);
u8 season_slot_index(u8 slot);       /* jornada 0..13 ou mao 0..9 */
/* Fase e mao da taca de uma mao 0..9 */
#define CUP_PHASE_OF(leg) ((u8)((leg) / 2u))
#define CUP_LEG_OF(leg)   ((u8)((leg) % 2u))

/* Jogo k (0..3) da jornada r (0..13) na divisao d: posicoes em g_div_team. */
void league_pair(u8 r, u8 k, u8 *home, u8 *away);
/* Jogo de t na jornada r: adversario e se joga em casa. */
u8   league_opponent(u8 t, u8 r, u8 *home);

void cup_draw(void);                 /* sorteia a fase actual com as equipas vivas */
u8   cup_tie_of(u8 t);               /* indice do confronto de t ou 0xFF */

/* Proximo jogo de t a partir do slot actual: adversario, casa?, slot. */
u8   season_next_match(u8 t, u8 *home, u8 *slot);

void season_start(void);             /* nova epoca: tabelas a zero, sorteio da taca */
/* Prepara g_live para o slot actual (IA escolhe tacticas, publico). */
void season_setup_games(void);
/* 2a mao da taca: o jogo i precisa de prolongamento? */
u8   season_cup_tied(u8 i);
/* Regista resultados, receitas; decide a taca na 2a mao. */
void season_apply_results(void);
/* Ordenados, forma, estadio, socios; avanca g_slot. */
void season_end_slot(void);
/* Fecho da epoca: premios e historico. */
void season_close(void);
/* Promocoes/despromocoes e nova epoca. Preenche moved[t]: -1 desce, +1 sobe. */
void season_promote(void);

#endif
