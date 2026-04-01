/*
 * screens/title.c -- Tela de t?tulo e sele??o de equipe
 *
 * Esta implementa??o serve dois prop?sitos:
 *   1. Tela de t?tulo funcional para Fase 0 (sele??o de equipe com ListNav).
 *   2. Tela de teste de paleta integrada (activar com BTN_ACTION na entrada).
 *
 * A tela de teste de paleta valida visualmente as 16 cores CGA->Genesis
 * antes de avan?ar para a Fase 1 -- ? a primeira coisa a verificar no emulador.
 *
 * Cores CGA usadas nesta tela (conforme design original):
 *   T?tulo "ELIFOOT II": amarelo (PAL0[3]) sobre preto
 *   Lista de equipes: branco (PAL0[2]) sobre preto
 *   Equipe selecionada: preto (PAL1[1]) sobre ciano (PAL1[0])
 *   Instru??o "[A] Selecionar": ciano brilhante (PAL0[4])
 */

#include <genesis.h>
#include "title.h"
#include "../game/data.h"
#include "../game/types.h"
#include "../engine/input.h"
#include "../engine/render.h"

/* ------------------------------------------------------------------ */
/* Tela de teste de paleta                                             */
/* ------------------------------------------------------------------ */

/*
 * render_palette_test() -- Valida visualmente as cores CGA->Genesis.
 *
 * Mostra cada entrada das paletas PAL0 e PAL1 como bloco de texto
 * colorido. Comparar com a tabela de refer?ncia em game/types.h e
 * no plano de port Se??o 4.2.
 *
 * Activar: pressionar BTN_ACTION (C) na tela de t?tulo.
 * Sair: pressionar qualquer bot?o.
 */
static void render_palette_test(void) {
    static const char * const pal0_labels[16] = {
        "[0] Preto       ", "[1] Azul esc.   ",
        "[2] Branco      ", "[3] Amarelo     ",
        "[4] Ciano br.   ", "[5] Verde br.   ",
        "[6] Vermelho br.", "[7] Cinza claro ",
        "[8] Magenta     ", "[9] Ciano esc.  ",
        "[A] Cinza esc.  ", "[B] Magenta br. ",
        "[C] Marrom      ", "[D] Verde esc.  ",
        "[E] Azul br.    ", "[F] Branco dup. ",
    };

    u8 i;

    render_clear_content();
    render_set_bg_color(0u);  /* fundo preto                           */

    render_text(BG_A, "=== TESTE DE PALETA CGA -> GENESIS ===",
                1u, CONTENT_ROW_FIRST, PAL_MAIN);

    /* PAL0: cada entrada como texto sobre fundo preto.
     * Como a fonte s? tem 2 cores (0=bg, 1=fg), cada entrada de paleta
     * ? usada como paleta do tile -- a cor do texto corresponde a PAL0[i].
     * NOTA: s? podemos mostrar a cor [2] de PAL0 com PAL_MAIN (branco),
     * pois render_text() usa a paleta inteira. Esta tela usa PAL_MAIN e
     * PAL_SELECTED para demonstrar os dois esquemas visuais principais.   */

    render_text(BG_A, "PAL0 (fundo preto):",
                2u, (u16)(CONTENT_ROW_FIRST + 2u), PAL_MAIN);

    /* Mostra blocos de cor com PAL0 e PAL1 alternados para demonstrar
     * a diferen?a visual. Na ROM real, cada bloco de texto aparece
     * na cor definida pela paleta selecionada para aquele tile.         */
    for (i = 0u; i < 8u; i++) {
        u16 row = (u16)(CONTENT_ROW_FIRST + 4u + i);
        render_text(BG_A, pal0_labels[i],     2u, row, PAL_MAIN);
        render_text(BG_A, pal0_labels[i + 8u], 22u, row, PAL_MAIN);
    }

    render_text(BG_A, "PAL1 (fundo ciano):",
                2u, (u16)(CONTENT_ROW_FIRST + 13u), PAL_MAIN);

    /* Demonstra PAL1: texto preto sobre ciano (cursor de sele??o)     */
    render_text_pad(BG_A, "  Item selecionado  ",
                    2u, (u16)(CONTENT_ROW_FIRST + 15u), 20u, PAL_SELECTED);
    render_text_pad(BG_A, "  Status bar        ",
                    2u, (u16)(CONTENT_ROW_FIRST + 16u), 20u, PAL_SELECTED);

    render_text(BG_A, "Pressione qualquer botao para voltar",
                2u, (u16)(CONTENT_ROW_FIRST + 19u), PAL_MAIN);

    render_help_bar("[Qualquer] Voltar ao menu", (const char *)0);
}

/* ------------------------------------------------------------------ */
/* screen_title()                                                      */
/* ------------------------------------------------------------------ */

