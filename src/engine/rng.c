/*
 * rng.c -- LCG 32 bits para Elifoot II Genesis
 *
 * m68k caveat: u32 tem 32 bits -- a multiplica??o de u32 * u32
 * no m68k produz resultado u32 (truncado), que ? exatamente o
 * comportamento desejado para o LCG (aritm?tica mod 2^32).
 * Usar sufixo UL nas constantes para garantir que o compilador
 * trate como long (32 bits) e n?o int (16 bits no m68k).
 */

#include <genesis.h>
#include "rng.h"

static u32 s_rng_state;

void rng_init(u16 seed) {
    /* Mistura a semente com constante para evitar estado zero.        */
    s_rng_state = (u32)seed * 1664525UL + 1013904223UL;
}

u16 rng_next(void) {
    s_rng_state = s_rng_state * 1664525UL + 1013904223UL;
    return (u16)(s_rng_state >> 16);
}

u16 rng_range(u16 range) {
    if (range == 0u) return 0u;
    return rng_next() % range;
}
