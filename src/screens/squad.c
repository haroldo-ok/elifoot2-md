/*
 * screens/squad.c -- Plantel com dois paineis (Campo / Banco) e substituicoes
 *
 * Original: JOGADORES EM CAMPO | JOGADORES NO BANCO
 *           F1 = Substituir (troca entre paineis)
 *           Esc = Fim
 *
 * Genesis mapping:
 *   A     = alternar painel activo (Campo / Banco)
 *   C     = ciclar formacao + auto-escalar
 *   X     = auto-escalar sem mudar formacao
 *   START = sair
 *   B     = sair
 *   CIMA/BAIXO = navegar dentro do painel
 *   Confirmar no painel Banco enquanto titular seleccionado = substituir
 */

#include <genesis.h>
#include "squad.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"
#include "../game/types.h"

static const char s_pos[4][3] = { "GR", "DF", "MD", "AV" };

static const char s_form_names[FORM_COUNT][6] = {
    "4-4-2", "4-3-3", "3-5-2", "5-3-2", "4-5-1",
    "3-4-3", "5-4-1", "4-2-4", "3-3-4", "5-2-3"
};

static const u8 s_form_slots[FORM_COUNT][4] = {
    {1,4,4,2}, {1,4,3,3}, {1,3,5,2}, {1,5,3,2}, {1,4,5,1},
    {1,3,4,3}, {1,5,4,1}, {1,4,2,4}, {1,3,3,4}, {1,5,2,3}
};

static void auto_select(u8 team_idx) {
    Team *team = &g_teams[team_idx];
    u8    fn   = (team->formation < (u8)FORM_COUNT) ? team->formation : 0u;
    u8    needed[4], pos, i;
    needed[0] = s_form_slots[fn][0]; needed[1] = s_form_slots[fn][1];
    needed[2] = s_form_slots[fn][2]; needed[3] = s_form_slots[fn][3];
    for (i = 0u; i < team->player_count; i++)
        g_players[(u16)(team->player_start + i)].on_field = 0u;
    for (pos = 0u; pos < 4u; pos++) {
        u8 count = 0u;
        while (count < needed[pos]) {
            u16 best = 0xFFFFu; u8 bstr = 0u;
            for (i = 0u; i < team->player_count; i++) {
                Player *pl = &g_players[(u16)(team->player_start + i)];
                if (pl->pos == pos && !pl->on_field && pl->strength > bstr)
                    { bstr = pl->strength; best = (u16)(team->player_start + i); }
            }
            if (best == 0xFFFFu) break;
            g_players[best].on_field = 1u;
            count++;
        }
    }
}

/* Build index lists for field and bench */
static u8 build_lists(u8 team_idx,
                       u16 *field, u8 *nfield,
                       u16 *bench, u8 *nbench) {
    Team *team = &g_teams[team_idx];
    u8 i;
    *nfield = 0u; *nbench = 0u;
    for (i = 0u; i < team->player_count; i++) {
        u16 pi = (u16)(team->player_start + i);
        if (g_players[pi].on_field) field[(*nfield)++] = pi;
        else                         bench[(*nbench)++] = pi;
    }
    return team->player_count;
}

