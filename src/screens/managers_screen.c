/*
 * screens/managers_screen.c -- TREINADORES EM JOGO (Alt+F4)
 *
 * Original: lista dos treinadores humanos; F1 Anular treinador,
 * F2 Novo treinador (pede Nome; recebe equipa da 4a divisao), Esc Fim.
 */

#include <genesis.h>
#include "screens.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"
#include "../game/coaches.h"

void screen_managers(void) {
    u8 sel = 0u, redraw = 1u;
    for (;;) {
        if (redraw) {
            u8 m;
            redraw = 0u;
            ui_cls(C_BLACK);
            ui_box(5u, 2u, 30u, 10u, C_WHITE, C_MAGENTA);
            ui_text(10u, 3u, C_WHITE, C_MAGENTA, "TREINADORES EM JOGO");
            for (m = 0u; m < g_mgr_count; m++) {
                u8 s = (m == sel);
                ui_textf(7u, (u16)(5u + m), s ? C_BLACK : C_WHITE, s ? C_LGRAY : C_MAGENTA,
                         "%u %-12s", (u16)(m + 1u), g_mgr[m].name);
                ui_team(21u, (u16)(5u + m), 13u, g_mgr[m].team);
            }
            ui_box(5u, 13u, 30u, 6u, C_WHITE, C_MAGENTA);
            ui_text(7u, 14u, C_WHITE, C_MAGENTA, "C  Anular treinador");
            ui_text(7u, 15u, C_WHITE, C_MAGENTA, "A  Novo treinador");
            ui_text(7u, 16u, C_WHITE, C_MAGENTA, "B  Fim");
            ui_help("CIMA/BAIXO  A:novo  C:anular  B:fim");
        }
        ui_wait_vblank();
        input_update();
        if (input_repeat(BTN_DOWN) && sel + 1u < g_mgr_count) { sel++; redraw = 1u; }
        if (input_repeat(BTN_UP) && sel > 0u) { sel--; redraw = 1u; }
        if (input_pressed(BTN_CANCEL)) return;
        if (input_pressed(BTN_CONFIRM) && g_mgr_count < MAX_MANAGERS) {
            char name[MGR_NAME_LEN];
            name[0] = '\0';
            if (kbd_text("Nome:", name, (u8)(MGR_NAME_LEN - 1u))) managers_add(name);
            redraw = 1u;
        }
        if (input_pressed(BTN_ACTION) && g_mgr_count > 1u) {
            char q[40];
            ui_sprintf(q, sizeof(q), "Anular %s ? (S/N)", g_mgr[sel].name);
            if (ui_ask_yn(22u, C_YELLOW, C_BLACK, q)) {
                managers_remove(sel);
                if (sel >= g_mgr_count) sel = (u8)(g_mgr_count - 1u);
            }
            redraw = 1u;
        }
    }
}
