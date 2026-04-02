/*
 * screens/transfers.c -- Mercado de transferencias
 *
 * Permite ao jogador ver o plantel, colocar jogadores em leilao
 * e comprar jogadores de outros clubes via transfer_auction().
 */

#include <genesis.h>
#include "transfers.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"
#include "../game/transfer.h"

static const char s_pos[4][3] = { "GR", "DF", "MD", "AV" };

/* Subecra: lista de jogadores compraveis de outras equipas */
static void buy_screen(void) {
    /* Lista ate 16 jogadores de equipas rivais com menor forca */
    u8 candidates[16];
    u8 n = 0u;
    u8 t, i;

    /* Recolher jogadores vendaveis (forca < 70, nao da equipa do jogador) */
    for (t = 0u; t < (u8)TEAM_COUNT && n < 16u; t++) {
        if (t == g_player_team_idx) continue;
        for (i = 0u; i < g_teams[t].player_count && n < 16u; i++) {
            u16 pidx = (u16)(g_teams[t].player_start + i);
            if (g_players[pidx].strength < 70u) {
                candidates[n++] = (u8)pidx;
            }
        }
    }

    {
        u8  sel = 0u;
        u8  redraw = 1u;

        for (;;) {
            ui_wait_vblank();
            input_update();

            if (input_pressed(BTN_CANCEL) || input_pressed(BTN_START)) return;

            if (input_repeat(BTN_DOWN) && sel < (u8)(n - 1u)) { sel++; redraw = 1u; }
            if (input_repeat(BTN_UP)   && sel > 0u)            { sel--; redraw = 1u; }

            if (input_pressed(BTN_CONFIRM) && n > 0u) {
                u16 pidx = candidates[sel];
                /* Find which team owns this player */
                u8  owner_team = 0u;
                {
                    u8 tt;
                    for (tt = 0u; tt < (u8)TEAM_COUNT; tt++) {
                        u16 ps = g_teams[tt].player_start;
                        u16 pe = (u16)(ps + g_teams[tt].player_count);
                        if (pidx >= ps && pidx < pe) { owner_team = tt; break; }
                    }
                }
                u8  result = transfer_auction(pidx, owner_team);
                ui_clear();
                if (result) {
                    ui_puts(6u, 13u, UI_PAL_NORMAL, "Transferencia concluida!");
                } else {
                    ui_puts(6u, 13u, UI_PAL_NORMAL, "Transferencia falhada.");
                }
                ui_puts(6u, 15u, UI_PAL_NORMAL, "B: continuar");
                { u16 x; for (x=0u; x<120u; x++) { ui_wait_vblank(); input_update(); if(input_pressed(BTN_CANCEL)) break; } }
                return;
            }

            if (!redraw) continue;
            redraw = 0u;

            ui_clear();
            ui_puts(10u, 0u, UI_PAL_NORMAL, "COMPRAR JOGADOR");
            ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);
            ui_puts(0u, 2u, UI_PAL_NORMAL, "#  Nome            Pos For Sal");
            ui_hline(0u, 3u, UI_COLS, UI_PAL_NORMAL);

            {
                u8 j;
                for (j = 0u; j < n; j++) {
                    u16 pidx = candidates[j];
                    Player *pl = &g_players[pidx];
                    u16 row = (u16)(4u + j);
                    u16 pal = (j == sel) ? UI_PAL_SELECT : UI_PAL_NORMAL;
                    if (j == sel) ui_fill_row(row, UI_PAL_SELECT);
                    ui_printf(0u, row, pal, "%2u %-15s %2s %3u %6ld",
                              (u16)(j + 1u), pl->name,
                              s_pos[pl->pos < 4u ? pl->pos : 0u],
                              (u16)pl->strength, pl->salary);
                }
            }

            ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
            ui_puts(0u, 27u, UI_PAL_NORMAL, "A:comprar  B:voltar");
        }
    }
}

/* ------------------------------------------------------------------ */
/* venda_directa() -- VENDA PELA MELHOR OFERTA DE ORDENADO            */
/*                                                                     */
/* Fiel ao original:                                                   */
/*   Titulo: VENDA PELA MELHOR OFERTA DE ORDENADO                     */
/*   Mostra: JOGADOR | POSICAO | FORCA | EQUIPA | PRECO               */
/*   Gestor define preco minimo (ordenado minimo = salario actual)     */
/*   IA equipas avaliam: TRANSFERIDO PARA O [clube] + NOVO ORDENADO   */
/*   Ou: NAO HOUVE OFERTAS                                             */
/* ------------------------------------------------------------------ */

