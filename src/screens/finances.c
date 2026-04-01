/*
 * screens/finances.c -- Finan?as
 *
 * Sub-tela A -- ORDENADOS:
 *   Lista jogadores com nome, posi??o, for?a, sal?rio actual.
 *   O jogador pode alterar o sal?rio de qualquer jogador.
 *   Se reduzir abaixo do m?nimo -> "Nem pensar!"
 *   Confirma??o de aumento: [A]=Aceitar [B]=Recusar
 *
 * Sub-tela B -- RECEITAS:
 *   Dinheiro actual, total de ordenados, saldo previsional.
 *
 * Sub-tela C -- ESTADIO:
 *   Capacidade actual, pre?o dos bilhetes, custo de expans?o.
 *   Constru??o de bancada: +5000 lugares por 50000 esc.
 */

#include <genesis.h>
#include "finances.h"
#include "../game/data.h"
#include "../game/economy.h"
#include "../game/types.h"
#include "../engine/input.h"
#include "../engine/render.h"

/* ------------------------------------------------------------------ */
/* Sub-tela: Ordenados                                                 */
/* ------------------------------------------------------------------ */

static const char * const s_pos_abbr[4] = { "GR", "DF", "MD", "AV" };

static void screen_finances_salaries(void) {
    Team   *team = &g_teams[g_player_team_idx];
    ListNav nav;
    u8      needs_redraw = 1u;
    u8      page_size    = (u8)(CONTENT_ROW_LAST - CONTENT_ROW_FIRST - 5u);

    list_nav_init(&nav, team->player_count, page_size);

    for (;;) {
        SYS_doVBlankProcess();
        input_update();

        if (input_pressed(BTN_CANCEL)) return;

        if (list_nav_update(&nav)) needs_redraw = 1u;

        /* Alterar ordenado do jogador seleccionado                   */
        if (input_pressed(BTN_CONFIRM)) {
            u16     pi = (u16)team->player_start + nav.selected;
            Player *pl = &g_players[pi];
            long    min_sal = economy_min_salary(pl->strength);
            long    new_sal;
            u16     row_inp = (u16)(CONTENT_ROW_LAST - 2u);

            /* Prompt de novo ordenado                                */
            render_fill_rect(BG_A, 1u, row_inp, 38u, 1u, PAL_SELECTED, 0u);
            render_textf(BG_A, 1u, row_inp, PAL_SELECTED,
                         "Novo ordenado p/ %-10s (min %ld):",
                         pl->name, min_sal);

            /* Entrada num?rica simples via D-pad (incrementa/decrementa) */
            new_sal = pl->salary;
            for (;;) {
                SYS_doVBlankProcess();
                input_update();
                if (input_repeat(BTN_UP))   new_sal += 100L;
                if (input_repeat(BTN_DOWN) && new_sal > 100L) new_sal -= 100L;
                render_fill_rect(BG_A, 1u, (u16)(row_inp + 1u),
                                 38u, 1u, PAL_SELECTED, 0u);
                render_textf(BG_A, 1u, (u16)(row_inp + 1u), PAL_SELECTED,
                             "  [^v] ajustar: %ld esc.  [A]OK [B]Cancela",
                             new_sal);
                if (input_pressed(BTN_CONFIRM)) {
                    if (new_sal < min_sal) {
                        /* "Nem pensar!" -- fiel ao original           */
                        render_text(BG_A,
                            "  Nem pensar!                           ",
                            1u, (u16)(row_inp + 1u), PAL_MAIN);
                        /* Pausa breve                                */
                        {
                            u8 t;
                            for (t = 0u; t < 90u; t++) SYS_doVBlankProcess();
                        }
                    } else {
                        team->salary_total -= pl->salary;
                        pl->salary          = new_sal;
                        team->salary_total += new_sal;
                        needs_redraw = 1u;
                    }
                    break;
                }
                if (input_pressed(BTN_CANCEL)) break;
            }
            needs_redraw = 1u;
        }

        if (!needs_redraw) continue;
        needs_redraw = 0u;

        render_clear_content();
        {
            u16 row = (u16)CONTENT_ROW_FIRST;
            render_textf(BG_A, 1u, row++, PAL_MAIN,
                         "ORDENADOS -- %-16s", team->name);
            render_textf(BG_A, 1u, row++, PAL_MAIN,
                         "Total mensal: %ld  Disponivel: %ld",
                         team->salary_total, team->money);
            render_hline(BG_A, 1u, row++, 38u, BOX_SIMPLE, PAL_MAIN);
            render_text(BG_A,
                        " Nome            Pos Frc   Ordenado  Min",
                        1u, row++, PAL_MAIN);
            render_hline(BG_A, 1u, row++, 38u, BOX_SIMPLE, PAL_MAIN);

            {
                u8 i;
                u8 end = (u8)(nav.page_top + nav.page_size);
                if (end > team->player_count) end = team->player_count;

                for (i = nav.page_top; i < end; i++) {
                    Player *pl  = &g_players[(u16)team->player_start + i];
                    long    min = economy_min_salary(pl->strength);
                    u16     pal = (i == nav.selected) ? PAL_SELECTED : PAL_MAIN;
                    u16     sal_pal = (pl->salary < min) ? PAL_MAIN : PAL_MAIN;

                    if (pal == PAL_SELECTED)
                        render_fill_rect(BG_A, 1u, row, 38u, 1u, PAL_SELECTED, 0u);
                    render_textf(BG_A, 1u, row, pal,
                                 " %-14s %s %3u %8ld %5ld",
                                 pl->name, s_pos_abbr[pl->pos],
                                 (u16)pl->strength, pl->salary, min);
                    if (pl->salary < min && pal != PAL_SELECTED) {
                        /* Indica sal?rio abaixo do m?nimo            */
                        render_text(BG_A, "!", 39u, row, PAL_MAIN);
                    }
                    (void)sal_pal;
                    row++;
                }
            }
        }
        render_help_bar("[A]Alterar  [^v]Navegar  [B]Voltar", NULL);
    }
}

