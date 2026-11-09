#ifndef ELIFOOT_HISTORY_H
#define ELIFOOT_HISTORY_H

#include <genesis.h>
#include "types.h"

/* Honras de uma equipa (PALMARES). */
typedef struct {
    u8 seasons[DIVISIONS];   /* epocas em cada divisao          */
    u8 titles[DIVISIONS];    /* campeonatos de cada divisao     */
    u8 cup_wins;
    u8 cup_finals;           /* presencas na final (incl. ganhas) */
    u8 cup_semis;
    u8 cup_quarters;
    u8 cup_r2;
    u8 _pad;
} TeamHonours;               /* 14 bytes */

/* Treinador de um titulo: 0..49 = IA, 200+m = humano m */
#define COACH_HUMAN(m) ((u8)(200u + (m)))

typedef struct {
    u8 season;
    u8 champ[DIVISIONS];
    u8 champ_coach[DIVISIONS];
    u8 cup;
    u8 cup_coach;
    u8 _pad;
} WinnerRec;                 /* 12 bytes */

#define WINNERS_MAX 20u

typedef struct {
    u8  first, second, third;
    u8  attack, attack_goals;
    u8  defence, defence_goals;
    u8  scorer_goals;
    u16 scorer;              /* indice de jogador, PLAYER_NONE */
} DivAwards;

extern TeamHonours g_honours[TEAM_COUNT];
extern WinnerRec   g_winners[WINNERS_MAX];
extern u8          g_winners_n;
extern DivAwards   g_awards[DIVISIONS];
extern u8          g_awards_valid;
/* Nomes dos treinadores humanos que ganharam titulos (indice m). */
extern char        g_hist_mgr_name[MAX_MANAGERS][MGR_NAME_LEN];

void history_reset(void);
void history_compute_awards(void);
void history_record_season(void);
u8   history_coach_of(u8 t);
const char *history_coach_name(u8 id);
u8   history_coach_titles(u8 id, u8 cup);

#endif
