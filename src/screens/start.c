/*
 * screens/start.c -- Titulo, "Quer continuar um jogo?", treinadores,
 * gravar/continuar (original: GRAVAR O JOGO / CONTINUAR UM JOGO /
 * Nome da gravacao / AGUARDE UM MOMENTO).
 */

#include <genesis.h>
#include "screens.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../engine/sram_io.h"
#include "../game/data.h"
#include "../game/coaches.h"

/* Letras 3x5 para o logotipo (1 = bloco). */
static const u8 s_logo_glyph[7][5] = {
    { 7, 4, 6, 4, 7 },   /* E */
    { 4, 4, 4, 4, 7 },   /* L */
    { 7, 2, 2, 2, 7 },   /* I */
    { 7, 4, 6, 4, 4 },   /* F */
    { 7, 5, 5, 5, 7 },   /* O */
    { 7, 2, 2, 2, 2 },   /* T */
    { 2, 2, 2, 2, 2 },   /* | (o "I" de II) */
};
static const u8 s_logo[10] = { 0, 1, 2, 3, 4, 4, 5, 0xFF, 6, 6 };

static void draw_logo(void) {
    u8 i, r, c;
    u16 x = 2u;
    ui_cls(C_RED);
    ui_fill(0u, 3u, UI_COLS, 9u, C_BLUE);
    for (i = 0u; i < 10u; i++) {
        if (s_logo[i] == 0xFFu) { x = (u16)(x + 1u); continue; }
        for (r = 0u; r < 5u; r++)
            for (c = 0u; c < 3u; c++)
                if (s_logo_glyph[s_logo[i]][r] & (4u >> c)) {
                    ui_putc_c((u16)(x + c), (u16)(5u + r), C_WHITE, C_BLUE, CH_BLOCK);
                }
        x = (u16)(x + (s_logo[i] == 6u ? 2u : 4u));
    }
    ui_text(14u, 14u, C_YELLOW, C_RED, "Mega Drive");
    ui_fill(1u, 17u, 38u, 1u, C_BLACK);
}

u8 screen_title(void) {
    draw_logo();
    if (g_autoplay) return START_NEW;
    ui_delay(90u, 1u);
    for (;;) {
        u8 any = 0u, s;
        for (s = 0u; s < SAVE_SLOTS; s++) any |= sram_slot_valid(s);
        if (!any) return START_NEW;
        ui_fill(0u, 25u, UI_COLS, 1u, C_RED);
        if (!ui_ask_yn(25u, C_YELLOW, C_BLUE, "Quer continuar um jogo?")) return START_NEW;
        if (screen_load()) return START_LOAD;
        draw_logo();
    }
}

static u8 pick_slot(const char *title, u8 for_save) {
    u8 sel = 0u, redraw = 1u, s;
    for (;;) {
        if (redraw) {
            redraw = 0u;
            ui_cls(C_BLUE);
            ui_title(0u, title, C_YELLOW, C_GREEN, C_WHITE);
            for (s = 0u; s < SAVE_SLOTS; s++) {
                char name[SAVE_NAME_LEN];
                u8 season;
                u16 y = (u16)(6u + s * 3u);
                u8 fg = (s == sel) ? C_BLACK : C_WHITE, bg = (s == sel) ? C_CYAN : C_RED;
                ui_fill(4u, y, 32u, 1u, bg);
                if (sram_slot_info(s, name, &season))
                    ui_textf(5u, y, fg, bg, "%u  %-13s  Epoca %u", (u16)(s + 1u), name, (u16)season);
                else
                    ui_textf(5u, y, fg, bg, "%u  (vazio)", (u16)(s + 1u));
            }
            ui_help(for_save ? "A: gravar aqui  B: voltar" : "A: continuar este  B: voltar");
        }
        ui_wait_vblank();
        input_update();
        if (input_repeat(BTN_DOWN) && sel + 1u < SAVE_SLOTS) { sel++; redraw = 1u; }
        if (input_repeat(BTN_UP) && sel > 0u) { sel--; redraw = 1u; }
        if (input_pressed(BTN_CANCEL)) return 0xFFu;
        if (input_pressed(BTN_CONFIRM)) {
            if (for_save || sram_slot_valid(sel)) return sel;
        }
    }
}

