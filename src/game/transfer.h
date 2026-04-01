#ifndef ELIFOOT_TRANSFER_H
#define ELIFOOT_TRANSFER_H

#include <genesis.h>
#include "types.h"

/*
 * game/transfer.h — Transferências e leilão por salário
 *
 * O sistema original do Elifoot II tem dois mecanismos:
 *
 *   1. Venda directa: o jogador humano vende um jogador por um preço
 *      negociado. A equipa compradora é escolhida pela IA.
 *
 *   2. Leilão por salário: o jogador insatisfeito com o salário
 *      pode ir a leilão. As equipas da IA fazem propostas. A equipa
 *      com a proposta mais alta fica com o jogador ao salário proposto.
 */

/*
 * transfer_player() — Move jogador player_idx da equipa from_team
 * para to_team, com o novo salário new_salary.
 * Atualiza player_start dos dois times e o salary_total.
 * Retorna 1 se bem-sucedido, 0 se falhar (índices inválidos).
 */
u8 transfer_player(u16 player_idx, u8 from_team, u8 to_team, long new_salary);

/*
 * transfer_auction() — Lança leilão para o jogador player_idx da
 * equipa current_team. As outras equipas da IA fazem propostas.
 * Retorna o índice da equipa vencedora, ou 0xFF se não houver propostas.
 * O jogador é transferido automaticamente se houver vencedor.
 */
u8 transfer_auction(u16 player_idx, u8 current_team);

/*
 * transfer_ai_buy() — A IA tenta comprar um jogador livre.
 * Chamada para equipas com menos de MIN_SQUAD_SIZE jogadores.
 * Gera um jogador sintético (força baseada no orçamento disponível).
 * Retorna 1 se comprou, 0 se sem orçamento.
 */
u8 transfer_ai_buy(u8 team_idx);

#endif /* ELIFOOT_TRANSFER_H */