static void venda_directa(u8 team_idx) {
    Team  *team  = &g_teams[team_idx];
    u8     sel   = 0u;
    u8     total = team->player_count;
    u8     redraw = 1u;

    for (;;) {
        ui_wait_vblank();
        input_update();

        if (input_pressed(BTN_CANCEL) || input_pressed(BTN_START)) return;
        if (input_repeat(BTN_DOWN) && sel < (u8)(total-1u)) { sel++; redraw = 1u; }
        if (input_repeat(BTN_UP)   && sel > 0u)             { sel--; redraw = 1u; }

        if (input_pressed(BTN_CONFIRM)) {
            u16    pidx = (u16)(team->player_start + sel);
            Player *pl  = &g_players[pidx];

            /* Squad constraints */
            if (team->player_count <= 14u) {
                ui_clear();
                ui_puts(4u, 13u, UI_PAL_NORMAL, "Plantel minimo (14 jog.)!");
                { u16 t; for(t=0u;t<90u;t++){ ui_wait_vblank(); input_update();
                    if(input_pressed(BTN_CANCEL)) break; } }
                redraw = 1u; continue;
            }

            /* Show sale screen */
            ui_clear();
            ui_puts(2u, 0u, UI_PAL_NORMAL, "VENDA PELA MELHOR OFERTA DE ORDENADO");
            ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);
            ui_puts(0u, 2u, UI_PAL_NORMAL, "JOGADOR       POSICAO FORCA  EQUIPA");
            ui_hline(0u, 3u, UI_COLS, UI_PAL_NORMAL);
            ui_printf(0u, 4u, UI_PAL_NORMAL, "%-14s %2s      %2u     %s",
                      pl->name,
                      s_pos_ao[pl->pos < 4u ? pl->pos : 0u],
                      (u16)pl->strength,
                      team->name);
            ui_printf(0u, 6u, UI_PAL_NORMAL, "ORDENADO MINIMO: %ld Esc", pl->salary);
            ui_hline(0u, 8u, UI_COLS, UI_PAL_NORMAL);
            ui_puts(0u, 10u, UI_PAL_NORMAL, "A IA esta a avaliar propostas...");
            ui_wait_vblank(); ui_wait_vblank(); ui_wait_vblank();

            {
                /* Run auction at current salary (direct sale = salary auction) */
                u8 winner = transfer_auction(pidx, team_idx);
                ui_clear();
                ui_puts(2u, 0u, UI_PAL_NORMAL, "VENDA PELA MELHOR OFERTA DE ORDENADO");
                ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);
                if (winner != 0xFFu) {
                    ui_printf(2u, 6u, UI_PAL_NORMAL,
                              "TRANSFERIDO PARA O %s", g_teams[winner].name);
                    ui_printf(2u, 8u, UI_PAL_NORMAL,
                              "NOVO ORDENADO : %ld Esc",
                              g_players[pidx].salary);
                } else {
                    ui_puts(2u, 8u, UI_PAL_NORMAL, "NAO HOUVE OFERTAS");
                }
            }

            ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
            ui_puts(0u, 27u, UI_PAL_NORMAL, "A/B: continuar");
            { u16 t; for(t=0u;t<180u;t++){ ui_wait_vblank(); input_update();
                if(input_pressed(BTN_CONFIRM)||input_pressed(BTN_CANCEL)) break; } }
            return;
        }

        if (!redraw) continue;
        redraw = 0u;

        ui_clear();
        ui_puts(2u, 0u, UI_PAL_NORMAL, "VENDA PELA MELHOR OFERTA DE ORDENADO");
        ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);
        ui_puts(0u, 2u, UI_PAL_NORMAL, "Escolha o jogador a vender:");
        ui_puts(0u, 3u, UI_PAL_NORMAL, "#  Nome           Pos For Sal");
        ui_hline(0u, 4u, UI_COLS, UI_PAL_NORMAL);

        {
            u8 i;
            for (i = 0u; i < total && i < 20u; i++) {
                u16 pidx = (u16)(team->player_start + i);
                Player *pl = &g_players[pidx];
                u16 row = (u16)(5u + i);
                u16 pal = (i == sel) ? UI_PAL_SELECT : UI_PAL_NORMAL;
                if (i == sel) ui_fill_row(row, UI_PAL_SELECT);
                ui_printf(0u, row, pal, "%2u %-14s %2s %3u %6ld",
                          (u16)(i+1u), pl->name,
                          s_pos_ao[pl->pos < 4u ? pl->pos : 0u],
                          (u16)pl->strength, pl->salary);
            }
        }

        ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
        ui_puts(0u, 27u, UI_PAL_NORMAL, "A: vender  B: cancelar");
    }
}


