/*
 * screens/squad.c -- Gest?o de plantel e forma??o
 *
 * Layout da tela (40?22 linhas de conte?do):
 *
 *   Linha  3: "PLANTEL: SAO PAULO    Formacao: 4-4-2"
 *   Linha  4: "?????????????????????????????????????"
 *   Linha  5: "[*] Zetti         GR  85  1500 esc"  <- titular
 *   Linha  6: "[ ] Rogerio       GR  72  1200 esc"  <- banco
 *   ...
 *   Linha 20: "?????????????????????????????????????"
 *   Linha 21: "Em campo: 11  Banco: 5"
 *   Linha 22: "GR:1 DF:4 MD:4 AV:2"
 *
 *   Help bar: [A]Campo/Banco  [Y]Formacao  [C]Auto  [B]Voltar
 *
 * Cores:
 *   Titular:  branco (PAL0)
 *   Banco:    cinza claro (PAL0[7])
 *   Selecionado: preto sobre ciano (PAL1)
 *   GR:  PAL0[4] ciano
 *   DF:  PAL0[2] branco
 *   MD:  PAL0[5] verde
 *   AV:  PAL0[6] vermelho
 */

#include <genesis.h>
#include "squad.h"
#include "../game/data.h"
#include "../game/types.h"
#include "../engine/input.h"
#include "../engine/render.h"

/* ------------------------------------------------------------------ */
/* Constantes                                                          */
/* ------------------------------------------------------------------ */

/* Nomes das forma??es (fiel ao original -- F1=3-4-3, F3=4-4-2, etc.) */
static const char * const s_form_names[FORM_COUNT] = {
    "4-4-2", "4-3-3", "3-5-2", "5-3-2",
    "4-5-1", "3-4-3", "5-4-1", "4-2-4",
    "3-3-4", "5-2-3",
};

/* Distribui??o GR/DF/MD/AV por forma??o [GR ? sempre 1]             */
/*                          GR DF MD AV */
static const u8 s_form_dist[FORM_COUNT][4] = {
    {1, 4, 4, 2},  /* 4-4-2 */
    {1, 4, 3, 3},  /* 4-3-3 */
    {1, 3, 5, 2},  /* 3-5-2 */
    {1, 5, 3, 2},  /* 5-3-2 */
    {1, 4, 5, 1},  /* 4-5-1 */
    {1, 3, 4, 3},  /* 3-4-3 */
    {1, 5, 4, 1},  /* 5-4-1 */
    {1, 4, 2, 4},  /* 4-2-4 */
    {1, 3, 3, 4},  /* 3-3-4 */
    {1, 5, 2, 3},  /* 5-2-3 */
};

/* Abreviaturas de posi??o para display                               */
static const char * const s_pos_abbr[4] = { "GR", "DF", "MD", "AV" };

/* Paleta por posi??o                                                  */
static const u16 s_pos_pal[4] = {
    PAL_MAIN,   /* GR: usaremos FG_CYAN_BR mas simplificamos p/ PAL0  */
    PAL_MAIN,   /* DF: branco                                          */
    PAL_MAIN,   /* MD: branco (Fase 3+: verde)                        */
    PAL_MAIN,   /* AV: branco (Fase 3+: vermelho)                     */
};

/* ------------------------------------------------------------------ */
/* Auto-escalar                                                        */
/* ------------------------------------------------------------------ */

static void auto_select(u8 team_idx) {
    /*
     * Selecciona os 11 melhores jogadores para a forma??o actual,
     * respeitando a distribui??o GR/DF/MD/AV.
     * Algoritmo: para cada posi??o, coloca em campo os N mais fortes.
     */
    Team *team = &g_teams[team_idx];
    Formation form = (Formation)team->formation;
    u8 targets[4];
    u8 placed[4];
    u8 i, pos;

    targets[0] = s_form_dist[form][0];
    targets[1] = s_form_dist[form][1];
    targets[2] = s_form_dist[form][2];
    targets[3] = s_form_dist[form][3];

    placed[0] = placed[1] = placed[2] = placed[3] = 0u;

    /* Primeiro: tira todos de campo                                   */
    for (i = 0u; i < team->player_count; i++) {
        g_players[(u16)team->player_start + i].on_field = 0u;
    }

    /* Para cada posi??o: coloca os N mais fortes                      */
    for (pos = 0u; pos < 4u; pos++) {
        u8 need = targets[pos];
        u8 done = 0u;

        /* Passagem simples: encontra o mais forte ainda n?o colocado */
        while (done < need) {
            u8  best_i    = 0xFFu;
            u8  best_str  = 0u;
            u8  found     = 0u;

            for (i = 0u; i < team->player_count; i++) {
                Player *pl = &g_players[(u16)team->player_start + i];
                if (pl->pos != pos)        continue;
                if (pl->on_field)          continue;  /* j? colocado  */
                if (pl->strength > best_str || !found) {
                    best_str = pl->strength;
                    best_i   = i;
                    found    = 1u;
                }
            }
            if (!found) break;
            g_players[(u16)team->player_start + best_i].on_field = 1u;
            done++;
        }
        placed[pos] = done;
    }
    (void)placed;
}

/* ------------------------------------------------------------------ */
/* Render da tela de plantel                                           */
/* ------------------------------------------------------------------ */

