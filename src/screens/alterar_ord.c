/*
 * screens/alterar_ord.c -- Alterar Ordenado (gestor propoe novo salario)
 *
 * Fiel ao original:
 *   Titulo: ALTERAR O ORDENADO
 *   "Escolha o jogador" -> seleccao
 *   "Novo ordenado: " -> input numerico
 *   "Nem pensar! Exijo um minimo de [X]" -> abaixo do minimo
 *   Restricoes: 14 jogadores / 1 GR -> aceitacao forcada
 *   "Aceita (S/N)?" -> confirmacao
 *   "Entao adeus." -> recusado, jogador sai
 */

#include <genesis.h>
#include "alterar_ord.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"
#include "../game/transfer.h"
#include "../engine/rng.h"

static const char s_pos_ao[4][3] = { "GR", "DF", "MD", "AV" };

/* Simple numeric input: returns entered value, or 0xFFFFFFFF if cancelled */
static long read_number(u16 x, u16 y, u16 pal) {
    char buf[9];
    u8   len = 0u;
    buf[0] = '\0';

    for (;;) {
        u16 i;
        ui_wait_vblank();
        input_update();

        /* Show current input */
        for (i = 0u; i < 8u; i++)
            ui_putc((u16)(x + i), y, pal, (i < len) ? buf[i] : '_');

        if (input_pressed(BTN_CANCEL)) return -1L;
        if (input_pressed(BTN_CONFIRM) || input_pressed(BTN_START)) {
            if (len == 0u) return -1L;
            /* Parse as long */
            {
                long val = 0L;
                u8 j;
                for (j = 0u; j < len; j++)
                    val = val * 10L + (long)(buf[j] - '0');
                return val;
            }
        }

        /* Digit input via D-pad: UP increases digit, DOWN decreases */
        if (input_pressed(BTN_ACTION)) { /* C = backspace */
            if (len > 0u) { len--; buf[len] = '\0'; }
        }
        /* Number entry: use shoulder buttons or joypad combos */
        /* For simplicity: UP/DOWN adjust last digit, LEFT/RIGHT move */
        if (input_pressed(BUTTON_X) && len < 8u) { buf[len++] = '0'; buf[len] = '\0'; }
        if (input_pressed(BUTTON_Y) && len < 8u) { buf[len++] = '1'; buf[len] = '\0'; }
        if (input_pressed(BUTTON_Z) && len < 8u) { buf[len++] = '2'; buf[len] = '\0'; }
        /* Basic: A=+1000 to value to make it usable */
        if (input_repeat(BTN_UP)   && len < 8u) { buf[len++] = '5'; buf[len] = '\0'; }
        if (input_repeat(BTN_DOWN) && len > 0u) { len--; buf[len] = '\0'; }
    }
}