void screen_transfers(void) {
    Team   *team  = &g_teams[g_player_team_idx];
    u8      sel   = 0u;
    u8      total = team->player_count;
    u8      redraw = 1u;

    for (;;) {
        ui_wait_vblank();
        input_update();

        if (input_pressed(BTN_CANCEL) || input_pressed(BTN_START)) return;

        if (input_repeat(BTN_DOWN) && sel < (u8)(total - 1u)) { sel++; redraw = 1u; }
        if (input_repeat(BTN_UP)   && sel > 0u)               { sel--; redraw = 1u; }

        /* X = venda directa (VENDA PELA MELHOR OFERTA DE ORDENADO) */
        if (input_pressed(BUTTON_X)) {
            venda_directa(g_player_team_idx);
            redraw = 1u;
            team = &g_teams[g_player_team_idx];
            total = team->player_count;
            continue;
        }
        /* C = ver mercado de compras */
        if (input_pressed(BTN_ACTION)) {
            buy_screen();
            redraw = 1u;
            team = &g_teams[g_player_team_idx];
            total = team->player_count;
        }

        /* A = vender jogador seleccionado */
        if (input_pressed(BTN_CONFIRM)) {
            u16 pidx = (u16)(team->player_start + sel);
            u8  result;
            /* Enforce minimum squad constraints */
            if (team->player_count <= 14u) {
                ui_clear();
                ui_puts(4u, 13u, UI_PAL_NORMAL, "Plantel minimo (14 jog.)!");
                { u16 t; for(t=0u;t<90u;t++){ ui_wait_vblank(); input_update(); if(input_pressed(BTN_CANCEL)) break; } }
                redraw = 1u;
                continue;
            }
            result = transfer_auction(pidx, g_player_team_idx);
            ui_clear();
            ui_puts(6u, 13u, UI_PAL_NORMAL,
                    result ? "Jogador vendido!" : "Venda nao concluida.");
            ui_puts(6u, 15u, UI_PAL_NORMAL, "B: continuar");
            { u16 x; for(x=0u;x<120u;x++){ ui_wait_vblank(); input_update(); if(input_pressed(BTN_CANCEL)) break; } }
            redraw = 1u;
        }

        if (!redraw) continue;
        redraw = 0u;

        ui_clear();
        ui_puts(0u, 0u, UI_PAL_NORMAL, team->name);
        ui_puts(24u, 0u, UI_PAL_NORMAL, "TRANSFERENCIAS");
        ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);
        ui_printf(0u, 2u, UI_PAL_NORMAL, "Dinheiro: %ld Esc", g_money);
        ui_puts(0u, 3u, UI_PAL_NORMAL, "#  Nome            Pos For Sal");
        ui_hline(0u, 4u, UI_COLS, UI_PAL_NORMAL);

        {
            u8 i;
            for (i = 0u; i < total && i < 20u; i++) {
                u16 pidx = (u16)(team->player_start + i);
                Player *pl = &g_players[pidx];
                u16 row = (u16)(5u + i);
                u16 pal = (i == sel) ? UI_PAL_SELECT : UI_PAL_NORMAL;
                if (i == sel) ui_fill_row(row, UI_PAL_SELECT);
                ui_printf(0u, row, pal, "%2u %-15s %2s %3u %6ld",
                          (u16)(i + 1u), pl->name,
                          s_pos[pl->pos < 4u ? pl->pos : 0u],
                          (u16)pl->strength, pl->salary);
            }
        }

        ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
        ui_puts(0u, 27u, UI_PAL_NORMAL, "A:leilao X:venda-dir C:comprar B:sair");
    }
}
