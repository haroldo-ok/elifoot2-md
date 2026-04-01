/*
 * screens/save_load.c -- Guardar e carregar jogo via SRAM
 */

#include <genesis.h>
#include "save_load.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../engine/sram_io.h"
#include "../game/data.h"

void screen_save_load(void) {
    u8  sel    = 0u;
    u8  redraw = 1u;
    u8  mode   = 0u;  /* 0=menu, 1=guardar, 2=carregar */

    sram_init();

    for (;;) {
        ui_wait_vblank();
        input_update();

        if (input_pressed(BTN_CANCEL) || input_pressed(BTN_START)) return;

        if (input_repeat(BTN_DOWN) && sel < 2u) { sel++; redraw = 1u; }
        if (input_repeat(BTN_UP)   && sel > 0u) { sel--; redraw = 1u; }

        if (input_pressed(BTN_CONFIRM)) {
            u8 result;
            ui_clear();
            if (mode == 1u) {
                result = sram_save(sel);
                ui_puts(8u, 13u, UI_PAL_NORMAL,
                        result ? "Jogo guardado!" : "Erro ao guardar.");
            } else if (mode == 2u) {
                if (!sram_slot_valid(sel)) {
                    ui_puts(8u, 13u, UI_PAL_NORMAL, "Slot vazio.");
                } else {
                    result = sram_load(sel);
                    ui_puts(8u, 13u, UI_PAL_NORMAL,
                            result ? "Jogo carregado!" : "Erro ao carregar.");
                }
            } else {
                mode = (sel == 0u) ? 1u : 2u;
                redraw = 1u;
                continue;
            }
            ui_puts(8u, 15u, UI_PAL_NORMAL, "B: continuar");
            { u16 t; for(t=0u;t<180u;t++){ ui_wait_vblank(); input_update(); if(input_pressed(BTN_CANCEL)) break; } }
            return;
        }

        if (!redraw) continue;
        redraw = 0u;

        ui_clear();
        ui_puts(12u, 0u, UI_PAL_NORMAL, "GUARDAR / CARREGAR");
        ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);

        if (mode == 0u) {
            /* Menu principal */
            u16 p0 = (sel == 0u) ? UI_PAL_SELECT : UI_PAL_NORMAL;
            u16 p1 = (sel == 1u) ? UI_PAL_SELECT : UI_PAL_NORMAL;
            if (sel == 0u) ui_fill_row(10u, UI_PAL_SELECT);
            if (sel == 1u) ui_fill_row(13u, UI_PAL_SELECT);
            ui_puts(15u, 10u, p0, "Guardar Jogo");
            ui_puts(15u, 13u, p1, "Carregar Jogo");
        } else {
            /* Escolha de slot */
            u8 s;
            ui_puts(13u, 3u, UI_PAL_NORMAL,
                    mode == 1u ? "Escolha slot para guardar:"
                               : "Escolha slot para carregar:");
            for (s = 0u; s < 3u; s++) {
                u16 row = (u16)(5u + s * 3u);
                u16 pal = (s == sel) ? UI_PAL_SELECT : UI_PAL_NORMAL;
                if (s == sel) ui_fill_row(row, UI_PAL_SELECT);
                if (sram_slot_valid(s)) {
                    ui_printf(10u, row, pal, "Slot %u - Temporada %u",
                              (u16)(s + 1u), (u16)sram_slot_season(s));
                } else {
                    ui_printf(10u, row, pal, "Slot %u - (vazio)", (u16)(s + 1u));
                }
            }
        }

        ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
        ui_puts(0u, 27u, UI_PAL_NORMAL, "A: confirmar  B: voltar");
    }
}
