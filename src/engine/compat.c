/*
 * compat.c — Funções ausentes em libmd.a do SGDK 1.70
 *
 * CRÍTICO: NÃO usar #ifndef SGDK_GCC neste arquivo.
 * Estas funções compilam e linkam em TODOS os builds.
 * O linker do SGDK vai usar estas implementações quando libmd.a
 * não fornecer os símbolos correspondentes.
 *
 * m68k caveats:
 *   - int = 16 bits: todos os contadores de loop usam u16.
 *   - Sem stdlib.h disponível — não usar tipos como size_t.
 *   - Ponteiros têm 32 bits (sizeof(void*) = 4 no m68k).
 *   - Não usar __builtin_memset / __builtin_memcpy — bypassam
 *     -fno-builtin e têm assinatura diferente.
 */

#include <genesis.h>
#include "compat.h"

/* ------------------------------------------------------------------ */
/* compat_memmove()                                                    */
/* ------------------------------------------------------------------ */

void *compat_memmove(void *dst, const void *src, u16 len) {
    /*
     * Trata sobreposição de regiões:
     *   - Se dst < src ou não há sobreposição: copia de frente para trás.
     *   - Se dst > src e há sobreposição: copia de trás para frente
     *     para evitar corromper dados ainda não copiados.
     */
    u8 *d = (u8 *)dst;
    const u8 *s = (const u8 *)src;
    u16 i;

    if (d == s || len == 0u) {
        return dst;
    }

    if (d < s) {
        /* Sem risco de sobreposição nesta direção: copia para frente. */
        for (i = 0u; i < len; i++) {
            d[i] = s[i];
        }
    } else {
        /* dst > src: possível sobreposição — copia de trás para frente.*/
        i = len;
        while (i > 0u) {
            i--;
            d[i] = s[i];
        }
    }

    return dst;
}

/* ------------------------------------------------------------------ */
/* compat_strncmp()                                                    */
/* ------------------------------------------------------------------ */

s16 compat_strncmp(const char *s1, const char *s2, u16 n) {
    u16 i;
    u8 c1, c2;

    if (n == 0u) return 0;

    for (i = 0u; i < n; i++) {
        c1 = (u8)s1[i];
        c2 = (u8)s2[i];
        if (c1 != c2) {
            return (s16)((s16)c1 - (s16)c2);
        }
        if (c1 == 0u) {
            return 0;  /* ambos chegaram ao NUL simultaneamente         */
        }
    }
    return 0;  /* primeiros n chars idênticos                          */
}

/* ------------------------------------------------------------------ */
/* compat_atol()                                                       */
/* ------------------------------------------------------------------ */

long compat_atol(const char *s) {
    long result = 0L;
    u8 negative = 0u;
    const char *p = s;

    /* Pula espaços iniciais.                                          */
    while (*p == ' ' || *p == '\t') p++;

    /* Sinal opcional.                                                 */
    if (*p == '-') {
        negative = 1u;
        p++;
    } else if (*p == '+') {
        p++;
    }

    /* Dígitos. Usa long arithmetic — int seria 16 bits aqui.         */
    while (*p >= '0' && *p <= '9') {
        result = result * 10L + (long)(*p - '0');
        p++;
    }

    return negative ? -result : result;
}

/* ------------------------------------------------------------------ */
/* compat_ltoa()                                                       */
/* ------------------------------------------------------------------ */

char *compat_ltoa(long value, char *buf) {
    /*
     * Converte long para string decimal.
     * Estratégia: preenche de trás para frente, depois reverte.
     * buf precisa de pelo menos 12 bytes: sinal(1) + dígitos(10) + NUL(1).
     */
    u8 pos = 0u;
    u8 start = 0u;
    u8 i, j;
    char tmp;
    long v = value;

    if (v < 0L) {
        buf[pos++] = '-';
        start = 1u;
        v = -v;
    }

    if (v == 0L) {
        buf[pos++] = '0';
    } else {
        while (v > 0L) {
            buf[pos++] = (char)('0' + (u8)(v % 10L));
            v /= 10L;
        }
        /* Reverte apenas a parte numérica (após o sinal).             */
        i = start;
        j = (u8)(pos - 1u);
        while (i < j) {
            tmp = buf[i];
            buf[i] = buf[j];
            buf[j] = tmp;
            i++;
            j--;
        }
    }

    buf[pos] = '\0';
    return buf;
}

/* ------------------------------------------------------------------ */
/* compat_min_u16() / compat_max_u16() / compat_clamp_u16()           */
/* ------------------------------------------------------------------ */

u16 compat_min_u16(u16 a, u16 b) {
    return (a < b) ? a : b;
}

u16 compat_max_u16(u16 a, u16 b) {
    return (a > b) ? a : b;
}

u16 compat_clamp_u16(u16 value, u16 lo, u16 hi) {
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}

/* ------------------------------------------------------------------ */
/* compat_clamp_long()                                                 */
/* ------------------------------------------------------------------ */

long compat_clamp_long(long value, long lo, long hi) {
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}
