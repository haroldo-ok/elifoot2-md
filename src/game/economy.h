#ifndef ELIFOOT_ECONOMY_H
#define ELIFOOT_ECONOMY_H

#include <genesis.h>
#include "types.h"
#include "match.h"

/* Premios (Ctrl+F3 no original, valores observados) */
#define PRIZE_FIRST   0u
#define PRIZE_SECOND  1u
#define PRIZE_THIRD   2u
#define PRIZE_CUP     3u
#define PRIZE_ATTACK  4u
#define PRIZE_DEFENCE 5u
#define PRIZE_SCORER  6u
long economy_prize(u8 kind);
const char *economy_prize_label(u8 kind);

u16  economy_attendance(u8 home, u8 away);
/* Bilheteira do jogo para o anfitriao; regista em Ultimas Receitas. */
void economy_home_revenue(const LiveGame *g, u8 kind);
void economy_socios_after(const LiveGame *g);
void economy_pay_salaries(u8 t);
void economy_stands_tick(u8 t);
void economy_ai_finances(u8 t);
void economy_pay_prizes(void);
/* Constroi uma bancada (100000). 1 se conseguiu. */
u8   economy_build_stand(u8 t);

#endif
