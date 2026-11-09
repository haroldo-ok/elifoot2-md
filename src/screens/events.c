/*
 * screens/events.c -- Leiloes ("VENDA PELA MELHOR OFERTA DE ORDENADO"),
 * pedidos de aumento, CHICOTADAS PSICOLOGICAS, fim de epoca, propostas
 * ("X: quer ir treinar o Y? (s/n)").
 */

#include <genesis.h>
#include "screens.h"
#include "../engine/ui.h"
#include "../engine/input.h"
#include "../engine/rng.h"
#include "../game/data.h"
#include "../game/market.h"
#include "../game/coaches.h"
#include "../game/season.h"
#include "../game/history.h"

static void auction_card(u16 p, long price, u16 minsal) {
    Player *pl = &g_players[p];
    u8 t = pl->team, fg = ui_team_fg(t), bg = ui_team_bg(t);
    ui_cls(C_BLUE);
    ui_box(0u, 0u, UI_COLS, 3u, C_YELLOW, C_RED);
    ui_text(2u, 1u, C_YELLOW, C_RED, "VENDA PELA MELHOR OFERTA DE ORDENADO");
    ui_box(0u, 4u, UI_COLS, 9u, C_WHITE, bg);
    ui_textf(1u, 5u, fg, bg, "JOGADOR  %-16s %c%c%c", pl->name, pl->nac[0], pl->nac[1], pl->nac[2]);
    ui_textf(1u, 6u, fg, bg, "POSICAO  %s", pos_name(pl->pos));
    ui_textf(1u, 7u, fg, bg, "FORCA    %u", (u16)pl->str);
    ui_textf(1u, 8u, fg, bg, "EQUIPA   %s", g_teams[t].name);
    ui_textf(1u, 9u, fg, bg, "PRECO    %ld", price);
    ui_textf(1u, 11u, fg, bg, "ORDENADO MINIMO: %u", (u16)minsal);
}

u8 run_auction(u16 p, long price, u8 seller) {
    u16 minsal = data_min_salary(p), best_sal = 0u, ai_sal;
    u8 best = TEAM_NONE, m, ai;
    auction_card(p, price, minsal);
    for (m = 0u; m < g_mgr_count; m++) {
        u8 t = g_mgr[m].team;
        char pr[24];
        long bid;
        if (t == seller || t == g_players[p].team) continue;
        ui_box(0u, 20u, UI_COLS, 3u, C_WHITE, C_RED);
        if (g_teams[t].money < price || data_squad_count(t) >= MAX_SQUAD) {
            ui_textf(1u, 21u, C_WHITE, C_RED, "%s: %s", g_mgr[m].name,
                     data_squad_count(t) >= MAX_SQUAD ? "plantel cheio" : "sem dinheiro");
            ui_delay(50u, 1u);
            continue;
        }
        ui_sprintf(pr, sizeof(pr), "%s:", g_mgr[m].name);
        bid = kbd_number(21u, pr, 0L, 5u, C_WHITE, C_RED);
        if (bid >= (long)minsal && bid > (long)best_sal) { best_sal = (u16)bid; best = t; }
    }
    ai = market_ai_bid(p, price, seller, &ai_sal);
    if (ai != TEAM_NONE && ai_sal > best_sal) { best = ai; best_sal = ai_sal; }

    ui_box(0u, 16u, UI_COLS, 4u, C_WHITE, C_LGRAY);
    if (best == TEAM_NONE) {
        ui_text(2u, 17u, C_BLACK, C_LGRAY, "NAO HOUVE OFERTAS");
        ui_help("A: continuar");
        ui_delay(120u, 1u);
        return 0u;
    }
    ui_text(2u, 17u, C_BLACK, C_LGRAY, "TRANSFERIDO PARA O");
    ui_team(21u, 17u, 16u, best);
    ui_textf(2u, 18u, C_BLACK, C_LGRAY, "NOVO ORDENADO : %u", (u16)best_sal);
    market_transfer(p, best, best_sal, price);
    ui_help("A: continuar");
    ui_delay(data_is_human(best) ? 600u : 120u, 1u);
    return 1u;
}

