/*
 * screens/main_screen.c -- Ecra principal de cada treinador humano
 *
 * Original (80 col.): plantel a esquerda; clube/lugar/lugares/socios;
 * painel de ajuda rotativo (TACTICAS, CAMPEONATO, FINANCAS, DIVERSOS);
 * proximo adversario nas cores dele; Dinheiro/Ordenados/Preco dos bilhetes.
 * Aqui em 40x28. As teclas F do DOS viram o menu de START.
 */

#include <genesis.h>
#include "screens.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../game/data.h"
#include "../game/season.h"
#include "../game/tactics.h"

#define SQUAD_TOP   3u
#define SQUAD_ROWS 19u

static const char * const s_help[4][2] = {
    { "TACTICAS   A: escolher F1..F10",     "3-4-3 4-3-3 4-4-2 4-5-1 5-2-3 ..." },
    { "CAMPEONATO START: Calendario,",      "Resultados, Palmares, Marcadores..." },
    { "FINANCAS   START: Vender, Ordenados,","Premios, Transferencias, Receitas" },
    { "DIVERSOS   START: Gravar, Estadio,",  "C: Seleccionar melhores, Treinadores" },
};

static void draw_next(u8 t) {
    u8 home, slot, opp = season_next_match(t, &home, &slot);
    char lbl[40];
    ui_fill(0u, 22u, 20u, 3u, C_BLACK);
    if (opp == TEAM_NONE) {
        ui_text(0u, 22u, C_WHITE, C_BLACK, "Fim da epoca");
        return;
    }
    {
        u8 fg = ui_team_fg(opp), bg = ui_team_bg(opp);
        ui_fill(0u, 22u, 20u, 3u, bg);
        ui_textw(0u, 22u, 15u, fg, bg, g_teams[opp].name);
        ui_text(15u, 22u, fg, bg, home ? "CASA" : "FORA");
        if (g_teams[opp].div != g_teams[t].div)
            ui_textf(0u, 23u, fg, bg, "%ua DIV %uo lugar", (u16)(g_teams[opp].div + 1u),
                     (u16)data_team_place(opp));
        else
            ui_textf(0u, 23u, fg, bg, "%uo lugar %u pontos", (u16)data_team_place(opp),
                     (u16)TEAM_PTS(&g_teams[opp]));
        if (season_slot_kind(slot) == SLOT_LEAGUE)
            ui_textf(0u, 24u, fg, bg, "%ua JORNADA", (u16)(season_slot_index(slot) + 1u));
        else {
            u8 idx = season_slot_index(slot);
            cup_leg_label(lbl, CUP_PHASE_OF(idx), CUP_LEG_OF(idx));
            ui_textw(0u, 24u, 20u, fg, bg, lbl);
        }
    }
}

