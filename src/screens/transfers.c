/*
 * screens/transfers.c -- Venda e leil?o de jogadores
 *
 * Layout fiel ao original:
 *
 *   VENDA PELA MELHOR OFERTA DE ORDENADO
 *   ?????????????????????????????????????????
 *    JOGADOR        | POS | FRC | EQUIPA     | PRECO
 *   ?????????????????????????????????????????????????
 *    Zetti          |  GR |  85 | SAO PAULO  |  Venda
 *    Rogerio        |  GR |  72 | SAO PAULO  |  ...
 *
 *   ORDENADO MINIMO: 8500
 *   [A] Vender  [B] Voltar
 *
 * Ap?s venda:
 *   TRANSFERIDO PARA O CORINTHIANS
 *   NOVO ORDENADO: 9200
 *
 * Ou:
 *   NAO HOUVE OFERTAS
 */

#include <genesis.h>
#include "transfers.h"
#include "../game/data.h"
#include "../game/economy.h"
#include "../game/transfer.h"
#include "../game/types.h"
#include "../engine/input.h"
#include "../engine/render.h"

static const char * const s_pos_abbr[4] = { "GR", "DF", "MD", "AV" };

/* ------------------------------------------------------------------ */
/* Render da lista de jogadores                                        */
/* ------------------------------------------------------------------ */

static void render_transfers(u8 team_idx, ListNav *nav) {
    Team  *team = &g_teams[team_idx];
    u16    row  = (u16)CONTENT_ROW_FIRST;
    u8     i;

    render_clear_content();

    render_text(BG_A, "VENDA PELA MELHOR OFERTA DE ORDENADO",
                1u, row++, PAL_MAIN);
    render_hline(BG_A, 1u, row++, 38u, BOX_SIMPLE, PAL_MAIN);
    render_text(BG_A, " Nome            Pos Frc  Ord.Min  Actual",
                1u, row++, PAL_MAIN);
    render_hline(BG_A, 1u, row++, 38u, BOX_SIMPLE, PAL_MAIN);

    {
        u8 end = (u8)(nav->page_top + nav->page_size);
        if (end > team->player_count) end = team->player_count;

        for (i = nav->page_top; i < end; i++) {
            Player *pl  = &g_players[(u16)team->player_start + i];
            long    min = economy_min_salary(pl->strength);
            u16     pal = (i == nav->selected) ? PAL_SELECTED : PAL_MAIN;

            if (pal == PAL_SELECTED)
                render_fill_rect(BG_A, 1u, row, 38u, 1u, PAL_SELECTED, 0u);
            render_textf(BG_A, 1u, row, pal,
                         " %-14s %s %3u %8ld %7ld",
                         pl->name, s_pos_abbr[pl->pos],
                         (u16)pl->strength, min, pl->salary);
            row++;
        }

        if (nav->page_top > 0u)
            render_text(BG_A, "^", 39u, (u16)(CONTENT_ROW_FIRST + 4u), PAL_MAIN);
        if (end < team->player_count)
            render_text(BG_A, "v", 39u, (u16)(row - 1u), PAL_MAIN);
    }

    /* Ordenado m?nimo do jogador seleccionado                        */
    {
        Player *sel_pl = &g_players[(u16)team->player_start + nav->selected];
        render_hline(BG_A, 1u, (u16)(CONTENT_ROW_LAST - 2u),
                     38u, BOX_SIMPLE, PAL_MAIN);
        render_textf(BG_A, 1u, (u16)(CONTENT_ROW_LAST - 1u), PAL_MAIN,
                     "ORDENADO MINIMO: %ld  Equipa: %-12s",
                     economy_min_salary(sel_pl->strength),
                     team->name);
    }

    render_help_bar("[A] Vender  [^v] Navegar  [B] Voltar", NULL);
}

/* ------------------------------------------------------------------ */
/* screen_transfers()                                                  */
/* ------------------------------------------------------------------ */