void screen_auctions(void) {
    u8 n = (u8)(3u + rng_range(3u)), i;
    for (i = 0u; i < n; i++) {
        u16 p = market_pick_auction();
        if (p == PLAYER_NONE) continue;
        run_auction(p, data_price(p), TEAM_NONE);
    }
}

/* ---- Pedidos de aumento ---------------------------------------------- */
void screen_salary_requests(void) {
    u8 m;
    for (m = 0u; m < g_mgr_count; m++) {
        u8 t = g_mgr[m].team, forced = 0u;
        u16 p, s;
        long price;
        Player *pl;
        if (!market_salary_request(t, &p, &s, &price)) continue;
        pl = &g_players[p];
        ui_cls(C_BLUE);
        ui_box(0u, 0u, UI_COLS, 16u, C_GREEN, C_RED);
        ui_textf(1u, 1u, C_WHITE, C_RED, "%s:", g_mgr[m].name);
        ui_textf(1u, 3u, C_WHITE, C_RED, "%s, %s de forca %u,", pl->name, pos_name(pl->pos), (u16)pl->str);
        ui_text(1u, 4u, C_WHITE, C_RED, "pede que lhe seja aumentado o");
        ui_textf(1u, 5u, C_WHITE, C_RED, "ordenado para %u.", (u16)s);
        if (data_squad_count(t) <= MIN_SQUAD) {
            ui_text(1u, 7u, C_WHITE, C_RED, "Como so possui 14 jogadores na");
            ui_text(1u, 8u, C_WHITE, C_RED, "equipa, este aumento tera de ser aceite");
            forced = 1u;
        } else if (pl->pos == POS_GR && data_gk_count(t) <= 1u) {
            ui_text(1u, 7u, C_WHITE, C_RED, "Como so possui um guarda-redes na");
            ui_text(1u, 8u, C_WHITE, C_RED, "equipa, este aumento tera de ser aceite");
            forced = 1u;
        } else {
            ui_text(1u, 7u, C_WHITE, C_RED, "Caso nao aceite o jogador pode decidir");
            ui_textf(1u, 8u, C_WHITE, C_RED, "por-se em leilao pelo preco de %ld", price);
        }
        if (forced || ui_ask_yn(10u, C_WHITE, C_RED, "Aceita o aumento (S/N) ?")) {
            pl->salary = s;
            pl->flags |= PF_RAISED;
            if (forced) { ui_help("A: continuar"); ui_wait_key(); }
        } else if (rng_range(2u)) {
            run_auction(p, price, t);
        } else {
            ui_text(1u, 12u, C_YELLOW, C_RED, "O jogador fica, contrariado.");
            ui_help("A: continuar");
            ui_wait_key();
        }
    }
}

/* ---- CHICOTADAS PSICOLOGICAS ----------------------------------------- */
void screen_sackings(void) {
    u8 i;
    u16 y = 4u;
    if (!g_sack_n) return;
    ui_cls(C_BLUE);
    ui_title(0u, "CHICOTADAS PSICOLOGICAS", C_YELLOW, C_RED, C_WHITE);
    for (i = 0u; i < g_sack_n && y < 25u; i++) {
        Sacking *s = &g_sack[i];
        ui_textw(1u, y, 20u, C_WHITE, C_BLUE, s->old_c < (u8)COACH_COUNT ? g_coaches[s->old_c] : "---");
        ui_text(1u, (u16)(y + 1u), C_WHITE, C_BLUE, "foi despedido do");
        ui_team(18u, (u16)(y + 1u), 16u, s->team);
        if (s->swap != TEAM_NONE) {
            ui_text(1u, (u16)(y + 2u), C_WHITE, C_BLUE, "Troca com");
            ui_team(11u, (u16)(y + 2u), 16u, s->swap);
        } else {
            ui_text(1u, (u16)(y + 2u), C_WHITE, C_BLUE, "Para o seu lugar foi escolhido");
            ui_textw(1u, (u16)(y + 3u), 20u, C_YELLOW, C_BLUE,
                     s->new_c < (u8)COACH_COUNT ? g_coaches[s->new_c] : "---");
        }
        y = (u16)(y + 5u);
    }
    ui_help("A: continuar");
    ui_wait_key();
}

