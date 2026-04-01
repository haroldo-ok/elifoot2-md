/*
 * game/economy.c — Sistema económico do Elifoot II Genesis
 *
 * m68k / SGDK 1.70 caveats:
 *   - Todos os valores monetários usam 'long' (32 bits).
 *     int = 16 bits no m68k — salários e receitas excedem facilmente 32767.
 *   - Bilheteria: attendance × ticket_price pode ser 30000 × 10 = 300000L.
 *   - Prémios: até 500000L para campeão da Div1.
 *   - Multiplicações u16 × u16 podem exceder u16 — usar long explicitamente.
 */

#include <genesis.h>
#include "economy.h"
#include "data.h"
#include "../engine/rng.h"
#include "../engine/compat.h"

/* ------------------------------------------------------------------ */
/* Tabela de prémios por posição (Div1 e Div2)                        */
/* ------------------------------------------------------------------ */

/* Prémio para posição pos (1-based) na divisão div (0=Div1, 1=Div2). */
static long s_prizes_div1[16] = {
    500000L, 350000L, 250000L, 180000L, 130000L, 100000L,
     80000L,  60000L,  40000L,  30000L,  20000L,  15000L,
     10000L,   8000L,   5000L,   3000L,
};
static long s_prizes_div2[13] = {
    200000L, 140000L, 100000L,  70000L,  50000L,  35000L,
     25000L,  18000L,  12000L,   8000L,   5000L,   3000L,
      2000L,
};

/* ------------------------------------------------------------------ */
/* economy_min_salary()                                                */
/* ------------------------------------------------------------------ */

long economy_min_salary(u8 strength) {
    /* Fiel ao original: salário mínimo = força × 100 escudos/mês.    */
    return (long)strength * 100L;
}

/* ------------------------------------------------------------------ */
/* economy_process_salary_requests()                                   */
/* ------------------------------------------------------------------ */

u8 economy_process_salary_requests(u8 team_idx) {
    Team  *team;
    Player *pl;
    u8     i, left = 0u;
    u8     forced_out = 0u;
    u16    pi;

    if (team_idx >= (u8)TEAM_COUNT) return 0u;
    team = &g_teams[team_idx];

    for (i = 0u; i < team->player_count; i++) {
        pi = (u16)team->player_start + i;
        pl = &g_players[pi];

        if (pl->salary >= economy_min_salary(pl->strength)) continue;

        /* Jogador insatisfeito com o salário.                         */
        /* Verificar se a equipa pode perder o jogador:               */
        /*   - Mínimo de 14 no plantel                                */
        /*   - Pelo menos 1 guarda-redes                              */
        u8 gk_count   = data_get_team_gk_count(team_idx);
        u8 total      = data_get_team_player_count(team_idx);
        u8 is_only_gk = (pl->pos == (u8)POS_GR && gk_count <= 1u);
        u8 too_few    = (total <= (u8)MIN_SQUAD_SIZE);

        if (is_only_gk || too_few) {
            /* Forçado a manter: aumenta salário ao mínimo            */
            team->salary_total -= pl->salary;
            pl->salary          = economy_min_salary(pl->strength);
            team->salary_total += pl->salary;
        } else {
            /* 50% de chance de sair (comportamento do original)      */
            if (rng_range(2u) == 0u) {
                /* Jogador sai — remove do plantel (marca como inativo) */
                /* Implementação simplificada: apenas reduz player_count */
                /* e compacta o array (memmove via compat_memmove)     */
                team->salary_total -= pl->salary;
                /* Compacta: move os jogadores seguintes uma posição   */
                if (i < (u8)(team->player_count - 1u)) {
                    u16 src = pi + 1u;
                    u16 dst = pi;
                    u16 rem = (u16)(team->player_count - 1u - i);
                    compat_memmove(
                        &g_players[dst],
                        &g_players[src],
                        (u16)(rem * sizeof(Player))
                    );
                    i--;   /* reprocessa a posição atual              */
                }
                team->player_count--;
                forced_out++;
            } else {
                /* Aceita continuar: aumenta salário ao mínimo        */
                team->salary_total -= pl->salary;
                pl->salary          = economy_min_salary(pl->strength);
                team->salary_total += pl->salary;
            }
        }
        left++;
    }
    (void)left;
    return forced_out;
}

/* ------------------------------------------------------------------ */
/* economy_ticket_revenue()                                            */
/* ------------------------------------------------------------------ */

long economy_ticket_revenue(u8 home_team) {
    Team *home;
    u16   attendance;

    if (home_team >= (u8)TEAM_COUNT) return 0L;
    home = &g_teams[home_team];

    /* Público = metade da capacidade + variação aleatória de 25%.    */
    /* Usar u16 — stadium_cap max 30000, cabe em u16.                 */
    attendance = (u16)(home->stadium_cap / 2u) +
                 rng_range((u16)(home->stadium_cap / 4u));

    if (attendance > home->stadium_cap) attendance = home->stadium_cap;

    return (long)attendance * (long)home->ticket_price;
}

/* ------------------------------------------------------------------ */
/* economy_pay_prizes()                                                */
/* ------------------------------------------------------------------ */

void economy_pay_prizes(void) {
    u8  order[TEAM_COUNT];
    u8  count;
    u8  i;
    u8  div;

    for (div = 0u; div < 2u; div++) {
        data_sort_standings(div, order, &count);
        for (i = 0u; i < count; i++) {
            long prize = (div == 0u) ?
                ((i < 16u) ? s_prizes_div1[i] : 0L) :
                ((i < 13u) ? s_prizes_div2[i] : 0L);
            g_teams[order[i]].money += prize;
        }
    }
}

/* ------------------------------------------------------------------ */
/* economy_pay_monthly_salaries()                                      */
/* ------------------------------------------------------------------ */

void economy_pay_monthly_salaries(void) {
    u8 t;
    for (t = 0u; t < (u8)TEAM_COUNT; t++) {
        g_teams[t].money -= g_teams[t].salary_total;
    }
    /* Sincroniza g_money com a equipa do jogador                      */
    g_money = g_teams[g_player_team_idx].money;
}

/* ------------------------------------------------------------------ */
/* economy_can_pay_salaries()                                          */
/* ------------------------------------------------------------------ */

u8 economy_can_pay_salaries(u8 team_idx) {
    if (team_idx >= (u8)TEAM_COUNT) return 0u;
    return (g_teams[team_idx].money >= g_teams[team_idx].salary_total) ? 1u : 0u;
}
