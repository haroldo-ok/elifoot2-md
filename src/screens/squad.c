/*
 * screens/squad.c -- Plantel com formacoes e auto-escalar
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

/* GR, DF, MD, AV slots por formacao */
static const u8 s_form_slots[FORM_COUNT][4] = {
    {1,4,4,2}, {1,4,3,3}, {1,3,5,2}, {1,5,3,2}, {1,4,5,1},
    {1,3,4,3}, {1,5,4,1}, {1,4,2,4}, {1,3,3,4}, {1,5,2,3}
};

static void auto_select(u8 team_idx) {
    Team   *team  = &g_teams[team_idx];
    u8      form  = (team->formation < (u8)FORM_COUNT) ? team->formation : 0u;
    u8      needed[4];
    u8      pos, i;

    needed[0] = s_form_slots[form][0];
    needed[1] = s_form_slots[form][1];
    needed[2] = s_form_slots[form][2];
    needed[3] = s_form_slots[form][3];

    for (i = 0u; i < team->player_count; i++)
        g_players[(u16)(team->player_start + i)].on_field = 0u;

    for (pos = 0u; pos < 4u; pos++) {
        u8 count = 0u;
        while (count < needed[pos]) {
            u16 best_idx = 0xFFFFu;
            u8  best_str = 0u;
            for (i = 0u; i < team->player_count; i++) {
                Player *pl = &g_players[(u16)(team->player_start + i)];
                if (pl->pos == pos && !pl->on_field && pl->strength > best_str) {
                    best_str = pl->strength;
                    best_idx = (u16)(team->player_start + i);
                }
            }
            if (best_idx == 0xFFFFu) break;
            g_players[best_idx].on_field = 1u;
            count++;
        }
    }
}

void screen_squad(void) {
    Team   *team  = &g_teams[g_player_team_idx];
    u8      sel   = 0u;
    u8      total = team->player_count;
    u8      redraw = 1u;
    u8      i;

    for (;;) {
        ui_wait_vblank();
        input_update();

        if (input_pressed(BTN_CANCEL) || input_pressed(BTN_START)) return;

        if (input_repeat(BTN_DOWN) && sel < (u8)(total - 1u)) { sel++; redraw = 1u; }
        if (input_repeat(BTN_UP)   && sel > 0u)               { sel--; redraw = 1u; }

        if (input_pressed(BTN_CONFIRM)) {
            Player *pl = &g_players[(u16)(team->player_start + sel)];
            pl->on_field = pl->on_field ? 0u : 1u;
            redraw = 1u;
        }
        /* C: ciclar formacao e auto-escalar */
        if (input_pressed(BTN_ACTION)) {
            team->formation = (u8)((team->formation + 1u) % (u8)FORM_COUNT);
            auto_select(g_player_team_idx);
            redraw = 1u;
        }
        /* X: auto-escalar sem mudar formacao */
        if (input_pressed(BUTTON_X)) {
            auto_select(g_player_team_idx);
            redraw = 1u;
        }

        if (!redraw) continue;
        redraw = 0u;

        ui_clear();

        {
            u8 fn = (team->formation < (u8)FORM_COUNT) ? team->formation : 0u;
            u8 tit = 0u;
            for (i = 0u; i < total; i++)
                if (g_players[(u16)(team->player_start + i)].on_field) tit++;

            ui_puts(0u, 0u, UI_PAL_NORMAL, team->name);
            ui_printf(22u, 0u, UI_PAL_NORMAL, "Form:%s", s_form_names[fn]);
            ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);
            ui_puts(0u, 2u, UI_PAL_NORMAL, "#  Nome            Pos For Sal      T");
            ui_hline(0u, 3u, UI_COLS, UI_PAL_NORMAL);

            for (i = 0u; i < total && i < 22u; i++) {
                u16     pidx = (u16)(team->player_start + i);
                Player *pl   = &g_players[pidx];
                u16     row  = (u16)(4u + i);
                u16     pal  = (i == sel) ? UI_PAL_SELECT : UI_PAL_NORMAL;
                if (i == sel) ui_fill_row(row, UI_PAL_SELECT);
                ui_printf(0u, row, pal, "%2u %-15s %2s  %2u %7ld %s",
                          (u16)(i + 1u), pl->name,
                          s_pos[pl->pos < 4u ? pl->pos : 0u],
                          (u16)pl->strength, pl->salary,
                          pl->on_field ? "T" : "S");
            }

            ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
            ui_printf(0u, 27u, UI_PAL_NORMAL,
                      "A:tog C:form X:auto  Tit:%u", (u16)tit);
        }
    }
}
