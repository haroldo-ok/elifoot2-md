#ifndef ELF_SCREENS_H
#define ELF_SCREENS_H

#include <genesis.h>
#include "../game/types.h"

/* ---- kbd.c: entrada sem teclado ------------------------------------ */
/* Teclado no ecra. Devolve o comprimento (0 = cancelado/vazio). */
u8   kbd_text(const char *title, char *buf, u8 maxlen);
/* Numero com digitos (ESQ/DIR escolhe o digito, CIMA/BAIXO muda).
 * Devolve -1 se cancelado. */
long kbd_number(u16 y, const char *prompt, long initial, u8 digits, u8 fg, u8 bg);

/* ---- common: listas -------------------------------------------------- */
/* Escolhe um jogador do plantel de t. PLAYER_NONE se cancelado. */
u16  pick_player(u8 t, const char *title);
/* Linha de jogador: "D Nome            15  1500  0 *" */
void draw_player_row(u16 x, u16 y, u16 p, u8 fg, u8 bg, u8 show_pos);
char pos_letter(u8 pos);
const char *pos_name(u8 pos);
/* "1a ELIMINATORIA (1a MAO)" etc. */
void cup_leg_label(char *out, u8 phase, u8 leg);

/* ---- start.c -------------------------------------------------------- */
#define START_NEW  0u
#define START_LOAD 1u
u8   screen_title(void);
u8   screen_load(void);          /* 1 se carregou */
void screen_managers_setup(void);
void screen_save(void);

/* ---- main_screen.c --------------------------------------------------- */
#define MAIN_PLAY 0u
#define MAIN_QUIT 1u
u8   screen_main(u8 mgr);

/* ---- tactic.c ------------------------------------------------------- */
u8   screen_tactic(u8 t);        /* 1 = confirmou, 0 = voltou */
void screen_subs(u8 game, u8 t, u8 *subs_used);

/* ---- live.c --------------------------------------------------------- */
void screen_live_round(void);

/* ---- cup_screens.c -------------------------------------------------- */
void screen_cup_draw(void);
void screen_cup_after_phase(u8 phase);
void screen_cup_status(void);

/* ---- league_screens.c ----------------------------------------------- */
void screen_standings_all(void);
void screen_standings_div(u8 div);
void screen_calendar(u8 t);
void screen_results_grid(u8 div);
void screen_next_rounds(u8 t);
void screen_scorers(void);

/* ---- history_screens.c --------------------------------------------- */
void screen_palmares(u8 t);
void screen_last_winners(void);
void screen_team_titles(void);
void screen_coach_titles(void);
void screen_prizes_table(void);
void screen_season_prizes(void);

/* ---- finance_screens.c --------------------------------------------- */
void screen_sell(u8 t);
void screen_alter_salary(u8 t);
void screen_transfers(void);
void screen_revenues(u8 mgr);
void screen_stadium(u8 t);

/* ---- events.c ------------------------------------------------------- */
void screen_auctions(void);
/* Leilao de um jogador. seller_human = clube humano que vende (ou TEAM_NONE).
 * Devolve 1 se foi transferido. */
u8   run_auction(u16 p, long price, u8 seller_human);
void screen_salary_requests(void);
void screen_sackings(void);
void screen_season_end(void);

/* ---- managers_screen.c ---------------------------------------------- */
void screen_managers(void);

#endif
