#ifndef INPUT_H
#define INPUT_H

/*
 * input.h — Leitura de controle para Elifoot II Genesis
 *
 * SGDK 1.70 caveat crítico:
 *   JOY_init() DEVE ser chamado antes de qualquer JOY_readJoypad().
 *   Sem isso, JOY_readJoypad() retorna 0 para sempre e o jogo
 *   parece completamente travado na tela inicial.
 *
 * Uso correto:
 *   1. Chamar JOY_init() como PRIMEIRA função no main().
 *   2. Chamar input_update() uma vez por frame, no início do loop.
 *   3. Consultar input_pressed() / input_held() / input_held_long()
 *      para detectar ações do jogador.
 *
 * NUNCA chamar input_update() dentro de loops de cálculo (simulação,
 * ordenação) — só chamar uma vez por frame no loop principal.
 *
 * Mapeamento de botões (controle de 3 ou 6 botões):
 *   D-pad:   navegar menus e listas
 *   A:       confirmar / selecionar  (equivalente a Enter/S no DOS)
 *   B:       cancelar / voltar       (equivalente a Esc/N no DOS)
 *   C:       ação secundária         (auto-escalar, ação especial)
 *   X:       atalho de formação      (apenas controle 6 botões)
 *   Y:       atalho de informações   (apenas controle 6 botões)
 *   Z:       atalho de plantel       (apenas controle 6 botões)
 *   Start:   menu principal / pausa
 */

#include <genesis.h>

/* ------------------------------------------------------------------ */
/* Aliases semânticos para os botões do SGDK                           */
/* ------------------------------------------------------------------ */

/* Navegação                                                            */
#define BTN_UP      BUTTON_UP
#define BTN_DOWN    BUTTON_DOWN
#define BTN_LEFT    BUTTON_LEFT
#define BTN_RIGHT   BUTTON_RIGHT

/* Ações principais                                                     */
#define BTN_CONFIRM  BUTTON_A   /* confirmar / selecionar              */
#define BTN_CANCEL   BUTTON_B   /* cancelar / voltar                   */
#define BTN_ACTION   BUTTON_C   /* ação secundária                     */
#define BTN_START    BUTTON_START

/* Atalhos (controle 6 botões — retorna 0 em controle de 3 botões)    */
#define BTN_FORMATION  BUTTON_X   /* atalho: formação                  */
#define BTN_INFO       BUTTON_Y   /* atalho: informações               */
#define BTN_SQUAD      BUTTON_Z   /* atalho: plantel                   */

/* ------------------------------------------------------------------ */
/* Limiares para hold detection                                        */
/* ------------------------------------------------------------------ */

/*
 * HOLD_INITIAL_FRAMES: frames até o primeiro repeat de hold.
 * 30 frames a 60fps = 0.5 segundos — confortável para evitar
 * aceleração acidental ao pressionar rapidamente.
 */
#define HOLD_INITIAL_FRAMES  30u

/*
 * HOLD_REPEAT_FRAMES: frames entre cada repeat após o inicial.
 * 6 frames = 10 repeats/segundo — boa velocidade para navegar listas.
 */
#define HOLD_REPEAT_FRAMES   6u

/* ------------------------------------------------------------------ */
/* Estrutura de navegação de lista                                     */
/* ------------------------------------------------------------------ */

/*
 * ListNav — estado de um seletor de lista com scroll.
 * Inicializar com list_nav_init() antes de usar.
 */
typedef struct {
    u8 selected;    /* índice do item atualmente selecionado (0-based) */
    u8 count;       /* número total de itens na lista                  */
    u8 page_top;    /* índice do primeiro item visível na página        */
    u8 page_size;   /* número de itens visíveis por vez                */
} ListNav;

/* ------------------------------------------------------------------ */
/* API pública                                                         */
/* ------------------------------------------------------------------ */

/*
 * input_init() — Inicializa o subsistema de input.
 * Chama JOY_init() internamente — não chamar JOY_init() diretamente.
 * Deve ser a PRIMEIRA função chamada em main(), antes de VDP_init().
 */
void input_init(void);

/*
 * input_update() — Lê estado atual do controle e atualiza contadores.
 * Chamar exatamente UMA VEZ por frame, no início do loop principal,
 * ANTES de qualquer consulta a input_pressed()/input_held().
 *
 * PROIBIDO chamar dentro de loops de cálculo ou dentro de callbacks
 * de simulação — provoca inconsistência no edge detection.
 */
void input_update(void);

/*
 * input_pressed() — Retorna valor != 0 se o botão foi pressionado
 * NESTE frame (borda de subida: estava solto, agora está pressionado).
 * Use para ações que devem ocorrer exatamente uma vez por pressão.
 */
u16 input_pressed(u16 btn);

/*
 * input_held() — Retorna valor != 0 se o botão está pressionado agora
 * (independentemente de quando foi pressionado).
 * Use para movimentos contínuos ou detecção de estado.
 */
u16 input_held(u16 btn);

/*
 * input_repeat() — Retorna valor != 0 quando o botão é pressionado
 * pela primeira vez OU após HOLD_INITIAL_FRAMES frames seguidos,
 * repetindo a cada HOLD_REPEAT_FRAMES frames.
 * Ideal para navegação em listas com aceleration natural.
 */
u16 input_repeat(u16 btn);

/*
 * input_any_pressed() — Retorna valor != 0 se qualquer botão foi
 * pressionado neste frame. Útil para telas de "pressione qualquer botão".
 */
u16 input_any_pressed(void);

/* ------------------------------------------------------------------ */
/* Navegação em listas                                                 */
/* ------------------------------------------------------------------ */

/*
 * list_nav_init() — Inicializa estrutura de navegação.
 *   count:     número total de itens
 *   page_size: número de itens visíveis por vez (linhas na tela)
 */
void list_nav_init(ListNav *nav, u8 count, u8 page_size);

/*
 * list_nav_update() — Atualiza a seleção com base no input atual.
 * Retorna valor != 0 se a seleção mudou (útil para re-render).
 * Usa input_repeat() para Down/Up — permite navegação rápida com hold.
 */
u8 list_nav_update(ListNav *nav);

#endif /* INPUT_H */
