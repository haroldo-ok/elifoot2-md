#ifndef ELIFOOT_COACHES_H
#define ELIFOOT_COACHES_H

#include <genesis.h>
#include "types.h"

/* CHICOTADAS PSICOLOGICAS da jornada (clubes da IA). */
typedef struct {
    u8 team;      /* clube que despediu                 */
    u8 old_c;     /* treinador despedido                */
    u8 new_c;     /* novo treinador                     */
    u8 swap;      /* clube com quem trocou, TEAM_NONE   */
} Sacking;

#define SACKINGS_MAX 6u
extern Sacking g_sack[SACKINGS_MAX];
extern u8      g_sack_n;

/* Corre depois de cada jornada do campeonato. */
void coaches_ai_sackings(void);

/* Treinador livre (nao empregado por nenhum clube da IA). */
u8   coaches_free(void);

/* Adiciona um treinador humano com uma equipa ao acaso da ultima divisao
 * ainda sem treinador humano. Devolve indice ou 0xFF. */
u8   managers_add(const char *name);
void managers_remove(u8 m);
/* Treinador humano m passa a treinar t (o treinador da IA de t vai
 * para o clube antigo). */
void managers_move(u8 m, u8 t);
/* Oferta de emprego no fim da epoca: devolve clube ou TEAM_NONE. */
u8   managers_job_offer(u8 m);

#endif
