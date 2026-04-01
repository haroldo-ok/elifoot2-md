/*
 * game/transfer.c -- Transfer?ncias e leil?o por sal?rio
 *
 * m68k caveats:
 *   - long para todos os valores monet?rios (int = 16 bits).
 *   - u16 para ?ndices de jogadores (464 > 255 -- n?o usar u8).
 *   - Sem malloc: todos os buffers s?o locais ou arrays est?ticos globais.
 *   - compat_memmove() para mover blocos de Player[] ao compactar plantel.
 */

#include <genesis.h>
#include "transfer.h"
#include "data.h"
#include "economy.h"
#include "../engine/rng.h"
#include "../engine/compat.h"

/* ------------------------------------------------------------------ */
/* transfer_player()                                                   */
/* ------------------------------------------------------------------ */

u8 transfer_player(u16 player_idx, u8 from_team, u8 to_team, long new_salary) {
    Team   *src, *dst;
    Player *pl;
    u16     dst_slot;

    if (from_team >= (u8)TEAM_COUNT || to_team >= (u8)TEAM_COUNT) return 0u;
    if (player_idx >= (u16)(TEAM_COUNT * PLAYERS_PER_TEAM))        return 0u;

    src = &g_teams[from_team];
    dst = &g_teams[to_team];
    pl  = &g_players[player_idx];

    /* Atualiza finan?as                                               */
    src->salary_total -= pl->salary;
    dst->salary_total += new_salary;
    pl->salary         = new_salary;

    /*
     * Move o jogador no array g_players[]:
     *
     * Estrat?gia: o jogador est? em player_idx dentro do bloco de src.
     * Precisamos remov?-lo do bloco de src e inseri-lo no bloco de dst.
     *
     * Para simplicidade e seguran?a no m68k (sem malloc), usamos
     * uma c?pia tempor?ria local do registro do jogador:
     */
    {
        Player  tmp;
        u16     i;

        /* Guarda o jogador                                            */
        tmp = *pl;
        tmp.salary   = new_salary;
        tmp.on_field = 0u;   /* novo na equipa: come?a no banco        */

        /* Remove do bloco de src (compacta)                          */
        {
            u16 offset_in_src = player_idx - (u16)src->player_start;
            u16 rem           = (u16)(src->player_count - 1u) - offset_in_src;
            if (rem > 0u) {
                compat_memmove(
                    &g_players[player_idx],
                    &g_players[player_idx + 1u],
                    (u16)(rem * sizeof(Player))
                );
            }
            src->player_count--;
        }

        /*
         * Ap?s remover do src, todos os player_start de equipas
         * com ?ndice > src->player_start precisam ser decrementados
         * se o seu bloco est? "? direita" do slot removido.
         */
        for (i = 0u; i < (u8)TEAM_COUNT; i++) {
            if (i == from_team) continue;
            if (g_teams[i].player_start > player_idx) {
                g_teams[i].player_start--;
            }
        }

        /* Insere no bloco de dst (no fim do bloco)                   */
        dst_slot = (u16)dst->player_start + (u16)dst->player_count;

        /* Abre espa?o no fim do bloco de dst (empurra equipas seguintes) */
        if (dst_slot < (u16)(TEAM_COUNT * PLAYERS_PER_TEAM) - 1u) {
            compat_memmove(
                &g_players[dst_slot + 1u],
                &g_players[dst_slot],
                (u16)(((u16)(TEAM_COUNT * PLAYERS_PER_TEAM) - 1u - dst_slot) * sizeof(Player))
            );
        }
        g_players[dst_slot] = tmp;
        dst->player_count++;

        /* Incrementa player_start das equipas cujo bloco foi deslocado */
        for (i = 0u; i < (u8)TEAM_COUNT; i++) {
            if (i == to_team) continue;
            if (g_teams[i].player_start >= dst_slot) {
                g_teams[i].player_start++;
            }
        }
    }

    return 1u;
}

/* ------------------------------------------------------------------ */
/* transfer_auction()                                                  */
/* ------------------------------------------------------------------ */

u8 transfer_auction(u16 player_idx, u8 current_team) {
    u8    winner_team = 0xFFu;
    long  best_offer  = 0L;
    long  min_sal;
    u8    i;

    if (player_idx >= (u16)(TEAM_COUNT * PLAYERS_PER_TEAM)) return 0xFFu;

    min_sal = economy_min_salary(g_players[player_idx].strength);

    for (i = 0u; i < (u8)TEAM_COUNT; i++) {
        long budget, offer;
        if (i == current_team) continue;

        /* A IA s? faz proposta se tiver or?amento m?nimo              */
        budget = g_teams[i].money / 10L;
        if (budget < min_sal) continue;

        /* Proposta: min_sal + varia??o aleat?ria at? 50% do min_sal   */
        {
            u16 spread = (u16)(min_sal / 2L > 9999L ? 9999L : min_sal / 2L);
            offer = min_sal + (long)rng_range(spread + 1u);
        }
        if (offer > budget) offer = budget;

        if (offer > best_offer) {
            best_offer  = offer;
            winner_team = i;
        }
    }

    if (winner_team != 0xFFu) {
        transfer_player(player_idx, current_team, winner_team, best_offer);
    }

    return winner_team;
}

/* ------------------------------------------------------------------ */
/* transfer_ai_buy()                                                   */
/* ------------------------------------------------------------------ */

u8 transfer_ai_buy(u8 team_idx) {
    Team   *team;
    Player *pl;
    u16     slot;
    u8      strength;
    long    salary;

    if (team_idx >= (u8)TEAM_COUNT) return 0u;
    team = &g_teams[team_idx];

    /* Or?amento para contrata??o: at? 20% do dinheiro dispon?vel     */
    salary = team->money / 5L;
    if (salary <= 0L) return 0u;

    /* For?a do jogador derivada do sal?rio que a equipa pode pagar    */
    strength = (u8)(salary / 100L);
    if (strength < 20u) strength = 20u;
    if (strength > 99u) strength = 99u;

    /* Adiciona jogador no fim do bloco da equipa                      */
    if ((u16)team->player_start + (u16)team->player_count >=
        (u16)(TEAM_COUNT * PLAYERS_PER_TEAM)) {
        return 0u;  /* sem espa?o no array global -- caso extremo        */
    }

    slot = (u16)team->player_start + (u16)team->player_count;

    /* Empurra equipas seguintes                                       */
    if (slot < (u16)(TEAM_COUNT * PLAYERS_PER_TEAM) - 1u) {
        u8 i;
        compat_memmove(
            &g_players[slot + 1u],
            &g_players[slot],
            (u16)(((u16)(TEAM_COUNT * PLAYERS_PER_TEAM) - 1u - slot) * sizeof(Player))
        );
        for (i = 0u; i < (u8)TEAM_COUNT; i++) {
            if (i == team_idx) continue;
            if (g_teams[i].player_start >= slot) g_teams[i].player_start++;
        }
    }

    pl = &g_players[slot];
    /* Nome gen?rico para jogador contratado pela IA                   */
    {
        u8 j;
        const char *src = "Jogador";
        for (j = 0u; j < 7u; j++) pl->name[j] = src[j];
        pl->name[7] = '\0';
    }
    pl->pos      = (u8)POS_DF;   /* padr?o: defensor                   */
    pl->nat      = 0u;
    pl->strength = strength;
    pl->on_field = 0u;
    pl->salary   = salary;

    team->player_count++;
    team->salary_total += salary;
    team->money        -= salary;  /* pagamento de luvas (simplificado) */

    return 1u;
}
