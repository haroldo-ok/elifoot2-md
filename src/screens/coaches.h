#ifndef ELIFOOT_COACHES_H
#define ELIFOOT_COACHES_H

#include <genesis.h>

/*
 * screens/coaches.h — Gestão de treinadores
 *
 * Tela "TREINADORES EM JOGO" (fiel ao original, offset 0x1e443).
 * Permite:
 *   [A] Ver treinador actual da equipa do jogador
 *   [B] Contratar novo treinador (lista dos 50)
 *   [Start] Voltar
 *
 * Ao mudar treinador: "chicotada psicológica" — boost de força
 * temporário de +5 a todos os titulares por 3 jornadas.
 * String literal: "[nome] foi despedido do [clube]"
 *                 "Para o seu lugar foi escolhido [novo]"
 */

/* Boost temporário aplicado ao mudar treinador (jornadas) */
#define COACH_BOOST_ROUNDS  3u
#define COACH_BOOST_STRENGTH 5u

/* Estado global do boost (declarado em coaches.c, usado em match.c) */
extern u8 g_coach_boost_team;    /* equipa com boost activo, 0xFF = nenhuma */
extern u8 g_coach_boost_rounds;  /* jornadas restantes de boost */

void screen_coaches(void);

/* Chamada por season_run() no início de cada jornada para decrementar boost */
void coaches_tick_boost(void);

#endif /* ELIFOOT_COACHES_H */