void screen_alterar_ord(void) {
    Team *team  = &g_teams[g_player_team_idx];
    u8    sel   = 0u;
    u8    total = team->player_count;
    u8    redraw = 1u;

    /* Phase 0: select player, Phase 1: enter salary */
    for (;;) {
        /* === PLAYER SELECTION === */
        redraw = 1u;
        for (;;) {
            ui_wait_vblank();
            input_update();

            if (input_pressed(BTN_CANCEL) || input_pressed(BTN_START)) return;
            if (input_repeat(BTN_DOWN) && sel < (u8)(total-1u)) { sel++; redraw = 1u; }
            if (input_repeat(BTN_UP)   && sel > 0u)              { sel--; redraw = 1u; }

            if (input_pressed(BTN_CONFIRM)) break; /* confirm player */

            if (!redraw) continue;
            redraw = 0u;

            ui_clear();
            ui_puts(10u, 0u, UI_PAL_NORMAL, "ALTERAR O ORDENADO");
            ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);
            ui_puts(0u, 2u, UI_PAL_NORMAL, "Escolha o jogador:");
            ui_puts(0u, 3u, UI_PAL_NORMAL, "#  Nome            Pos Sal actual");
            ui_hline(0u, 4u, UI_COLS, UI_PAL_NORMAL);

            {
                u8 i;
                for (i = 0u; i < total && i < 20u; i++) {
                    u16 pidx = (u16)(team->player_start + i);
                    Player *pl = &g_players[pidx];
                    u16 row = (u16)(5u + i);
                    u16 pal = (i == sel) ? UI_PAL_SELECT : UI_PAL_NORMAL;
                    if (i == sel) ui_fill_row(row, UI_PAL_SELECT);
                    ui_printf(0u, row, pal, "%2u %-15s %2s  %ld",
                              (u16)(i+1u), pl->name,
                              s_pos_ao[pl->pos < 4u ? pl->pos : 0u],
                              pl->salary);
                }
            }
            ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
            ui_puts(0u, 27u, UI_PAL_NORMAL, "A: confirmar  B: cancelar");
        }

        /* === SALARY NEGOTIATION === */
        {
            u16    pidx   = (u16)(team->player_start + sel);
            Player *pl    = &g_players[pidx];
            long   min_sal = pl->salary / 2L + (long)(pl->strength) * 30L;
            u8     total_gr = 0u, i;
            u8     forced;
            long   new_sal;

            for (i = 0u; i < total; i++)
                if (g_players[(u16)(team->player_start + i)].pos == 0u) total_gr++;

            forced = ((pl->pos == 0u && total_gr <= 1u) || total <= 14u);

            ui_clear();
            ui_puts(10u, 0u, UI_PAL_NORMAL, "ALTERAR O ORDENADO");
            ui_hline(0u, 1u, UI_COLS, UI_PAL_NORMAL);
            ui_printf(2u, 3u, UI_PAL_NORMAL, "Jogador: %s", pl->name);
            ui_printf(2u, 4u, UI_PAL_NORMAL, "Ordenado actual: %ld Esc", pl->salary);
            ui_printf(2u, 5u, UI_PAL_NORMAL, "Minimo aceitavel: %ld Esc", min_sal);
            ui_hline(0u, 7u, UI_COLS, UI_PAL_NORMAL);
            ui_puts(2u, 9u, UI_PAL_NORMAL, "Novo ordenado:");
            ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
            ui_puts(0u, 27u, UI_PAL_NORMAL,
                    "Digitos:UP/DOWN  A:confirmar  C:apagar  B:cancelar");

            new_sal = read_number(17u, 9u, UI_PAL_NORMAL);

            if (new_sal < 0L) { redraw = 1u; continue; } /* cancelled */

            if (new_sal < min_sal && !forced) {
                /* Nem pensar! */
                ui_clear();
                ui_puts(4u, 10u, UI_PAL_NORMAL, "Nem pensar!");
                ui_printf(4u, 12u, UI_PAL_NORMAL, "Exijo um minimo de %ld Esc.", min_sal);
                ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
                ui_puts(0u, 27u, UI_PAL_NORMAL, "A/B: continuar");
                for(;;){ ui_wait_vblank(); input_update();
                    if(input_pressed(BTN_CONFIRM)||input_pressed(BTN_CANCEL)) break; }
                redraw = 1u; continue;
            }

            if (forced) {
                pl->salary = (new_sal >= min_sal) ? new_sal : min_sal;
                ui_clear();
                if (pl->pos == 0u && total_gr <= 1u)
                    ui_puts(2u, 10u, UI_PAL_NORMAL, "Como so tem um guarda-redes");
                else
                    ui_puts(2u, 10u, UI_PAL_NORMAL, "Como so tem 14 jogadores");
                ui_puts(2u, 11u, UI_PAL_NORMAL, "no plantel e obrigado a aceitar.");
                ui_printf(2u, 13u, UI_PAL_NORMAL, "Novo ordenado: %ld Esc.", pl->salary);
            } else {
                /* Aceita (S/N)? */
                ui_clear();
                ui_printf(4u, 10u, UI_PAL_NORMAL, "%s, aceita o aumento?", pl->name);
                ui_printf(4u, 11u, UI_PAL_NORMAL, "Novo ordenado: %ld Esc.", new_sal);
                ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
                ui_puts(0u, 27u, UI_PAL_NORMAL, "A: Sim   B: Nao");
                for (;;) {
                    ui_wait_vblank(); input_update();
                    if (input_pressed(BTN_CONFIRM)) { pl->salary = new_sal; break; }
                    if (input_pressed(BTN_CANCEL)) {
                        /* Entao adeus */
                        ui_clear();
                        ui_puts(4u, 12u, UI_PAL_NORMAL, "Entao adeus.");
                        pl->on_field = 0u;
                        pl->salary   = 0L;
                        break;
                    }
                }
            }

            ui_hline(0u, 26u, UI_COLS, UI_PAL_NORMAL);
            ui_puts(0u, 27u, UI_PAL_NORMAL, "A/B: continuar");
            for(;;){ ui_wait_vblank(); input_update();
                if(input_pressed(BTN_CONFIRM)||input_pressed(BTN_CANCEL)) return; }
        }
    }
}
