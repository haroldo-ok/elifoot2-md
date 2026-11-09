/*
 * main.c -- Elifoot II para Mega Drive
 *
 * Ciclo de uma jornada (como no original):
 *   ecra principal de cada treinador humano (escolhe a tactica)
 *   -> leiloes -> jogo ao vivo (intervalo com substituicoes)
 *   -> classificacao / taca -> chicotadas -> pedidos de aumento.
 * Epoca = 24 jornadas (14 do campeonato + 10 maos da taca).
 */

#include <genesis.h>
#include "engine/ui.h"
#include "engine/input.h"
#include "engine/rng.h"
#include "engine/sram_io.h"
#include "game/data.h"
#include "game/season.h"
#include "game/coaches.h"
#include "game/history.h"
#include "game/tactics.h"
#include "screens/screens.h"

static void new_game(void) {
    data_init();
    history_reset();
    screen_managers_setup();
    season_start();
    screen_cup_draw();
}

/* Joga a jornada g_slot. Devolve 0 se um treinador pediu para sair. */
static u8 play_slot(void) {
    u8 m, kind = season_slot_kind(g_slot), idx = season_slot_index(g_slot);
    for (m = 0u; m < g_mgr_count; m++)
        if (screen_main(m) == MAIN_QUIT) return 0u;
    /* titulares garantidos para equipas humanas */
    for (m = 0u; m < g_mgr_count; m++) {
        u16 st[16];
        u8 t = g_mgr[m].team;
        if (tactics_starters(t, st) < 11u) tactics_autoselect(t, g_teams[t].formation);
    }
    screen_auctions();
    season_setup_games();
    screen_live_round();
    season_apply_results();
    if (kind == SLOT_LEAGUE) {
        screen_standings_all();
        coaches_ai_sackings();
        screen_sackings();
    } else if (CUP_LEG_OF(idx) == 1u) {
        screen_cup_after_phase(CUP_PHASE_OF(idx));
        if (g_cup_phase < CUP_PHASES) screen_cup_draw();
    }
    season_end_slot();
    screen_salary_requests();
    return 1u;
}

int main(bool hardReset) {
    (void)hardReset;
    JOY_init();
    input_init();
    rng_init(GET_VCOUNTER);
    data_init();
    ui_init();
    sram_init();

    for (;;) {
        u8 st = screen_title();
        rng_init((u16)vtimer);
        if (st == START_LOAD) {
            /* jogo carregado */
        } else {
            new_game();
        }
        for (;;) {
            if (g_slot >= SEASON_SLOTS) {
                screen_season_end();
                screen_cup_draw();
                continue;
            }
            if (!play_slot()) break;
        }
    }
    return 0;
}
