#ifndef ELIFOOT_ECONOMY_H
#define ELIFOOT_ECONOMY_H

#include <genesis.h>
#include "types.h"

/* Sal?rio m?nimo que um jogador de for?a 'strength' aceita.          */
long economy_min_salary(u8 strength);

/* Processa pedidos de aumento salarial no in?cio de cada temporada.
 * Percorre todos os jogadores da equipa team_idx. Para cada um que
 * ganha abaixo do m?nimo, pede aumento ou sai (se a equipa n?o pode
 * perder o jogador por ter poucos no plantel). Retorna n?mero de
 * jogadores que sa?ram (vendidos por 0 -- rescis?o for?ada).          */
u8 economy_process_salary_requests(u8 team_idx);

/* Receita de bilheteria para o jogo em casa da equipa home_team.     */
long economy_ticket_revenue(u8 home_team);

/* Pr?mios de classifica??o no fim da temporada.
 * Chama economy_pay_prize() para cada equipa com base na posi??o.    */
void economy_pay_prizes(void);

/* Paga sal?rios mensais de todos os jogadores de todas as equipas.
 * Chamada uma vez por jornada. Desconta de team->money.
 * Se money < 0 depois do pagamento, a equipa fica em d?vida.        */
void economy_pay_monthly_salaries(void);

/* Retorna TRUE se a equipa team_idx tem dinheiro suficiente para
 * pagar os sal?rios deste m?s sem ficar negativa.                    */
u8 economy_can_pay_salaries(u8 team_idx);

#endif /* ELIFOOT_ECONOMY_H */