void screen_transfers(void) {
    Team   *team = &g_teams[g_player_team_idx];
    ListNav nav;
    u8      needs_redraw = 1u;
    u8      page_size    = (u8)(CONTENT_ROW_LAST - CONTENT_ROW_FIRST - 6u);

    list_nav_init(&nav, team->player_count, page_size);

    for (;;) {
        SYS_doVBlankProcess();
        input_update();

        if (input_pressed(BTN_CANCEL)) return;

        if (list_nav_update(&nav)) needs_redraw = 1u;

        /* Vender jogador seleccionado                                */
        if (input_pressed(BTN_CONFIRM)) {
            u16  pi      = (u16)team->player_start + nav.selected;
            u8   winner;
            u8   total   = data_get_team_player_count(g_player_team_idx);
            u8   gk_cnt  = data_get_team_gk_count(g_player_team_idx);
            Player *pl   = &g_players[pi];

            /* Verifica restri??es de plantel                         */
            if (total <= (u8)MIN_SQUAD_SIZE) {
                render_text(BG_A,
                    "Como so possui 14 jogadores na equipa.",
                    1u, (u16)(CONTENT_ROW_LAST - 1u), PAL_MAIN);
                {
                    u8 t;
                    for (t = 0u; t < 120u; t++) SYS_doVBlankProcess();
                }
                needs_redraw = 1u;
                continue;
            }
            if (pl->pos == (u8)POS_GR && gk_cnt <= (u8)MIN_GK_COUNT) {
                render_text(BG_A,
                    "Como so possui um guarda-redes na equipa.",
                    1u, (u16)(CONTENT_ROW_LAST - 1u), PAL_MAIN);
                {
                    u8 t;
                    for (t = 0u; t < 120u; t++) SYS_doVBlankProcess();
                }
                needs_redraw = 1u;
                continue;
            }

            /* Lan?a leil?o                                            */
            winner = transfer_auction(pi, g_player_team_idx);

            /* Exibe resultado -- fiel ao original                     */
            render_clear_content();
            render_text(BG_A, "VENDA PELA MELHOR OFERTA DE ORDENADO",
                        1u, (u16)CONTENT_ROW_FIRST, PAL_MAIN);
            render_hline(BG_A, 1u, (u16)(CONTENT_ROW_FIRST + 1u),
                         38u, BOX_SIMPLE, PAL_MAIN);

            if (winner != 0xFFu) {
                render_textf(BG_A, 2u, (u16)(CONTENT_ROW_FIRST + 3u),
                             PAL_MAIN,
                             "TRANSFERIDO PARA O %s",
                             g_teams[winner].name);
                /* O novo ordenado ? o sal?rio ap?s a transfer?ncia   */
                render_textf(BG_A, 2u, (u16)(CONTENT_ROW_FIRST + 4u),
                             PAL_MAIN,
                             "NOVO ORDENADO: %ld esc.",
                             g_players[pi].salary);
                /* Sincroniza dinheiro                                */
                g_money = team->money;
                /* O array foi compactado -- ajusta nav                */
                if (nav.selected >= team->player_count && nav.selected > 0u)
                    nav.selected--;
                nav.count = team->player_count;
            } else {
                render_text(BG_A, "NAO HOUVE OFERTAS",
                            2u, (u16)(CONTENT_ROW_FIRST + 3u), PAL_MAIN);
            }

            render_help_bar("[A/B] Continuar", NULL);
            for (;;) {
                SYS_doVBlankProcess();
                input_update();
                if (input_pressed(BTN_CONFIRM) || input_pressed(BTN_CANCEL))
                    break;
            }
            nav.count = team->player_count;
            needs_redraw = 1u;
        }

        if (!needs_redraw) continue;
        needs_redraw = 0u;
        nav.count = team->player_count;
        render_transfers(g_player_team_idx, &nav);
    }
}
