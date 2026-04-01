/*
 * main.c — Entry point do Elifoot II Genesis
 *
 * Ordem de inicialização OBRIGATÓRIA (SGDK 1.70):
 *   1. input_init()  → chama JOY_init() internamente — DEVE SER PRIMEIRO.
 *      Sem JOY_init(), JOY_readJoypad() retorna 0 para sempre.
 *   2. VDP_init()    → inicializa o VDP.
 *   3. render_init() → carrega fonte, paletas, configura planes.
 *   4. rng_init()    → semente via V-counter (variabilidade por momento de ligar).
 *   5. data_init()   → descompacta ROM → RAM (equipes, jogadores, treinadores).
 *
 * Assinatura de main():
 *   u16 main(u16 hardReset)
 *   SGDK 1.70 / sys.c espera exatamente esta assinatura.
 *   Usar 'int' aqui causaria LTO type mismatch (int=16bit mas a
 *   declaração em sys.c usa u16 — conflito reportado como "type of
 *   'main' does not match original declaration" no link step).
 *
 * SYS_doVBlankProcess():
 *   Bloqueia até o próximo VBlank (~16ms a 60fps).
 *   NUNCA chamar dentro de loops de cálculo (simulação, ordenação).
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
/* Variáveis de estado global (game/data.c é quem as define)          */
/* ------------------------------------------------------------------ */
/* Declaradas extern aqui para uso em render_status_bar().            */
/* Definidas com valores iniciais em game/data.c.                     */
u8   g_season_num      = 1u;
u8   g_round           = 0u;
u8   g_player_team_idx = 0u;
u8   g_division        = 0u;
long g_money           = 0L;

/* ------------------------------------------------------------------ */
/* main()                                                              */
/* ------------------------------------------------------------------ */

u16 main(u16 hardReset) {
    /* Suprime warning de parâmetro não usado — hardReset é fornecido  */
    /* pelo SGDK mas não é necessário no loop principal.               */
    (void)hardReset;

    /* ---- 1. INPUT INIT (PRIMEIRO — obrigatório antes de tudo) ---- */
    input_init();

    /* ---- 2. VDP -------------------------------------------------- */
    VDP_init();

    /* Modo de resolução padrão: 320×224 (40×28 tiles).               */
    /* VDP_init() já configura isso, mas é explícito para documentação.*/
    /* VDP_setScreenWidth(320); — comentado, VDP_init() já faz isso.  */

    /* ---- 3. RENDER (carrega fonte, paletas, limpa planes) --------- */
    render_init();

    /* ---- 4. RNG (semente via V-counter) -------------------------- */
    /* GET_VCOUNTER() lê o contador de varredura vertical do VDP.     */
    /* O valor varia com o momento exato em que o console é ligado,   */
    /* criando variabilidade natural entre partidas.                   */
    rng_init((u16)GET_VCOUNTER());

    /* ---- 5. DADOS (ROM → RAM) ------------------------------------ */
    data_init();

    /* ---------------------------------------------------------------- */
    /* Tela de título / seleção de equipe / continua save              */
    /* ---------------------------------------------------------------- */
    screen_title();

    /* ---------------------------------------------------------------- */
    /* Loop principal de temporadas                                     */
    /* ---------------------------------------------------------------- */
    while (TRUE) {
        season_run();
        /* screen_season_end() mostra prêmios, promoção/rebaixamento   */
        /* e pergunta se quer jogar outra temporada.                   */
        screen_season_end();
    }

    /* O loop while(TRUE) nunca termina, mas o compilador pode         */
    /* reclamar de missing return — retornamos 0 por conformidade.     */
    return 0u;
}
