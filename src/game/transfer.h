#ifndef ELIFOOT_TRANSFER_H
#define ELIFOOT_TRANSFER_H

#include <genesis.h>
#include "types.h"

/*
 * game/transfer.h -- Transfer?ncias e leil?o por sal?rio
 *
 * O sistema original do Elifoot II tem dois mecanismos:
 *
 *   1. Venda directa: o jogador humano vende um jogador por um pre?o
 *      negociado. A equipa compradora ? escolhida pela IA.
 *
 *   2. Leil?o por sal?rio: o jogador insatisfeito com o sal?rio
 *      pode ir a leil?o. As equipas da IA fazem propostas. A equipa
 *      com a proposta mais alta fica com o jogador ao sal?rio proposto.
 */

/*
 * transfer_player() -- Move jogador player_idx da equipa from_team
 * para to_team, com o novo sal?rio new_salary.
 * Atualiza player_start dos dois times e o salary_total.
 * Retorna 1 se bem-sucedido, 0 se falhar (?ndices inv?lidos).
 */
u8 transfer_player(u16 player_idx, u8 from_team, u8 to_team, long new_salary);

/*
 * transfer_auction() -- Lan?a leil?o para o jogador player_idx da
 * equipa current_team. As outras equipas da IA fazem propostas.
 * Retorna o ?ndice da equipa vencedora, ou 0xFF se n?o houver propostas.
 * O jogador ? transferido automaticamente se houver vencedor.
 */
u8 transfer_auction(u16 player_idx, u8 current_team);

/*
 * transfer_ai_buy() -- A IA tenta comprar um jogador livre.
 * Chamada para equipas com menos de MIN_SQUAD_SIZE jogadores.
 * Gera um jogador sint?tico (for?a baseada no or?amento dispon?vel).
 * Retorna 1 se comprou, 0 se sem or?amento.
 */
u8 transfer_ai_buy(u8 team_idx);

#endif /* ELIFOOT_TRANSFER_H */
