/*
 * main.c -- Entry point do Elifoot II Genesis
 *
 * Ordem de inicializa??o OBRIGAT?RIA (SGDK 1.70):
 *   1. input_init()  -> chama JOY_init() internamente -- DEVE SER PRIMEIRO.
 *      Sem JOY_init(), JOY_readJoypad() retorna 0 para sempre.
 *   2. VDP_init()    -> inicializa o VDP.
 *   3. render_init() -> carrega fonte, paletas, configura planes.
 *   4. rng_init()    -> semente via V-counter (variabilidade por momento de ligar).
 *   5. data_init()   -> descompacta ROM -> RAM (equipes, jogadores, treinadores).
 *
 * Assinatura de main():
 *   int main(int hardReset)
 *   SGDK 1.70 / sys.c espera exatamente esta assinatura.
 *   Usar 'int' aqui causaria LTO type mismatch (int=16bit mas a
 *   declara??o em sys.c usa u16 -- conflito reportado como "type of
 *   'main' does not match original declaration" no link step).
 *
 * SYS_doVBlankProcess():
 *   Bloqueia at? o pr?ximo VBlank (~16ms a 60fps).
 *   NUNCA chamar dentro de loops de c?lculo (simula??o, ordena??o).
 *   Chamar apenas uma vez por frame no loop principal abaixo.
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

/* ------------------------------------------------------------------ */
/* Vari?veis de estado global (game/data.c ? quem as define)          */
/* ------------------------------------------------------------------ */
/* Declaradas extern aqui para uso em render_status_bar().            */
/* Definidas com valores iniciais em game/data.c.                     */

/* ------------------------------------------------------------------ */
/* main()                                                              */
/* ------------------------------------------------------------------ */

int main(int hardReset) {
    /* Suprime warning de par?metro n?o usado -- hardReset ? fornecido  */
    /* pelo SGDK mas n?o ? necess?rio no loop principal.               */
    (void)hardReset;

    /* ---- 1. INPUT INIT (PRIMEIRO -- obrigat?rio antes de tudo) ---- */
    input_init();

    /* ---- 2. VDP -------------------------------------------------- */
    VDP_init();

    /* Modo de resolu??o padr?o: 320?224 (40?28 tiles).               */
    /* VDP_init() j? configura isso, mas ? expl?cito para documenta??o.*/
    /* VDP_setScreenWidth(320); -- comentado, VDP_init() j? faz isso.  */

    /* ---- 3. RENDER (carrega fonte, paletas, limpa planes) --------- */
    render_init();

    /* ---- 4. RNG (semente via V-counter) -------------------------- */
    /* GET_VCOUNTER() l? o contador de varredura vertical do VDP.     */
    /* O valor varia com o momento exato em que o console ? ligado,   */
    /* criando variabilidade natural entre partidas.                   */
    rng_init((u16)GET_VCOUNTER);

    /* ---- 5. DADOS (ROM -> RAM) ------------------------------------ */
    data_init();

    /* ---------------------------------------------------------------- */
    /* Tela de t?tulo / sele??o de equipe / continua save              */
    /* ---------------------------------------------------------------- */
    screen_title();

    /* ---------------------------------------------------------------- */
    /* Loop principal de temporadas                                     */
    /* ---------------------------------------------------------------- */
    while (TRUE) {
        season_run();
        /* screen_season_end() mostra pr?mios, promo??o/rebaixamento   */
        /* e pergunta se quer jogar outra temporada.                   */
        screen_season_end();
    }

    /* O loop while(TRUE) nunca termina, mas o compilador pode         */
    /* reclamar de missing return -- retornamos 0 por conformidade.     */
    return 0;
}
