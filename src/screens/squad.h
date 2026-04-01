#ifndef ELIFOOT_SQUAD_H
#define ELIFOOT_SQUAD_H

#include <genesis.h>

/*
 * screens/squad.h -- Gest?o de plantel e forma??o
 *
 * Tela principal de pr?-jornada. Permite:
 *   - Ver plantel completo (11 titulares + banco)
 *   - Trocar jogador entre titular e banco
 *   - Mudar forma??o t?ctica (10 forma??es)
 *   - Auto-escalar (seleccionar os 11 melhores para a forma??o)
 *
 * Mapeamento de bot?es:
 *   D-pad ??  Navegar jogadores
 *   A         Colocar em campo / tirar do campo (toggle)
 *   C         Auto-escalar (equivalente ao Alt+F3 original)
 *   Y         Mudar forma??o (cicla pelas 10 forma??es)
 *   B         Voltar ao menu pr?-jornada
 */

/* Abre a tela de gest?o do plantel da equipa do jogador.
 * Retorna quando o jogador pressiona B. */
void screen_squad(void);

#endif /* ELIFOOT_SQUAD_H */