static void draw_main(u8 m, u8 top, u8 help) {
    u8 t = g_mgr[m].team;
    Team *tm = &g_teams[t];
    u8 fg = ui_team_fg(t), bg = ui_team_bg(t);
    u16 sq[MAX_SQUAD + 8];
    u8 n = data_squad(t, sq, (u8)(MAX_SQUAD + 8u)), i;

    ui_cls(C_BLACK);
    ui_fill(0u, 0u, UI_COLS, 3u, bg);
    ui_textw(0u, 0u, 16u, fg, bg, tm->name);
    ui_textw(17u, 0u, 13u, fg, bg, g_mgr[m].name);
    ui_textf(32u, 0u, fg, bg, "Epoca%2u", (u16)g_season);
    ui_textf(0u, 1u, fg, bg, "%uo lugar  %u pontos   %ua DIVISAO",
             (u16)data_team_place(t), (u16)TEAM_PTS(tm), (u16)(tm->div + 1u));
    ui_textf(0u, 2u, fg, bg, "%u lugares   %u socios",
             (u16)data_capacity(t), (u16)tm->socios);

    ui_fill(0u, SQUAD_TOP, UI_COLS, SQUAD_ROWS, bg);
    for (i = 0u; i < SQUAD_ROWS && (u8)(top + i) < n; i++) {
        u8 k = (u8)(top + i);
        u8 show = (k == 0u) || g_players[sq[k]].pos != g_players[sq[k - 1u]].pos;
        draw_player_row(0u, (u16)(SQUAD_TOP + i), sq[k], fg, bg, show);
    }
    if (n > SQUAD_ROWS) ui_textf(34u, SQUAD_TOP, fg, bg, "%u/%u", (u16)(top + 1u), (u16)n);

    draw_next(t);
    ui_fill(20u, 22u, 20u, 3u, bg);
    ui_textf(20u, 22u, fg, bg, "Dinheiro: %9ld", tm->money);
    ui_textf(20u, 23u, fg, bg, "Ordenados:%9ld", data_total_salaries(t));
    ui_textf(20u, 24u, fg, bg, "Bilhetes: %9u", (u16)tm->ticket);

    ui_fill(0u, 25u, UI_COLS, 2u, (u8)(help == 0u ? C_GREEN : help == 1u ? C_BLUE : help == 2u ? C_CYAN : C_MAGENTA));
    ui_text(0u, 25u, C_WHITE, (u8)(help == 0u ? C_GREEN : help == 1u ? C_BLUE : help == 2u ? C_CYAN : C_MAGENTA), s_help[help][0]);
    ui_text(0u, 26u, C_YELLOW, (u8)(help == 0u ? C_GREEN : help == 1u ? C_BLUE : help == 2u ? C_CYAN : C_MAGENTA), s_help[help][1]);
    ui_help("A:Jogar(tactica) START:Menu C:Melhores");
}

/* ---- Menu de comandos (teclas F do original) ----------------------- */
#define CMD_COUNT 20u
static const char * const s_cmd[CMD_COUNT] = {
    /* CAMPEONATO */
    "Calendario", "Resultados", "Palmares", "Marcadores", "Ult. vencedores",
    "Classificacao", "Prox. jornadas", "Titulos equipas", "Titulos treinad.", "Taca",
    /* FINANCAS */
    "Vender", "Ordenados", "Premios", "Transferencias", "Receitas",
    /* DIVERSOS */
    "Gravar", "Estadio", "Selecc. melhores", "Treinadores", "Sair"
};

static u8 cmd_menu(u8 *sel) {
    u8 redraw = 1u, i;
    for (;;) {
        if (redraw) {
            redraw = 0u;
            ui_cls(C_BLUE);
            ui_title(0u, "MENU", C_YELLOW, C_RED, C_WHITE);
            ui_box(0u, 3u, 20u, 22u, C_WHITE, C_BLUE);
            ui_text(2u, 3u, C_YELLOW, C_BLUE, " CAMPEONATO ");
            ui_box(20u, 3u, 20u, 9u, C_WHITE, C_CYAN);
            ui_text(22u, 3u, C_LRED, C_CYAN, " FINANCAS ");
            ui_box(20u, 13u, 20u, 9u, C_WHITE, C_MAGENTA);
            ui_text(22u, 13u, C_WHITE, C_MAGENTA, " DIVERSOS ");
            for (i = 0u; i < CMD_COUNT; i++) {
                u16 x, y; u8 bg;
                if (i < 10u)      { x = 2u;  y = (u16)(5u + i * 2u); bg = C_BLUE; }
                else if (i < 15u) { x = 22u; y = (u16)(5u + (i - 10u)); bg = C_CYAN; }
                else              { x = 22u; y = (u16)(15u + (i - 15u)); bg = C_MAGENTA; }
                if (i == *sel) ui_textw(x, y, 16u, C_BLACK, C_LGRAY, s_cmd[i]);
                else ui_textw(x, y, 16u, bg == C_CYAN ? C_LRED : C_WHITE, bg, s_cmd[i]);
            }
            ui_help("Setas: escolher  A: abrir  B: voltar");
        }
        ui_wait_vblank();
        input_update();
        if (input_repeat(BTN_DOWN) && *sel + 1u < CMD_COUNT) { (*sel)++; redraw = 1u; }
        if (input_repeat(BTN_UP) && *sel > 0u) { (*sel)--; redraw = 1u; }
        if (input_pressed(BTN_RIGHT) && *sel < 10u) { *sel = (u8)(10u + (*sel / 2u)); redraw = 1u; }
        if (input_pressed(BTN_LEFT) && *sel >= 10u) { *sel = (u8)(((*sel - 10u) % 5u) * 2u); redraw = 1u; }
        if (input_pressed(BTN_CANCEL)) return 0xFFu;
        if (input_pressed(BTN_CONFIRM)) return *sel;
    }
}

