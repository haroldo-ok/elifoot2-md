#ifndef RNG_H
#define RNG_H

/*
 * rng.h — Gerador de números pseudoaleatórios para Elifoot II Genesis
 *
 * LCG 32 bits com parâmetros de Numerical Recipes / Knuth:
 *   X(n+1) = X(n) * 1664525 + 1013904223  (mod 2^32)
 *
 * Estes são os mesmos parâmetros do LCG interno do Turbo Pascal,
 * garantindo comportamento similar ao jogo DOS original.
 *
 * Os 16 bits superiores do estado têm melhor distribuição que os
 * inferiores — rng_next() retorna sempre os bits 31..16.
 */

#include <genesis.h>

/* Inicializa o RNG com a semente fornecida.
 * Recomendado: usar GET_VCOUNTER() no momento de ligar para
 * variabilidade natural entre partidas. */
void rng_init(u16 seed);

/* Retorna próximo valor pseudoaleatório no range [0, 65535]. */
u16 rng_next(void);

/* Retorna valor no range [0, range-1]. range deve ser > 0. */
u16 rng_range(u16 range);

#endif /* RNG_H */
