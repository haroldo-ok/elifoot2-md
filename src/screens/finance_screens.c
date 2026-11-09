/*
 * screens/finance_screens.c -- VENDER JOGADOR, ALTERAR O ORDENADO,
 * ULTIMAS TRANSFERENCIAS REALIZADAS, ULTIMAS RECEITAS, SITUACAO DO ESTADIO
 */

#include <genesis.h>
#include "screens.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"
#include "../game/market.h"
#include "../game/economy.h"

static void msg(u16 y, u8 fg, u8 bg, const char *s) {
    ui_fill(0u, y, UI_COLS, 1u, bg);
    ui_text(1u, y, fg, bg, s);
}

/* ---- VENDER JOGADOR (Ctrl+F1) --------------------------------------- */
void screen_sell(u8 t) {
    u16 p = pick_player(t, "VENDER JOGADOR - Escolha o jogador");
    long price;
    if (p == PLAYER_NONE) return;
    ui_cls(C_BLUE);
    ui_title(0u, "VENDER JOGADOR", C_YELLOW, C_RED, C_WHITE);
    if (!market_can_lose(t, p)) {
        msg(8u, C_WHITE, C_RED, data_gk_count(t) <= 1u && g_players[p].pos == POS_GR
            ? "So tem um guarda-redes." : "So tem 14 jogadores no plantel.");
        ui_help("A: continuar");
        ui_wait_key();
        return;
    }
    ui_textf(1u, 5u, C_WHITE, C_BLUE, "%s  %s  forca %u", g_players[p].name,
             pos_name(g_players[p].pos), (u16)g_players[p].str);
    price = kbd_number(8u, "Preco de venda:", data_price(p), 7u, C_WHITE, C_RED);
    if (price < 0L) return;
    run_auction(p, price, t);
}

/* ---- ALTERAR O ORDENADO (Ctrl+F2) ----------------------------------- */
void screen_alter_salary(u8 t) {
    u16 p = pick_player(t, "ALTERAR O ORDENADO");
    long v;
    u16 demand;
    char buf[40];
    if (p == PLAYER_NONE) return;
    ui_cls(C_BLUE);
    ui_title(0u, "ALTERAR O ORDENADO", C_YELLOW, C_RED, C_WHITE);
    ui_textf(1u, 4u, C_WHITE, C_BLUE, "%s  forca %u  ordenado %u", g_players[p].name,
             (u16)g_players[p].str, (u16)g_players[p].salary);
    v = kbd_number(6u, "Novo ordenado:", (long)g_players[p].salary, 5u, C_WHITE, C_RED);
    if (v < 0L) return;
    demand = market_demand(p);
    if (v >= (long)demand) {
        g_players[p].salary = (u16)v;
        msg(9u, C_WHITE, C_BLUE, "Aceite.");
    } else {
        ui_sprintf(buf, sizeof(buf), "Nem pensar! Exijo um minimo de %u", (u16)demand);
        msg(9u, C_YELLOW, C_RED, buf);
        if (data_squad_count(t) <= MIN_SQUAD) {
            msg(11u, C_WHITE, C_BLUE, "Como so tem 14 jogadores no plantel");
            msg(12u, C_WHITE, C_BLUE, "e obrigado a aceitar.");
            g_players[p].salary = demand;
        } else if (g_players[p].pos == POS_GR && data_gk_count(t) <= 1u) {
            msg(11u, C_WHITE, C_BLUE, "Como so tem um guarda-redes");
            msg(12u, C_WHITE, C_BLUE, "e obrigado a aceitar.");
            g_players[p].salary = demand;
        } else if (ui_ask_yn(11u, C_WHITE, C_BLUE, "Aceita (S/N)?")) {
            g_players[p].salary = demand;
        } else {
            msg(13u, C_YELLOW, C_BLUE, "Entao adeus.");
            market_release(p);
        }
    }
    ui_help("A: continuar");
    ui_wait_key();
}

