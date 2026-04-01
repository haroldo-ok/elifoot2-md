/*
 * main.c -- Entry point do Elifoot II Genesis (DEBUG BUILD)
 *
 * KLog() escreve no porto de debug do emulador (Gens: CPU->Debug->Log).
 * Signatures:
 *   void KLog(char *text)
 *   void KLog_U1(char *t1, u32 v1)
 *   void KLog_U2(char *t1, u32 v1, char *t2, u32 v2)
 *   void KLog_S1(char *t1, s32 v1)
 *
 * int main(int hardReset) -- deve corresponder ao sys.c do SGDK.
 */

#include <genesis.h>
#include "engine/input.h"
#include "engine/render.h"
#include "engine/rng.h"
#include "engine/compat.h"
#include "game/data.h"
#include "screens/title.h"
#include "screens/main_menu.h"
#include "game/season.h"

int main(int hardReset) {
    (void)hardReset;

    KLog("main: START");

    /* 1. Input init -- OBRIGATORIO PRIMEIRO */
    KLog("main: calling input_init");
    input_init();
    KLog("main: input_init OK");

    /* 2. VDP */
    KLog("main: calling VDP_init");
    VDP_init();
    KLog("main: VDP_init OK");

    /* 3. Render -- carrega fonte, paletas, limpa planes */
    KLog("main: calling render_init");
    render_init();
    KLog("main: render_init OK");

    /* 4. RNG */
    KLog("main: calling rng_init");
    rng_init((u16)GET_VCOUNTER);
    KLog("main: rng_init OK");

    /* 5. Data -- carrega ROM -> RAM */
    KLog("main: calling data_init");
    data_init();
    KLog("main: data_init OK");

    /* 6. Tela de titulo */
    KLog("main: calling screen_title");
    screen_title();
    KLog("main: screen_title returned");

    /* 7. Loop principal */
    KLog("main: entering season loop");
    while (TRUE) {
        KLog("main: season_run start");
        season_run();
        KLog("main: screen_season_end start");
        screen_season_end();
    }

    return 0;
}