/* ------------------------------------------------------------------ */
/* Sub-tela: Receitas                                                  */
/* ------------------------------------------------------------------ */

static void screen_finances_revenues(void) {
    Team *team = &g_teams[g_player_team_idx];
    u16   row;

    render_clear_content();
    row = (u16)CONTENT_ROW_FIRST;

    render_textf(BG_A, 1u, row++, PAL_MAIN,
                 "FINANCAS -- %-16s", team->name);
    render_hline(BG_A, 1u, row++, 38u, BOX_SIMPLE, PAL_MAIN);

    render_textf(BG_A, 2u, row++, PAL_MAIN,
                 "Dinheiro actual  : %ld esc.", team->money);
    render_textf(BG_A, 2u, row++, PAL_MAIN,
                 "Total ordenados  : %ld esc./mes", team->salary_total);
    {
        long saldo = team->money - team->salary_total;
        render_textf(BG_A, 2u, row++, PAL_MAIN,
                     "Saldo pos-ord.   : %ld esc.", saldo);
    }
    row++;
    render_textf(BG_A, 2u, row++, PAL_MAIN,
                 "Divisao          : Div%u", (u16)(team->division + 1u));
    render_textf(BG_A, 2u, row++, PAL_MAIN,
                 "Treinador        : %-16s",
                 g_coaches[team->coach_idx]);
    render_textf(BG_A, 2u, row++, PAL_MAIN,
                 "Jogadores        : %u", (u16)team->player_count);
    row++;
    render_textf(BG_A, 2u, row++, PAL_MAIN,
                 "V:%u E:%u D:%u  GF:%u GA:%u  Pts:%u",
                 (u16)team->wins, (u16)team->draws, (u16)team->losses,
                 (u16)team->goals_for, (u16)team->goals_against,
                 (u16)team->points);

    render_help_bar("[B] Voltar", NULL);

    for (;;) {
        SYS_doVBlankProcess();
        input_update();
        if (input_pressed(BTN_CANCEL) || input_pressed(BTN_CONFIRM)) return;
    }
}

/* ------------------------------------------------------------------ */
/* Sub-tela: Est?dio                                                   */
/* ------------------------------------------------------------------ */

#define STADIUM_EXPANSION_CAPACITY  5000u
#define STADIUM_EXPANSION_COST     50000L

