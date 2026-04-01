#ifndef ELIFOOT_CUP_H
#define ELIFOOT_CUP_H

/*
 * game/cup.h — Sistema de Copa (eliminatórias em 2 mãos)
 *
 * Fases (fiel às strings literais do executável):
 *   Fase 0: 1ª ELIMINATORIA  (16 equipas → 8 vencedores)
 *   Fase 1: 2ª ELIMINATORIA  (8 → 4)
 *   Fase 2: QUARTOS DE FINAL (4 → 2)
 *   Fase 3: MEIAS FINAIS     (2 → 1 finalista cada)
 *   Fase 4: FINAL DA TACA
 *
 * Cada fase tem 1ª mão e 2ª mão. O vencedor agrega mais gols.
 * Em empate agregado: a equipa com mais gols fora avança.
 * Se persistir empate: série de penáltis (simulada).
 *
 * As equipas participantes são as 16 primeiras da Div1 mais
 * as que têm n1 > 0 na Div2 (até completar 16 participantes).
 * O campo n1 de cada equipa define o pote de sorteio.
 *
 * Layout em g_cup_ties[] (max 16 confrontos activos):
 *   Fase 0: até 8 confrontos
 *   Fase 1: até 4
 *   Fase 2: até 2
 *   Fase 3: até 1 (para cada meia-final)
 *   Fase 4: 1 final
 */

#include <genesis.h>
#include "types.h"

/* Nome da fase para display (fiel ao original) */
const char *cup_phase_name(u8 phase);

/*
 * cup_draw() — Sorteio dos confrontos da 1ª eliminatória.
 * Preenche g_cup_ties[] com os 8 pares de equipas.
 * Usa n1 de cada equipa como pote de sorteio:
 *   pote 0 (n1=0)  vs  pote 1 (n1>0)
 * Retorna o número de confrontos criados.
 */
u8 cup_draw(void);

/*
 * cup_simulate_leg() — Simula uma mão de todos os confrontos activos.
 * leg=0 → 1ª mão, leg=1 → 2ª mão.
 * Chama match_simulate() para cada confronto.
 * NÃO chama SYS_doVBlankProcess().
 */
void cup_simulate_leg(u8 leg);

/*
 * cup_advance() — Determina os vencedores de cada confronto (após 2ª mão)
 * e preenche g_cup_ties[] com os confrontos da fase seguinte.
 * Actualiza g_cup_phase e g_cup_ties_count.
 * Retorna o índice da equipa campeã se a final acabou, 0xFF caso contrário.
 */
u8 cup_advance(void);

/*
 * cup_team_active() — Retorna 1 se a equipa team_idx ainda está na copa.
 */
u8 cup_team_active(u8 team_idx);

#endif /* ELIFOOT_CUP_H */
