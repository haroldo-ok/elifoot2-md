#ifndef ELIFOOT_ECONOMY_H
#define ELIFOOT_ECONOMY_H

#include <genesis.h>
#include "types.h"

/* Salário mínimo que um jogador de força 'strength' aceita.          */
long economy_min_salary(u8 strength);

/* Processa pedidos de aumento salarial no início de cada temporada.
 * Percorre todos os jogadores da equipa team_idx. Para cada um que
 * ganha abaixo do mínimo, pede aumento ou sai (se a equipa não pode
 * perder o jogador por ter poucos no plantel). Retorna número de
 * jogadores que saíram (vendidos por 0 — rescisão forçada).          */
u8 economy_process_salary_requests(u8 team_idx);

/* Receita de bilheteria para o jogo em casa da equipa home_team.     */
long economy_ticket_revenue(u8 home_team);

/* Prémios de classificação no fim da temporada.
 * Chama economy_pay_prize() para cada equipa com base na posição.    */
void economy_pay_prizes(void);

/* Paga salários mensais de todos os jogadores de todas as equipas.
 * Chamada uma vez por jornada. Desconta de team->money.
 * Se money < 0 depois do pagamento, a equipa fica em dívida.        */
void economy_pay_monthly_salaries(void);

/* Retorna TRUE se a equipa team_idx tem dinheiro suficiente para
 * pagar os salários deste mês sem ficar negativa.                    */
u8 economy_can_pay_salaries(u8 team_idx);

#endif /* ELIFOOT_ECONOMY_H */
