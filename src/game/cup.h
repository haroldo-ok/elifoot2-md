#ifndef ELIFOOT_CUP_H
#define ELIFOOT_CUP_H

/*
 * game/cup.h -- Sistema de Copa (eliminat?rias em 2 m?os)
 *
 * Fases (fiel ?s strings literais do execut?vel):
 *   Fase 0: 1? ELIMINATORIA  (16 equipas -> 8 vencedores)
 *   Fase 1: 2? ELIMINATORIA  (8 -> 4)
 *   Fase 2: QUARTOS DE FINAL (4 -> 2)
 *   Fase 3: MEIAS FINAIS     (2 -> 1 finalista cada)
 *   Fase 4: FINAL DA TACA
 *
 * Cada fase tem 1? m?o e 2? m?o. O vencedor agrega mais gols.
 * Em empate agregado: a equipa com mais gols fora avan?a.
 * Se persistir empate: s?rie de pen?ltis (simulada).
 *
 * As equipas participantes s?o as 16 primeiras da Div1 mais
 * as que t?m n1 > 0 na Div2 (at? completar 16 participantes).
 * O campo n1 de cada equipa define o pote de sorteio.
 *
 * Layout em g_cup_ties[] (max 16 confrontos activos):
 *   Fase 0: at? 8 confrontos
 *   Fase 1: at? 4
 *   Fase 2: at? 2
 *   Fase 3: at? 1 (para cada meia-final)
 *   Fase 4: 1 final
 */

#include <genesis.h>
#include "types.h"

/* Nome da fase para display (fiel ao original) */
const char *cup_phase_name(u8 phase);

/*
 * cup_draw() -- Sorteio dos confrontos da 1? eliminat?ria.
 * Preenche g_cup_ties[] com os 8 pares de equipas.
 * Usa n1 de cada equipa como pote de sorteio:
 *   pote 0 (n1=0)  vs  pote 1 (n1>0)
 * Retorna o n?mero de confrontos criados.
 */
u8 cup_draw(void);

/*
 * cup_simulate_leg() -- Simula uma m?o de todos os confrontos activos.
 * leg=0 -> 1? m?o, leg=1 -> 2? m?o.
 * Chama match_simulate() para cada confronto.
 * N?O chama SYS_doVBlankProcess().
 */
void cup_simulate_leg(u8 leg);

/*
 * cup_advance() -- Determina os vencedores de cada confronto (ap?s 2? m?o)
 * e preenche g_cup_ties[] com os confrontos da fase seguinte.
 * Actualiza g_cup_phase e g_cup_ties_count.
 * Retorna o ?ndice da equipa campe? se a final acabou, 0xFF caso contr?rio.
 */
u8 cup_advance(void);

/*
 * cup_team_active() -- Retorna 1 se a equipa team_idx ainda est? na copa.
 */
u8 cup_team_active(u8 team_idx);

#endif /* ELIFOOT_CUP_H */