/* Devolve 1 se pediu para sair. */
static u8 run_cmd(u8 m, u8 c) {
    u8 t = g_mgr[m].team;
    u8 home, slot, opp = season_next_match(t, &home, &slot);
    switch (c) {
        case 0:  screen_calendar(t); break;
        case 1:  screen_results_grid(g_teams[t].div); break;
        case 2:  screen_palmares(t); if (opp != TEAM_NONE) screen_palmares(opp); break;
        case 3:  screen_scorers(); break;
        case 4:  screen_last_winners(); break;
        case 5:  screen_standings_div(g_teams[t].div); break;
        case 6:  screen_next_rounds(t); break;
        case 7:  screen_team_titles(); break;
        case 8:  screen_coach_titles(); break;
        case 9:  screen_cup_status(); break;
        case 10: screen_sell(t); break;
        case 11: screen_alter_salary(t); break;
        case 12: screen_prizes_table(); break;
        case 13: screen_transfers(); break;
        case 14: screen_revenues(m); break;
        case 15: screen_save(); break;
        case 16: screen_stadium(t); break;
        case 17: tactics_autoselect(t, g_teams[t].formation); break;
        case 18: screen_managers(); break;
        case 19:
            ui_cls(C_BLUE);
            if (ui_ask_yn(12u, C_YELLOW, C_RED, "SAIR PARA O MENU INICIAL ? (S/N)")) return 1u;
            break;
        default: break;
    }
    return 0u;
}

u8 screen_main(u8 m) {
    u8 top = 0u, help = 0u, redraw = 1u, sel = 0u;
    u16 timer = 0u;
    g_cur_mgr = m;
    if (g_autoplay) {
        tactics_autoselect(g_mgr[m].team, g_teams[g_mgr[m].team].formation);
        draw_main(m, 0u, 0u);
        ui_wait_vblank();
        return MAIN_PLAY;
    }
    for (;;) {
        u8 t, n;
        if (m >= g_mgr_count) return MAIN_PLAY;   /* treinador anulado */
        t = g_mgr[m].team;
        n = data_squad_count(t);
        if (redraw) { redraw = 0u; draw_main(m, top, help); }
        ui_wait_vblank();
        input_update();
        if (++timer >= 240u) { timer = 0u; help = (u8)((help + 1u) & 3u); redraw = 1u; }
        if (input_repeat(BTN_DOWN) && top + SQUAD_ROWS < n) { top++; redraw = 1u; }
        if (input_repeat(BTN_UP) && top > 0u) { top--; redraw = 1u; }
        if (input_pressed(BTN_ACTION)) { tactics_autoselect(t, g_teams[t].formation); redraw = 1u; }
        if (input_pressed(BTN_CONFIRM)) {
            if (screen_tactic(t)) return MAIN_PLAY;
            redraw = 1u;
        }
        if (input_pressed(BTN_START)) {
            u8 c = cmd_menu(&sel);
            if (c != 0xFFu && run_cmd(m, c)) return MAIN_QUIT;
            redraw = 1u;
        }
    }
}