void screen_squad(void) {
    Team  *team     = &g_teams[g_player_team_idx];
    u16    field[16], bench[16];
    u8     nfield, nbench;
    u8     active_panel = 0u;  /* 0=campo, 1=banco */
    u8     sel_field = 0u, sel_bench = 0u;
    u8     redraw    = 1u;
    u8     swap_mode = 0u;     /* 1 = waiting to pick swap target */
    u8     swap_src  = 0u;     /* index in field[] to swap */

    build_lists(g_player_team_idx, field, &nfield, bench, &nbench);

    for (;;) {
        ui_wait_vblank();
        input_update();

        if (input_pressed(BTN_CANCEL) || input_pressed(BTN_START)) return;

        /* C: cycle formation + auto-select */
        if (input_pressed(BTN_ACTION)) {
            team->formation = (u8)((team->formation + 1u) % (u8)FORM_COUNT);
            auto_select(g_player_team_idx);
            build_lists(g_player_team_idx, field, &nfield, bench, &nbench);
            sel_field = 0u; sel_bench = 0u; swap_mode = 0u; redraw = 1u;
            continue;
        }
        /* X: auto-select only */
        if (input_pressed(BUTTON_X)) {
            auto_select(g_player_team_idx);
            build_lists(g_player_team_idx, field, &nfield, bench, &nbench);
            sel_field = 0u; sel_bench = 0u; swap_mode = 0u; redraw = 1u;
            continue;
        }
        /* A: toggle active panel */
        if (input_pressed(BTN_CONFIRM)) {
            if (swap_mode) {
                /* Confirm swap: swap field[swap_src] with bench[sel_bench] */
                if (nbench > 0u) {
                    g_players[field[swap_src]].on_field = 0u;
                    g_players[bench[sel_bench]].on_field = 1u;
                    build_lists(g_player_team_idx, field, &nfield, bench, &nbench);
                    if (sel_field >= nfield && nfield > 0u) sel_field = (u8)(nfield - 1u);
                    if (sel_bench >= nbench && nbench > 0u) sel_bench = (u8)(nbench - 1u);
                    swap_mode = 0u;
                }
            } else if (active_panel == 0u && nfield > 0u) {
                /* Start swap: select from bench */
                swap_mode = 1u;
                swap_src  = sel_field;
                active_panel = 1u;
            } else if (active_panel == 1u) {
                active_panel = 0u;
            }
            redraw = 1u;
            continue;
        }

        if (input_repeat(BTN_UP)) {
            if (active_panel == 0u && sel_field > 0u)        { sel_field--; redraw = 1u; }
            else if (active_panel == 1u && sel_bench > 0u)   { sel_bench--; redraw = 1u; }
        }
        if (input_repeat(BTN_DOWN)) {
            if (active_panel == 0u && sel_field < (u8)(nfield > 0u ? nfield-1u : 0u)) { sel_field++; redraw = 1u; }
            else if (active_panel == 1u && sel_bench < (u8)(nbench > 0u ? nbench-1u : 0u)) { sel_bench++; redraw = 1u; }
        }
        /* LEFT/RIGHT: switch panels */
        if (input_pressed(BUTTON_LEFT) || input_pressed(BUTTON_RIGHT)) {
            active_panel ^= 1u;
            if (swap_mode && active_panel == 0u) swap_mode = 0u;
            redraw = 1u;
        }

        if (!redraw) continue;
        redraw = 0u;

        ui_clear();

        /* Header */
        {
            u8 fn = (team->formation < (u8)FORM_COUNT) ? team->formation : 0u;
            ui_puts(0u, 0u, UI_PAL_NORMAL, team->name);
            ui_printf(22u, 0u, UI_PAL_NORMAL, "Form:%s", s_form_names[fn]);
            ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);
        }

        /* Two-panel display */
        /* Left panel: CAMPO (col 0-19) */
        {
            u8 i;
            u16 hpal = (active_panel == 0u) ? UI_PAL_SELECT : UI_PAL_NORMAL;
            if (swap_mode) hpal = UI_PAL_NORMAL; /* greyed during swap */
            ui_puts(0u, 2u, hpal, "JOGADORES EM CAMPO");
            for (i = 0u; i < nfield && i < 11u; i++) {
                Player *pl  = &g_players[field[i]];
                u16     row = (u16)(3u + i);
                u16     pal = (!swap_mode && active_panel == 0u && i == sel_field)
                              ? UI_PAL_SELECT : UI_PAL_NORMAL;
                if (swap_mode && i == swap_src) pal = UI_PAL_SELECT; /* show who swaps */
                if (pal == UI_PAL_SELECT) ui_fill_row(row, UI_PAL_SELECT);
                ui_printf(0u, row, pal, "%-12s %2s %2u",
                          pl->name, s_pos[pl->pos < 4u ? pl->pos : 0u],
                          (u16)pl->strength);
            }
        }

        /* Divider */
        {
            u8 r;
            for (r = 2u; r < 14u; r++)
                ui_putc(19u, (u16)r, UI_PAL_NORMAL, '|');
        }

        /* Right panel: BANCO (col 20-39) */
        {
            u8 i;
            u16 hpal = (active_panel == 1u) ? UI_PAL_SELECT : UI_PAL_NORMAL;
            ui_puts(20u, 2u, hpal, "JOGADORES NO BANCO");
            for (i = 0u; i < nbench && i < 11u; i++) {
                Player *pl  = &g_players[bench[i]];
                u16     row = (u16)(3u + i);
                u16     pal = (active_panel == 1u && i == sel_bench)
                              ? UI_PAL_SELECT : UI_PAL_NORMAL;
                if (pal == UI_PAL_SELECT) ui_fill_row(row, UI_PAL_SELECT);
                ui_printf(20u, row, pal, "%-12s %2s %2u",
                          pl->name, s_pos[pl->pos < 4u ? pl->pos : 0u],
                          (u16)pl->strength);
            }
        }

        /* Salary summary */
        ui_hline(0u, 15u, UI_COLS, UI_PAL_NORMAL);
        ui_printf(0u, 16u, UI_PAL_NORMAL,
                  "Em campo: %u  Banco: %u  Total: %u",
                  (u16)nfield, (u16)nbench,
                  (u16)(nfield + nbench));

        /* Help */
        ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
        if (swap_mode)
            ui_puts(0u, 27u, UI_PAL_NORMAL, "Escolha suplente a entrar: A=confirmar B=cancelar");
        else
            ui_puts(0u, 27u, UI_PAL_NORMAL, "A:substituir C:form X:auto </>=painel B:sair");
    }
}
