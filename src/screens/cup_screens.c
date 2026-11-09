/*
 * screens/cup_screens.c -- SORTEIO DOS JOGOS DA TACA, EQUIPAS APURADAS,
 * EQUIPAS ELIMINADAS, FINAL DA TACA, VENCEDOR DA TACA
 *
 * Original: as equipas em 4 colunas de 8 nas suas cores; os pares sao
 * sorteados um a um para a caixa de cima (animado).
 */

#include <genesis.h>
#include "screens.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"
#include "../game/season.h"

static const char * const s_phase[5] = {
    "1a ELIMINATORIA", "2a ELIMINATORIA", "QUARTOS DE FINAL", "MEIAS FINAIS", "FINAL"
};

void screen_cup_draw(void) {
    u8 pool[TEAM_COUNT], n = 0u, i, t, fast = 0u;
    u8 shown[TEAM_COUNT];
    if (g_cup_phase >= CUP_PHASES) return;
    /* equipas em prova pela ordem das divisoes */
    for (t = 0u; t < (u8)TEAM_COUNT; t++) if (g_teams[t].cup_alive) { pool[n] = t; shown[n++] = 1u; }
    ui_cls(C_GREEN);
    ui_box(0u, 0u, UI_COLS, 3u, C_BLUE, C_GREEN);
    ui_text(7u, 1u, C_BLUE, C_GREEN, "SORTEIO DOS JOGOS DA TACA");
    for (i = 0u; i < n; i++)
        ui_team((u16)((i / 8u) * 10u), (u16)(3u + (i % 8u)), 9u, pool[i]);
    ui_textw(0u, 11u, UI_COLS, C_YELLOW, C_GREEN, s_phase[g_cup_phase]);
    for (i = 0u; i < g_cup_n; i++) {
        u8 side, k;
        for (side = 0u; side < 2u; side++) {
            u8 tm = side ? g_cup[i].b : g_cup[i].a;
            for (k = 0u; k < n; k++)
                if (pool[k] == tm && shown[k]) {
                    shown[k] = 0u;
                    ui_fill((u16)((k / 8u) * 10u), (u16)(3u + (k % 8u)), 9u, 1u, C_GREEN);
                }
            ui_team((u16)(side ? 20u : 2u), (u16)(12u + i), 16u, tm);
            if (!fast && ui_delay(12u, 1u)) fast = 1u;
        }
    }
    ui_textw(0u, 11u, UI_COLS, C_YELLOW, C_GREEN, "Prima A para continuar");
    ui_wait_key();
}

void screen_cup_after_phase(u8 phase) {
    u8 t, y;
    if (phase >= (u8)(CUP_PHASES - 1u) && g_cup_winner < (u8)TEAM_COUNT) {
        CupTie *c = &g_cup[0];
        ui_cls(C_BLUE);
        ui_title(0u, "FINAL DA TACA", C_YELLOW, C_RED, C_WHITE);
        ui_team(2u, 6u, 15u, c->a);
        ui_textf(18u, 6u, C_WHITE, C_BLUE, "%u-%u", (u16)(c->ga1 + c->ga2), (u16)(c->gb1 + c->gb2));
        ui_team(23u, 6u, 15u, c->b);
        if (c->pa || c->pb)
            ui_textf(12u, 8u, C_WHITE, C_BLUE, "Penaltis %u-%u", (u16)c->pa, (u16)c->pb);
        ui_box(6u, 12u, 28u, 5u, C_WHITE, C_RED);
        ui_text(12u, 13u, C_YELLOW, C_RED, "VENCEDOR DA TACA");
        ui_team(10u, 15u, 20u, g_cup_winner);
        ui_help("A: continuar");
        ui_wait_key();
        return;
    }
    ui_cls(C_BLUE);
    ui_title(0u, "EQUIPAS APURADAS", C_YELLOW, C_GREEN, C_WHITE);
    y = 3u;
    for (t = 0u; t < (u8)TEAM_COUNT && y < 20u; t++)
        if (g_teams[t].cup_alive) {
            u16 x = (u16)((y - 3u) / 8u * 20u), yy = (u16)(3u + (y - 3u) % 8u);
            ui_team(x, yy, 14u, t);
            ui_textf((u16)(x + 15u), yy, C_WHITE, C_BLUE, "%ua", (u16)(g_teams[t].div + 1u));
            y++;
        }
    ui_title(12u, "EQUIPAS ELIMINADAS", C_YELLOW, C_RED, C_WHITE);
    y = 0u;
    for (t = 0u; t < (u8)TEAM_COUNT; t++)
        if (!g_teams[t].cup_alive && g_teams[t].cup_reach == phase) {
            ui_team((u16)((y / 8u) * 20u), (u16)(15u + y % 8u), 14u, t);
            ui_textf((u16)((y / 8u) * 20u + 15u), (u16)(15u + y % 8u), C_WHITE, C_BLUE,
                     "%ua", (u16)(g_teams[t].div + 1u));
            y++;
        }
    ui_help("A: continuar");
    ui_wait_key();
}

void screen_cup_status(void) {
    u8 i;
    ui_cls(C_BLUE);
    ui_title(0u, "TACA", C_YELLOW, C_GREEN, C_WHITE);
    if (g_cup_phase >= CUP_PHASES) {
        ui_text(10u, 8u, C_WHITE, C_BLUE, "VENCEDOR DA TACA");
        ui_team(10u, 10u, 20u, g_cup_winner);
    } else {
        ui_text(0u, 3u, C_YELLOW, C_BLUE, s_phase[g_cup_phase]);
        ui_text(26u, 3u, C_WHITE, C_BLUE, "1a mao");
        for (i = 0u; i < g_cup_n; i++) {
            CupTie *c = &g_cup[i];
            u16 y = (u16)(5u + i);
            ui_team(0u, y, 13u, c->a);
            ui_team(14u, y, 13u, c->b);
            /* resultado da 1a mao se ja jogada nesta fase */
            if (g_slot > 0u && season_slot_kind((u8)(g_slot - 1u)) == SLOT_CUP
                    && CUP_LEG_OF(season_slot_index((u8)(g_slot - 1u))) == 0u)
                ui_textf(28u, y, C_WHITE, C_BLUE, "%u-%u", (u16)c->ga1, (u16)c->gb1);
        }
    }
    ui_help("B: voltar");
    ui_wait_key();
}