/* ---- ULTIMAS TRANSFERENCIAS REALIZADAS (Ctrl+F4) -------------------- */
void screen_transfers(void) {
    u8 page = 0u, pages = (u8)((g_transfer_n + 10u) / 11u), redraw = 1u;
    if (!pages) pages = 1u;
    for (;;) {
        if (redraw) {
            u8 i;
            redraw = 0u;
            ui_cls(C_BLUE);
            ui_box(0u, 0u, UI_COLS, 3u, C_WHITE, C_RED);
            ui_text(3u, 1u, C_YELLOW, C_RED, "ULTIMAS TRANSFERENCIAS REALIZADAS");
            ui_text(0u, 3u, C_LGREEN, C_BLUE, "JOGADOR        PS F  NAC  ORD");
            if (!g_transfer_n) ui_text(6u, 10u, C_WHITE, C_BLUE, "Ainda nao houve transferencias.");
            for (i = 0u; i < 11u; i++) {
                u8 k = (u8)(page * 11u + i);
                TransferRec *r;
                Player *pl;
                u16 y = (u16)(4u + i * 2u);
                if (k >= g_transfer_n) break;
                r = &g_transfers[g_transfer_n - 1u - k];
                pl = &g_players[r->player];
                ui_textf(0u, y, C_WHITE, C_BLUE, "%-15s %c %2u %c%c%c %5u", pl->name,
                         pos_letter(pl->pos), (u16)r->str, pl->nac[0], pl->nac[1], pl->nac[2],
                         (u16)r->salary);
                ui_team(2u, (u16)(y + 1u), 15u, r->from);
                ui_text(18u, (u16)(y + 1u), C_WHITE, C_BLUE, ">");
                ui_team(20u, (u16)(y + 1u), 15u, r->to);
            }
            ui_help(pages > 1u ? "</>: pagina   B: voltar" : "B: voltar");
        }
        ui_wait_vblank();
        input_update();
        if (input_pressed(BTN_RIGHT) && page + 1u < pages) { page++; redraw = 1u; }
        if (input_pressed(BTN_LEFT) && page > 0u) { page--; redraw = 1u; }
        if (input_pressed(BTN_CANCEL) || input_pressed(BTN_CONFIRM) || input_pressed(BTN_START)) return;
    }
}

/* ---- ULTIMAS RECEITAS (Ctrl+F5) ------------------------------------- */
void screen_revenues(u8 m) {
    u8 kind = 0u, redraw = 1u;
    for (;;) {
        if (redraw) {
            u8 i, n = g_rev_n[m][kind];
            long total = 0L;
            redraw = 0u;
            ui_cls(C_BLUE);
            ui_title(0u, "ULTIMAS RECEITAS", C_YELLOW, C_RED, C_WHITE);
            ui_text(0u, 3u, C_YELLOW, C_BLUE, kind ? "TACA" : "CAMP");
            ui_text(0u, 4u, C_LGREEN, C_BLUE, "ADVERSARIO  JOGO  BILHET ESPECT RECEITA");
            if (!n) ui_text(8u, 12u, C_WHITE, C_BLUE, "Ainda nao jogou em casa.");
            for (i = 0u; i < n; i++) {
                RevenueRec *r = &g_rev[m][kind][n - 1u - i];
                u16 y = (u16)(5u + i * 2u);
                ui_team(0u, y, 11u, r->opp);
                ui_textf(12u, y, C_WHITE, C_BLUE, "%u-%u", (u16)r->gh, (u16)r->ga);
                ui_textf(17u, y, C_WHITE, C_BLUE, "%6u %6u %7ld", (u16)r->tickets,
                         (u16)r->spectators, r->revenue);
                total += r->revenue;
            }
            if (n) ui_textf(0u, 23u, C_YELLOW, C_BLUE, "TOTAL %33ld", total);
            ui_help("</>: campeonato / taca   B: voltar");
        }
        ui_wait_vblank();
        input_update();
        if (input_pressed(BTN_RIGHT) || input_pressed(BTN_LEFT)) { kind ^= 1u; redraw = 1u; }
        if (input_pressed(BTN_CANCEL) || input_pressed(BTN_CONFIRM) || input_pressed(BTN_START)) return;
    }
}

/* ---- SITUACAO DO ESTADIO (Alt+F2) ------------------------------------ */
void screen_stadium(u8 t) {
    for (;;) {
        Team *tm = &g_teams[t];
        u8 i;
        ui_cls(C_BLUE);
        ui_box(0u, 0u, UI_COLS, 3u, C_WHITE, C_RED);
        ui_text(10u, 1u, C_WHITE, C_RED, "SITUACAO DO ESTADIO");
        ui_box(0u, 4u, UI_COLS, 20u, C_WHITE, C_RED);
        for (i = 0u; i < tm->nstands; i++) {
            u16 len = (u16)(((u16)tm->stand_life[i] * 34u + STAND_LIFE - 1u) / STAND_LIFE), k;
            ui_textf(1u, (u16)(5u + i), C_WHITE, C_RED, "%2u", (u16)(i + 1u));
            for (k = 0u; k < len && k < 34u; k++)
                ui_putc_c((u16)(4u + k), (u16)(5u + i), C_WHITE, C_RED, CH_BLOCK);
        }
        ui_textf(1u, 22u, C_YELLOW, C_RED, "%u lugares   Dinheiro %ld", (u16)data_capacity(t), tm->money);
        if (tm->nstands >= STANDS_MAX || tm->money < STAND_PRICE) {
            msg(25u, C_WHITE, C_RED, tm->nstands >= STANDS_MAX ? "O estadio esta completo."
                                                               : "Nao tem dinheiro para uma bancada.");
            ui_help("B: voltar");
            ui_wait_key();
            return;
        }
        if (!ui_ask_yn(25u, C_WHITE, C_RED, "Construir uma bancada (preco 100000)?"))
            return;
        economy_build_stand(t);
    }
}
