#ifndef ELIFOOT_MATCH_H
#define ELIFOOT_MATCH_H

#include <genesis.h>
#include "types.h"

/*
 * game/match.h — Simulação de partidas
 *
 * match_simulate() é a função central do jogo. Calcula a força média
 * dos 11 titulares de cada equipe, aplica bónus de casa (+5 pontos de
 * força), e distribui gols aleatoriamente com probabilidade proporcional
 * à força relativa. Nunca chama SYS_doVBlankProcess().
 */

/*
 * match_simulate() — Simula uma partida entre home e away.
 * Atualiza g_teams[] (wins/draws/losses/goals) internamente.
 * Retorna o resultado para exibição.
 */
MatchResult match_simulate(u8 home, u8 away);

/*
 * match_team_strength() — Força média dos titulares da equipe.
 * Público para uso em screens (ex: mostrar força na tela de escalação).
 */
u16 match_team_strength(u8 team_idx);

#endif /* ELIFOOT_MATCH_H */
