#ifndef COMPAT_H
#define COMPAT_H

/*
 * compat.h — Funções ausentes em libmd.a do SGDK 1.70
 *
 * SGDK 1.70 / libmd.a NÃO fornece:
 *   - memmove()  — necessária quando origem e destino se sobrepõem
 *   - strncmp()  — necessária para comparações de string com limite
 *   - atol()     — necessária para parsing de números longos (32 bits)
 *
 * ATENÇÃO — caveats críticos:
 *
 *   1. NÃO usar #ifndef SGDK_GCC aqui. Estas funções precisam compilar
 *      e linkar tanto no build SGDK quanto em builds de teste no host.
 *      O guard #ifndef SGDK_GCC quebraria o build SGDK.
 *
 *   2. strlen() do SGDK retorna u16, não size_t. NÃO redeclarar strlen()
 *      aqui — incluir genesis.h é suficiente para tê-la.
 *
 *   3. strcmp() do SGDK retorna s16, não int. Mesma regra.
 *
 *   4. memcpy() e memset() estão em libmd.a — NÃO reimplementar.
 *      Usar -fno-builtin para evitar substituição por builtins GCC.
 *
 *   5. int = 16 bits no m68k. Comprimentos de buffer usam u16.
 *      Usar long para valores que podem exceder 32767.
 */

#include <genesis.h>

/*
 * compat_memmove() — Move bloco de memória, tratando sobreposição.
 * Equivalente ao memmove() padrão, mas com len u16 (máx 65535 bytes).
 * Suficiente para todos os buffers do Elifoot II Genesis.
 */
void *compat_memmove(void *dst, const void *src, u16 len);

/*
 * compat_strncmp() — Compara no máximo n chars de s1 e s2.
 * Retorna s16: <0 se s1<s2, 0 se iguais, >0 se s1>s2.
 * n é u16 (consistente com o padrão do SGDK para tamanhos de string).
 */
s16 compat_strncmp(const char *s1, const char *s2, u16 n);

/*
 * compat_atol() — Converte string decimal para long (32 bits).
 * Aceita sinal '-' opcional. Para para no primeiro char não-dígito.
 * Necessário porque int = 16 bits no m68k — atoi() transbordaria
 * para valores monetários como salários (podem exceder 32767).
 */
long compat_atol(const char *s);

/*
 * compat_ltoa() — Converte long para string decimal, NUL-terminated.
 * buf deve ter pelo menos 12 bytes (sinal + 10 dígitos + NUL).
 * Retorna ponteiro para buf.
 */
char *compat_ltoa(long value, char *buf);

/*
 * compat_min_u16() / compat_max_u16() — Min/max sem macro.
 * Evitam double-evaluation de expressões com efeitos colaterais.
 */
u16 compat_min_u16(u16 a, u16 b);
u16 compat_max_u16(u16 a, u16 b);

/*
 * compat_clamp_u16() — Limita value ao intervalo [lo, hi].
 */
u16 compat_clamp_u16(u16 value, u16 lo, u16 hi);

/*
 * compat_clamp_long() — Limita value ao intervalo [lo, hi] (32 bits).
 */
long compat_clamp_long(long value, long lo, long hi);

#endif /* COMPAT_H */
