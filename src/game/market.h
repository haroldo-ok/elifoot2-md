#ifndef ELIFOOT_MARKET_H
#define ELIFOOT_MARKET_H

#include <genesis.h>
#include "types.h"

/*
 * game/market.h -- "VENDA PELA MELHOR OFERTA DE ORDENADO"
 *
 * No original, antes de cada jornada varios jogadores de outros clubes vao
 * a leilao: cada treinador humano oferece um ordenado; ganha a maior oferta
 * (>= ordenado minimo) e o clube comprador paga o PRECO ao vendedor.
 * A venda (Ctrl+F1) e os pedidos de aumento recusados usam o mesmo leilao.
 */

/* Escolhe um jogador da IA para leilao. PLAYER_NONE se nao houver. */
u16  market_pick_auction(void);
/* Melhor oferta da IA (excepto exclude). Devolve equipa ou TEAM_NONE. */
u8   market_ai_bid(u16 p, long price, u8 exclude, u16 *salary);
/* Transfere p para to com novo ordenado; to paga price ao clube de p. */
void market_transfer(u16 p, u8 to, u16 salary, long price);
/* Jogador sai ("Entao adeus."): vai para um clube da IA sem custo. */
void market_release(u16 p);
/* Pode o clube t ficar sem p? (min. 14 jogadores e 1 guarda-redes) */
u8   market_can_lose(u8 t, u16 p);

/* Pedido de aumento para a equipa t: 1 se houve. */
u8   market_salary_request(u8 t, u16 *p, u16 *new_salary, long *auction_price);
/* Quanto o jogador exige no minimo (Alterar o ordenado). */
u16  market_demand(u16 p);

#endif
