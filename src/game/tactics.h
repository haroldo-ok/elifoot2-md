#ifndef ELIFOOT_TACTICS_H
#define ELIFOOT_TACTICS_H

#include <genesis.h>
#include "types.h"

/* Nome da formacao F1..F10 ("3-4-3" ...) */
const char *tactics_name(u8 form);
/* Numero de jogadores pedidos por posicao (GR, DF, MD, AV). */
u8 tactics_need(u8 form, u8 pos);

/* Escala os melhores jogadores de t para a formacao (Seleccionar melhores). */
void tactics_autoselect(u8 t, u8 form);
/* Formacao e escalacao da IA. */
void tactics_ai(u8 t);

/* Titulares (em campo) e suplentes de t. */
u8 tactics_starters(u8 t, u16 *out);
u8 tactics_bench(u8 t, u16 *out);

/* Ataque e defesa da equipa com os titulares actuais. */
void tactics_rating(u8 t, u16 *att, u16 *def);

#endif
