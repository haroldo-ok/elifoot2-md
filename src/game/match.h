#ifndef ELIFOOT_MATCH_H
#define ELIFOOT_MATCH_H

#include <genesis.h>
#include "types.h"

/*
 * game/match.h -- Simula??o de partidas
 *
 * match_simulate() ? a fun??o central do jogo. Calcula a for?a m?dia
 * dos 11 titulares de cada equipe, aplica b?nus de casa (+5 pontos de
 * for?a), e distribui gols aleatoriamente com probabilidade proporcional
 * ? for?a relativa. Nunca chama SYS_doVBlankProcess().
 */

/*
 * match_simulate() -- Simula uma partida entre home e away.
 * Atualiza g_teams[] (wins/draws/losses/goals) internamente.
 * Retorna o resultado para exibi??o.
 */
MatchResult match_simulate(u8 home, u8 away);

/*
 * match_team_strength() -- For?a m?dia dos titulares da equipe.
 * P?blico para uso em screens (ex: mostrar for?a na tela de escala??o).
 */
u16 match_team_strength(u8 team_idx);

#endif /* ELIFOOT_MATCH_H */