static void render_squad(u8 team_idx, u8 sel, u8 scroll_top) {
    Team  *team = &g_teams[team_idx];
    u16    row  = (u16)CONTENT_ROW_FIRST;
    u8     i;
    u8     on_field = 0u, on_bench = 0u;
    u8     pos_count[4] = {0u, 0u, 0u, 0u};

    render_clear_content();

    /* T?tulo                                                          */
    render_textf(BG_A, 1u, row, PAL_MAIN,
                 "PLANTEL: %-16s Form: %s",
                 team->name,
                 s_form_names[team->formation]);
    row++;
    render_hline(BG_A, 1u, row++, 38u, BOX_SIMPLE, PAL_MAIN);

    /* Cabe?alho de colunas                                            */
    render_text(BG_A, " T  Nome            Pos Forca Ordenado",
                1u, row++, PAL_MAIN);
    render_hline(BG_A, 1u, row++, 38u, BOX_SIMPLE, PAL_MAIN);

    /* Lista de jogadores (scroll de PAGE_SIZE elementos)             */
    {
        u8 page_size = (u8)(CONTENT_ROW_LAST - (u8)row - 3u);
        u8 end = (u8)(scroll_top + page_size);
        if (end > team->player_count) end = team->player_count;

        for (i = scroll_top; i < end; i++) {
            Player *pl  = &g_players[(u16)team->player_start + i];
            u16     pal = (i == sel) ? PAL_SELECTED : PAL_MAIN;
            char    marker = pl->on_field ? '*' : ' ';

            if (pal == PAL_SELECTED) {
                render_fill_rect(BG_A, 1u, row, 38u, 1u, PAL_SELECTED, 0u);
            }
            render_textf(BG_A, 1u, row, pal,
                         "[%c] %-14s %s  %3u %6ld",
                         marker,
                         pl->name,
                         s_pos_abbr[pl->pos],
                         (u16)pl->strength,
                         pl->salary);
            row++;
        }

        /* Indicadores de scroll                                      */
        if (scroll_top > 0u) {
            render_text(BG_A, "^", 39u,
                        (u16)(CONTENT_ROW_FIRST + 4u), PAL_MAIN);
        }
        if (end < team->player_count) {
            render_text(BG_A, "v", 39u, (u16)(row - 1u), PAL_MAIN);
        }
    }

    /* Contagens                                                       */
    for (i = 0u; i < team->player_count; i++) {
        Player *pl = &g_players[(u16)team->player_start + i];
        if (pl->on_field) { on_field++; pos_count[pl->pos]++; }
        else               on_bench++;
    }

    render_hline(BG_A, 1u, (u16)(CONTENT_ROW_LAST - 1u),
                 38u, BOX_SIMPLE, PAL_MAIN);
    render_textf(BG_A, 1u, (u16)CONTENT_ROW_LAST, PAL_MAIN,
                 "Campo:%2u  Banco:%2u  GR:%u DF:%u MD:%u AV:%u",
                 (u16)on_field, (u16)on_bench,
                 (u16)pos_count[0], (u16)pos_count[1],
                 (u16)pos_count[2], (u16)pos_count[3]);

    render_help_bar("[A]Campo/Banco [Y]Formacao [C]Auto [B]Voltar", NULL);
}

/* ------------------------------------------------------------------ */
/* screen_squad()                                                      */
/* ------------------------------------------------------------------ */

void screen_squad(void) {
    Team  *team    = &g_teams[g_player_team_idx];
    ListNav nav;
    u8 needs_redraw = 1u;
    u8 scroll_top   = 0u;
    u8 page_size    = (u8)(CONTENT_ROW_LAST - CONTENT_ROW_FIRST - 4u);

    list_nav_init(&nav, team->player_count, page_size);

    for (;;) {
        SYS_doVBlankProcess();
        input_update();

        /* Voltar                                                      */
        if (input_pressed(BTN_CANCEL)) return;

        /* Mudar forma??o (Y)                                         */
        if (input_pressed(BTN_FORMATION)) {
            team->formation = (u8)((team->formation + 1u) % (u8)FORM_COUNT);
            needs_redraw = 1u;
        }

        /* Auto-escalar (C)                                           */
        if (input_pressed(BTN_ACTION)) {
            auto_select(g_player_team_idx);
            needs_redraw = 1u;
        }

        /* Navega lista                                                */
        if (list_nav_update(&nav)) {
            scroll_top   = nav.page_top;
            needs_redraw = 1u;
        }

        /* Colocar/tirar do campo (A)                                 */
        if (input_pressed(BTN_CONFIRM)) {
            Player *pl = &g_players[(u16)team->player_start + nav.selected];
            u8      on = pl->on_field;

            if (on) {
                /* Tirar do campo -- verificar m?nimos                  */
                /* M?nimo 1 GR sempre                                 */
                u8 gk_on = 0u;
                u8 j;
                for (j = 0u; j < team->player_count; j++) {
                    Player *p2 = &g_players[(u16)team->player_start + j];
                    if (p2->on_field && p2->pos == (u8)POS_GR) gk_on++;
                }
                if (pl->pos == (u8)POS_GR && gk_on <= 1u) {
                    /* N?o pode retirar o ?nico GR -- feedback visual  */
                    render_text(BG_A,
                        "Precisa de pelo menos 1 GR em campo!",
                        1u, (u16)(CONTENT_ROW_LAST - 2u), PAL_MAIN);
                } else {
                    pl->on_field = 0u;
                    needs_redraw = 1u;
                }
            } else {
                /* P?r em campo -- verificar se j? tem 11             */
                u8 total_on = 0u, j;
                for (j = 0u; j < team->player_count; j++) {
                    if (g_players[(u16)team->player_start + j].on_field)
                        total_on++;
                }
                if (total_on >= 11u) {
                    render_text(BG_A,
                        "Ja tem 11 jogadores em campo!        ",
                        1u, (u16)(CONTENT_ROW_LAST - 2u), PAL_MAIN);
                } else {
                    pl->on_field = 1u;
                    needs_redraw = 1u;
                }
            }
        }

        if (needs_redraw) {
            needs_redraw = 0u;
            render_squad(g_player_team_idx, nav.selected, scroll_top);
        }
    }
}