u8 screen_load(void) {
    u8 s = pick_slot("CONTINUAR UM JOGO", 0u);
    if (s == 0xFFu) return 0u;
    ui_cls(C_BLUE);
    ui_text(10u, 13u, C_WHITE, C_BLUE, "AGUARDE UM MOMENTO");
    ui_wait_vblank();
    if (sram_load(s)) return 1u;
    ui_cls(C_BLUE);
    ui_text(8u, 12u, C_YELLOW, C_RED, "ERRO A LER O FICHEIRO");
    ui_wait_key();
    return 0u;
}

void screen_save(void) {
    char name[SAVE_NAME_LEN];
    u8 s = pick_slot("GRAVAR O JOGO", 1u);
    if (s == 0xFFu) return;
    data_strcpy(name, g_mgr[0].name, SAVE_NAME_LEN);
    if (!kbd_text("Nome da gravacao:", name, (u8)(SAVE_NAME_LEN - 1u))) return;
    ui_cls(C_BLUE);
    ui_text(10u, 13u, C_WHITE, C_BLUE, "AGUARDE UM MOMENTO");
    ui_wait_vblank();
    sram_save(s, name);
    ui_text(14u, 15u, C_YELLOW, C_BLUE, "Gravado.");
    ui_delay(60u, 1u);
}

/* ---- Treinadores no inicio (NOME / EQUIPA) ------------------------- */
void screen_managers_setup(void) {
    u8 redraw = 1u;
    if (g_autoplay) { managers_add("Teste"); return; }
    for (;;) {
        if (redraw) {
            u8 m;
            redraw = 0u;
            ui_cls(C_BLACK);
            ui_box(4u, 1u, 32u, 3u, C_RED, C_BLACK);
            ui_text(6u, 2u, C_GREEN, C_BLACK, "NOME");
            ui_text(20u, 2u, C_GREEN, C_BLACK, "EQUIPA");
            ui_box(4u, 5u, 32u, (u16)(2u + (g_mgr_count ? g_mgr_count : 1u)), C_RED, C_BLACK);
            for (m = 0u; m < g_mgr_count; m++) {
                ui_text(6u, (u16)(6u + m), C_LRED, C_BLACK, g_mgr[m].name);
                ui_team(20u, (u16)(6u + m), 14u, g_mgr[m].team);
            }
            if (g_mgr_count < MAX_MANAGERS)
                ui_text(6u, 14u, C_WHITE, C_BLACK, "A: novo treinador");
            if (g_mgr_count)
                ui_text(6u, 16u, C_WHITE, C_BLACK, "START: comecar o jogo");
            ui_text(6u, 18u, C_DGRAY, C_BLACK, "Cada treinador recebe uma");
            ui_text(6u, 19u, C_DGRAY, C_BLACK, "equipa da 4a divisao.");
            ui_help("A: novo  B: retira o ultimo  START: jogar");
        }
        ui_wait_vblank();
        input_update();
        if (input_pressed(BTN_CONFIRM) && g_mgr_count < MAX_MANAGERS) {
            char name[MGR_NAME_LEN];
            name[0] = '\0';
            if (kbd_text("NOME DO TREINADOR", name, (u8)(MGR_NAME_LEN - 1u)))
                managers_add(name);
            redraw = 1u;
        }
        if (input_pressed(BTN_CANCEL) && g_mgr_count) {
            managers_remove((u8)(g_mgr_count - 1u));
            redraw = 1u;
        }
        if (input_pressed(BTN_START) && g_mgr_count) return;
    }
}