/* ---- Fim de epoca ----------------------------------------------------- */
void screen_season_end(void) {
    u8 m, old_div[MAX_MANAGERS];
    season_close();
    screen_season_prizes();

    /* Despedimento por dividas (inferido) e propostas de outros clubes */
    for (m = 0u; m < g_mgr_count; m++) {
        u8 t = g_mgr[m].team, offer;
        if (g_teams[t].money < -500000L) {
            u8 cand[TEAM_COUNT], n = 0u, o;
            for (o = 0u; o < (u8)TEAM_COUNT; o++)
                if (g_teams[o].div == DIVISIONS - 1u && !data_is_human(o)) cand[n++] = o;
            ui_cls(C_BLUE);
            ui_textf(1u, 8u, C_WHITE, C_BLUE, "%s foi despedido do", g_mgr[m].name);
            ui_team(1u, 9u, 16u, t);
            if (n) {
                managers_move(m, cand[rng_range(n)]);
                ui_text(1u, 11u, C_WHITE, C_BLUE, "Vai treinar o");
                ui_team(15u, 11u, 16u, g_mgr[m].team);
            }
            ui_help("A: continuar");
            ui_wait_key();
            continue;
        }
        offer = managers_job_offer(m);
        if (offer != TEAM_NONE) {
            char q[40];
            ui_cls(C_BLUE);
            ui_team(1u, 6u, 16u, offer);
            ui_textf(1u, 7u, C_WHITE, C_BLUE, "(%ua divisao) faz-lhe uma proposta.", (u16)(g_teams[offer].div + 1u));
            ui_sprintf(q, sizeof(q), "%s: quer ir treinar o", g_mgr[m].name);
            ui_text(1u, 9u, C_YELLOW, C_BLUE, q);
            ui_team(1u, 10u, 16u, offer);
            if (ui_ask_yn(12u, C_YELLOW, C_BLUE, "(s/n) ?")) managers_move(m, offer);
        }
    }

    for (m = 0u; m < g_mgr_count; m++) old_div[m] = g_teams[g_mgr[m].team].div;
    season_promote();
    ui_cls(C_BLUE);
    ui_title(0u, "NOVA EPOCA", C_YELLOW, C_GREEN, C_WHITE);
    ui_textf(14u, 4u, C_WHITE, C_BLUE, "EPOCA %u", (u16)g_season);
    for (m = 0u; m < g_mgr_count; m++) {
        u8 t = g_mgr[m].team, d = g_teams[t].div;
        u16 y = (u16)(7u + m * 3u);
        ui_textw(1u, y, 13u, C_WHITE, C_BLUE, g_mgr[m].name);
        ui_team(15u, y, 16u, t);
        if (d < old_div[m])      ui_textf(1u, (u16)(y + 1u), C_LGREEN, C_BLUE, "SOBE a %ua divisao", (u16)(d + 1u));
        else if (d > old_div[m]) ui_textf(1u, (u16)(y + 1u), C_LRED, C_BLUE, "DESCE a %ua divisao", (u16)(d + 1u));
        else                     ui_textf(1u, (u16)(y + 1u), C_WHITE, C_BLUE, "Fica na %ua divisao", (u16)(d + 1u));
    }
    ui_help("A: continuar");
    ui_wait_key();
}