void screen_title(void) {
    KLog("screen_title: START");
    ListNav nav;
    u8 needs_redraw = 1u;
    u8 in_palette_test = 0u;
    u16 i;

    KLog_U1("screen_title: TEAM_COUNT=", (u32)TEAM_COUNT);
    list_nav_init(&nav, (u8)TEAM_COUNT, (u8)18u);  /* 18 linhas vis?veis */

    KLog("screen_title: calling render_set_bg_color");
    render_set_bg_color(0u);   /* fundo azul escuro (CGA cor 1)        */
    KLog("screen_title: calling render_clear_content");
    render_clear_content();
    KLog("screen_title: entering main loop");

    for (;;) {
        /* ---- Input ---- */
        KLog("screen_title: top of loop, calling SYS_doVBlankProcess");
        SYS_doVBlankProcess();
        KLog("screen_title: SYS_doVBlankProcess done, calling input_update");
        input_update();
        KLog("screen_title: input_update done");

        if (in_palette_test) {
            if (input_any_pressed()) {
                in_palette_test = 0u;
                needs_redraw = 1u;
                KLog("screen_title: calling render_set_bg_color");
    render_set_bg_color(0u);
            }
            continue;
        }

        /* BTN_ACTION (C) -> tela de teste de paleta                   */
        if (input_pressed(BTN_ACTION)) {
            in_palette_test = 1u;
            render_palette_test();
            continue;
        }

        /* Confirmar sele??o                                           */
        if (input_pressed(BTN_CONFIRM)) {
            g_player_team_idx = nav.selected;
            g_division        = g_teams[nav.selected].division;
            g_money           = g_teams[nav.selected].money;
            return;
        }

        /* Navega lista                                                */
        if (list_nav_update(&nav)) {
            needs_redraw = 1u;
        }

        /* ---- Render (s? quando necess?rio) ---- */
        if (!needs_redraw) continue;
        needs_redraw = 0u;
        KLog("screen_title: calling render_clear_content inside loop");
        render_clear_content();
        KLog("screen_title: render_clear_content done");

        KLog("screen_title: rendering title text - calling render_text row 4");
        /* TEST: write at row 0 to confirm PAL_MAIN text visible */
        render_text(BG_A, "ELI", 0u, 0u, PAL_MAIN);
        render_text(BG_A, "ELIFOOT II",
                    15u, (u16)(CONTENT_ROW_FIRST + 1u), PAL_MAIN);
        KLog_U1("screen_title: title written at row", (u32)(CONTENT_ROW_FIRST + 1u));
        render_text(BG_A, "Selecione a sua equipa:",
                    9u, (u16)(CONTENT_ROW_FIRST + 3u), PAL_MAIN);

        /* Separador                                                   */
        render_hline(BG_A, 1u, (u16)(CONTENT_ROW_FIRST + 4u),
                     38u, BOX_SIMPLE, PAL_MAIN);

        KLog("screen_title: starting team list render");
        /* Lista de equipes com scroll                                 */
        for (i = 0u; i < nav.page_size; i++) {
            u16 team_idx = (u16)(nav.page_top + i);
            u16 row = (u16)(CONTENT_ROW_FIRST + 5u + i);

            if (team_idx >= (u16)TEAM_COUNT) break;

            /* Linha selecionada: ciano (PAL1), resto: branco (PAL0)   */
            if (team_idx == (u16)nav.selected) {
                /* Preenche linha inteira com ciano e escreve texto    */
                render_fill_rect(BG_A, 1u, row, 38u, 1u,
                                 PAL_SELECTED, 0u);
                render_textf(BG_A, 3u, row, PAL_SELECTED,
                             "%2u. %s",
                             team_idx + 1u,
                             g_teams[team_idx].name);
            } else {
                render_clear_rect(BG_A, 1u, row, 38u, 1u);
                render_textf(BG_A, 3u, row, PAL_MAIN,
                             "%2u. %s",
                             team_idx + 1u,
                             g_teams[team_idx].name);
            }
        }

        /* Indicador de scroll                                         */
        if (nav.page_top > 0u) {
            render_text(BG_A, "^", 39u,
                        (u16)(CONTENT_ROW_FIRST + 5u), PAL_MAIN);
        }
        if ((u16)(nav.page_top + nav.page_size) < (u16)TEAM_COUNT) {
            render_text(BG_A, "v", 39u,
                        (u16)(CONTENT_ROW_FIRST + 5u + nav.page_size - 1u),
                        PAL_MAIN);
        }

        KLog("screen_title: render complete, calling help bar");
        /* Help bar contextual                                         */
        render_help_bar("[A]Selecionar  [C]Teste de cores  [^v]Navegar",
                        (const char *)0);
    }
}
