#ifndef ELIFOOT_MATCH_H
#define ELIFOOT_MATCH_H

#include <genesis.h>
#include "types.h"

/*
 * game/match.h -- Jogos ao vivo (minuto a minuto)
 *
 * Uma jornada tem ate 16 jogos simultaneos (4 divisoes x 4, ou 16
 * eliminatorias). Cada minuto rola um golo para cada equipa com
 * probabilidade proporcional ao ataque proprio contra a defesa adversaria
 * (titulares actuais), por isso as substituicoes ao intervalo contam.
 */

typedef struct {
    u8  home, away;
    u8  hg, ag;           /* golos neste jogo (inclui prolongamento) */
    u8  last_team;        /* equipa do ultimo golo, TEAM_NONE         */
    u8  last_min;
    u16 last_player;
    u16 spectators;
    u16 thr_h, thr_a;     /* probabilidade de golo por minuto (/10000) */
    u8  extra;            /* joga prolongamento                       */
    u8  pens;             /* decidiu-se nos penaltis                  */
    u8  ph, pa;           /* penaltis marcados                        */
} LiveGame;

extern LiveGame g_live[16];
extern u8       g_live_n;
extern u8       g_live_cup;     /* 1 = jornada da taca */

/* Prepara o jogo i (calcula probabilidades com os titulares actuais). */
void match_setup(u8 i, u8 home, u8 away, u8 league);
void match_recompute(u8 i);
/* Simula o minuto m (1..120) do jogo i. Devolve 1 se houve golo. */
u8   match_minute(u8 i, u8 m, u8 league);

/* Serie de penaltis (5 + morte subita). Preenche ph/pa. Devolve vencedor.
 * cb != NULL recebe cada penalti (para mostrar): kick 0.., home?, golo? */
typedef void (*PenaltyCb)(u8 game, u8 kick, u8 is_home, u8 scored);
u8   match_penalties(u8 i, PenaltyCb cb);

#endif