static void screen_finances_stadium(void) {
    Team  *team = &g_teams[g_player_team_idx];
    u8     needs_redraw = 1u;

    for (;;) {
        SYS_doVBlankProcess();
        input_update();

        if (input_pressed(BTN_CANCEL)) return;

        /* Construir bancada (A)                                       */
        if (input_pressed(BTN_CONFIRM)) {
            if (team->money >= STADIUM_EXPANSION_COST) {
                team->money        -= STADIUM_EXPANSION_COST;
                team->stadium_cap  += (u16)STADIUM_EXPANSION_CAPACITY;
                g_money             = team->money;
                needs_redraw = 1u;
            } else {
                /* Sem dinheiro suficiente                             */
                render_text(BG_A,
                    "Dinheiro insuficiente!              ",
                    2u, (u16)(CONTENT_ROW_LAST - 1u), PAL_MAIN);
            }
        }

        /* Alterar pre?o bilhete (Up/Down)                            */
        if (input_repeat(BTN_UP) && team->ticket_price < 50u) {
            team->ticket_price++;
            needs_redraw = 1u;
        }
        if (input_repeat(BTN_DOWN) && team->ticket_price > 1u) {
            team->ticket_price--;
            needs_redraw = 1u;
        }

        if (!needs_redraw) continue;
        needs_redraw = 0u;

        render_clear_content();
        {
            u16 row = (u16)CONTENT_ROW_FIRST;
            u16 cap = team->stadium_cap;

            render_textf(BG_A, 1u, row++, PAL_MAIN,
                         "ESTADIO -- %-16s", team->name);
            render_hline(BG_A, 1u, row++, 38u, BOX_SIMPLE, PAL_MAIN);

            render_textf(BG_A, 2u, row++, PAL_MAIN,
                         "Capacidade     : %u lugares", (u16)cap);
            render_textf(BG_A, 2u, row++, PAL_MAIN,
                         "Preco bilhete  : %u esc.  [^v] alterar",
                         (u16)team->ticket_price);
            render_textf(BG_A, 2u, row++, PAL_MAIN,
                         "Receita maxima : %ld esc.",
                         (long)cap * (long)team->ticket_price);
            row++;
            render_textf(BG_A, 2u, row++, PAL_MAIN,
                         "Expansao (+%u lugares): %ld esc.",
                         (u16)STADIUM_EXPANSION_CAPACITY,
                         STADIUM_EXPANSION_COST);
            render_textf(BG_A, 2u, row++, PAL_MAIN,
                         "Dinheiro disponivel: %ld esc.", team->money);
            row++;
            if (team->money >= STADIUM_EXPANSION_COST) {
                render_text(BG_A,
                    "[A] Construir bancada",
                    2u, row, PAL_MAIN);
            } else {
                render_text(BG_A,
                    "[A] Construir bancada (sem fundos)",
                    2u, row, PAL_MAIN);
            }
        }
        render_help_bar("[A]Construir  [^v]Preco bilhete  [B]Voltar", NULL);
    }
}

/* ------------------------------------------------------------------ */
/* screen_finances() -- menu de tabs                                    */
/* ------------------------------------------------------------------ */

void screen_finances(void) {
    for (;;) {
        render_clear_content();
        {
            u16 row = (u16)CONTENT_ROW_FIRST;
            render_text(BG_A, "FINANCAS", 16u, row++, PAL_MAIN);
            render_hline(BG_A, 1u, row++, 38u, BOX_SIMPLE, PAL_MAIN);
            row++;
            render_text(BG_A, "[A] Ordenados dos jogadores",
                        4u, row++, PAL_MAIN);
            render_text(BG_A, "[B] Resumo financeiro",
                        4u, row++, PAL_MAIN);
            render_text(BG_A, "[C] Estadio e bilhetes",
                        4u, row++, PAL_MAIN);
            row++;
            render_text(BG_A, "[Start] Voltar ao menu",
                        4u, row, PAL_MAIN);
        }
        render_help_bar("[A]Ordenados [B]Receitas [C]Estadio [Start]Sair",
                        NULL);

        for (;;) {
            SYS_doVBlankProcess();
            input_update();
            if (input_pressed(BTN_CONFIRM)) {
                screen_finances_salaries();
                break;
            }
            if (input_pressed(BTN_CANCEL)) {
                screen_finances_revenues();
                break;
            }
            if (input_pressed(BTN_ACTION)) {
                screen_finances_stadium();
                break;
            }
            if (input_pressed(BTN_START)) return;
        }
    }
}
