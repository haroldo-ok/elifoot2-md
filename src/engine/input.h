#ifndef INPUT_H
#define INPUT_H

/*
 * input.h -- Leitura de controle para Elifoot II Genesis
 *
 * SGDK 1.70 caveat cr?tico:
 *   JOY_init() DEVE ser chamado antes de qualquer JOY_readJoypad().
 *   Sem isso, JOY_readJoypad() retorna 0 para sempre e o jogo
 *   parece completamente travado na tela inicial.
 *
 * Uso correto:
 *   1. Chamar JOY_init() como PRIMEIRA fun??o no main().
 *   2. Chamar input_update() uma vez por frame, no in?cio do loop.
 *   3. Consultar input_pressed() / input_held() / input_held_long()
 *      para detectar a??es do jogador.
 *
 * NUNCA chamar input_update() dentro de loops de c?lculo (simula??o,
 * ordena??o) -- s? chamar uma vez por frame no loop principal.
 *
 * Mapeamento de bot?es (controle de 3 ou 6 bot?es):
 *   D-pad:   navegar menus e listas
 *   A:       confirmar / selecionar  (equivalente a Enter/S no DOS)
 *   B:       cancelar / voltar       (equivalente a Esc/N no DOS)
 *   C:       a??o secund?ria         (auto-escalar, a??o especial)
 *   X:       atalho de forma??o      (apenas controle 6 bot?es)
 *   Y:       atalho de informa??es   (apenas controle 6 bot?es)
 *   Z:       atalho de plantel       (apenas controle 6 bot?es)
 *   Start:   menu principal / pausa
 */

#include <genesis.h>

/* ------------------------------------------------------------------ */
/* Aliases sem?nticos para os bot?es do SGDK                           */
/* ------------------------------------------------------------------ */

/* Navega??o                                                            */
#define BTN_UP      BUTTON_UP
#define BTN_DOWN    BUTTON_DOWN
#define BTN_LEFT    BUTTON_LEFT
#define BTN_RIGHT   BUTTON_RIGHT

/* A??es principais                                                     */
#define BTN_CONFIRM  BUTTON_A   /* confirmar / selecionar              */
#define BTN_CANCEL   BUTTON_B   /* cancelar / voltar                   */
#define BTN_ACTION   BUTTON_C   /* a??o secund?ria                     */
#define BTN_START    BUTTON_START

/* Atalhos (controle 6 bot?es -- retorna 0 em controle de 3 bot?es)    */
#define BTN_FORMATION  BUTTON_X   /* atalho: forma??o                  */
#define BTN_INFO       BUTTON_Y   /* atalho: informa??es               */
#define BTN_SQUAD      BUTTON_Z   /* atalho: plantel                   */

/* ------------------------------------------------------------------ */
/* Limiares para hold detection                                        */
/* ------------------------------------------------------------------ */

/*
 * HOLD_INITIAL_FRAMES: frames at? o primeiro repeat de hold.
 * 30 frames a 60fps = 0.5 segundos -- confort?vel para evitar
 * acelera??o acidental ao pressionar rapidamente.
 */
#define HOLD_INITIAL_FRAMES  30u

/*
 * HOLD_REPEAT_FRAMES: frames entre cada repeat ap?s o inicial.
 * 6 frames = 10 repeats/segundo -- boa velocidade para navegar listas.
 */
#define HOLD_REPEAT_FRAMES   6u

/* ------------------------------------------------------------------ */
/* Estrutura de navega??o de lista                                     */
/* ------------------------------------------------------------------ */

/*
 * ListNav -- estado de um seletor de lista com scroll.
 * Inicializar com list_nav_init() antes de usar.
 */
typedef struct {
    u8 selected;    /* ?ndice do item atualmente selecionado (0-based) */
    u8 count;       /* n?mero total de itens na lista                  */
    u8 page_top;    /* ?ndice do primeiro item vis?vel na p?gina        */
    u8 page_size;   /* n?mero de itens vis?veis por vez                */
} ListNav;

/* ------------------------------------------------------------------ */
/* API p?blica                                                         */
/* ------------------------------------------------------------------ */

/*
 * input_init() -- Inicializa o subsistema de input.
 * Chama JOY_init() internamente -- n?o chamar JOY_init() diretamente.
 * Deve ser a PRIMEIRA fun??o chamada em main(), antes de VDP_init().
 */
void input_init(void);

/*
 * input_update() -- L? estado atual do controle e atualiza contadores.
 * Chamar exatamente UMA VEZ por frame, no in?cio do loop principal,
 * ANTES de qualquer consulta a input_pressed()/input_held().
 *
 * PROIBIDO chamar dentro de loops de c?lculo ou dentro de callbacks
 * de simula??o -- provoca inconsist?ncia no edge detection.
 */
void input_update(void);

/*
 * input_pressed() -- Retorna valor != 0 se o bot?o foi pressionado
 * NESTE frame (borda de subida: estava solto, agora est? pressionado).
 * Use para a??es que devem ocorrer exatamente uma vez por press?o.
 */
u16 input_pressed(u16 btn);

/*
 * input_held() -- Retorna valor != 0 se o bot?o est? pressionado agora
 * (independentemente de quando foi pressionado).
 * Use para movimentos cont?nuos ou detec??o de estado.
 */
u16 input_held(u16 btn);

/*
 * input_repeat() -- Retorna valor != 0 quando o bot?o ? pressionado
 * pela primeira vez OU ap?s HOLD_INITIAL_FRAMES frames seguidos,
 * repetindo a cada HOLD_REPEAT_FRAMES frames.
 * Ideal para navega??o em listas com aceleration natural.
 */
u16 input_repeat(u16 btn);

/*
 * input_any_pressed() -- Retorna valor != 0 se qualquer bot?o foi
 * pressionado neste frame. ?til para telas de "pressione qualquer bot?o".
 */
u16 input_any_pressed(void);

/* ------------------------------------------------------------------ */
/* Navega??o em listas                                                 */
/* ------------------------------------------------------------------ */

/*
 * list_nav_init() -- Inicializa estrutura de navega??o.
 *   count:     n?mero total de itens
 *   page_size: n?mero de itens vis?veis por vez (linhas na tela)
 */
void list_nav_init(ListNav *nav, u8 count, u8 page_size);

/*
 * list_nav_update() -- Atualiza a sele??o com base no input atual.
 * Retorna valor != 0 se a sele??o mudou (?til para re-render).
 * Usa input_repeat() para Down/Up -- permite navega??o r?pida com hold.
 */
u8 list_nav_update(ListNav *nav);

#endif /* INPUT_H */
