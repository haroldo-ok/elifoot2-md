#ifndef ELIFOOT_SQUAD_H
#define ELIFOOT_SQUAD_H

#include <genesis.h>

/*
 * screens/squad.h — Gestão de plantel e formação
 *
 * Tela principal de pré-jornada. Permite:
 *   - Ver plantel completo (11 titulares + banco)
 *   - Trocar jogador entre titular e banco
 *   - Mudar formação táctica (10 formações)
 *   - Auto-escalar (seleccionar os 11 melhores para a formação)
 *
 * Mapeamento de botões:
 *   D-pad ↑↓  Navegar jogadores
 *   A         Colocar em campo / tirar do campo (toggle)
 *   C         Auto-escalar (equivalente ao Alt+F3 original)
 *   Y         Mudar formação (cicla pelas 10 formações)
 *   B         Voltar ao menu pré-jornada
 */

/* Abre a tela de gestão do plantel da equipa do jogador.
 * Retorna quando o jogador pressiona B. */
void screen_squad(void);

#endif /* ELIFOOT_SQUAD_H */
