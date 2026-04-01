/*
 * input.c -- Leitura de controle para Elifoot II Genesis
 *
 * SGDK 1.70 / m68k caveats:
 *   - JOY_init() chamado em input_init() -- nunca chamar diretamente.
 *   - JOY_readJoypad() retorna u32 no SGDK 1.70 -- usar BUTTON_* para
 *     m?scaras, n?o valores literais.
 *   - int = 16 bits: held_frames usa u16, nunca int.
 *   - Nenhuma chamada a SYS_doVBlankProcess() aqui -- s? no main loop.
 */

#include <genesis.h>
#include "input.h"

/* ------------------------------------------------------------------ */
/* Estado interno                                                      */
/* ------------------------------------------------------------------ */

static u16 s_prev_state;     /* estado do controle no frame anterior   */
static u16 s_curr_state;     /* estado do controle no frame atual      */

/*
 * s_held_frames[]: contador de frames que cada bot?o est? pressionado.
 * Indexado pelos bits do valor retornado por JOY_readJoypad().
 * SGDK define BUTTON_* como pot?ncias de 2 -- usamos uma array de 16
 * posi??es correspondente ao n?mero de bot?es poss?veis.
 *
 * Mapeamento dos bits de JOY_readJoypad() para ?ndice de array:
 *   BUTTON_UP    = 0x0001  -> bit 0  -> ?ndice 0
 *   BUTTON_DOWN  = 0x0002  -> bit 1  -> ?ndice 1
 *   BUTTON_LEFT  = 0x0004  -> bit 2  -> ?ndice 2
 *   BUTTON_RIGHT = 0x0008  -> bit 3  -> ?ndice 3
 *   BUTTON_A     = 0x0040  -> bit 6  -> ?ndice 6
 *   BUTTON_B     = 0x0010  -> bit 4  -> ?ndice 4
 *   BUTTON_C     = 0x0020  -> bit 5  -> ?ndice 5
 *   BUTTON_START = 0x0080  -> bit 7  -> ?ndice 7
 *   BUTTON_X     = 0x0400  -> bit 10 -> ?ndice 10
 *   BUTTON_Y     = 0x0200  -> bit 9  -> ?ndice 9
 *   BUTTON_Z     = 0x0100  -> bit 8  -> ?ndice 8
 *   BUTTON_MODE  = 0x0800  -> bit 11 -> ?ndice 11
 */
#define HELD_ARRAY_SIZE  16u
static u16 s_held_frames[HELD_ARRAY_SIZE];

/* ------------------------------------------------------------------ */
/* input_init()                                                        */
/* ------------------------------------------------------------------ */

void input_init(void) {
    u8 i;

    /* CR?TICO: JOY_init() DEVE ser chamado antes de JOY_readJoypad(). */
    /* Sem isso, JOY_readJoypad() sempre retorna 0.                    */
    JOY_init();

    s_prev_state = 0u;
    s_curr_state = 0u;
    for (i = 0u; i < HELD_ARRAY_SIZE; i++) {
        s_held_frames[i] = 0u;
    }
}

/* ------------------------------------------------------------------ */
/* input_update()                                                      */
/* ------------------------------------------------------------------ */

void input_update(void) {
    u8 i;
    u16 bit;

    s_prev_state = s_curr_state;
    s_curr_state = JOY_readJoypad(JOY_1);

    /* Atualiza contadores de hold para cada bit de bot?o.             */
    for (i = 0u; i < HELD_ARRAY_SIZE; i++) {
        bit = (u16)(1u << i);
        if (s_curr_state & bit) {
            /* Satura em 0xFFFF para evitar overflow no longo prazo.   */
            if (s_held_frames[i] < 0xFFFFu) {
                s_held_frames[i]++;
            }
        } else {
            s_held_frames[i] = 0u;
        }
    }
}

/* ------------------------------------------------------------------ */
/* input_pressed()                                                     */
/* ------------------------------------------------------------------ */

u16 input_pressed(u16 btn) {
    /* Borda de subida: estava 0 no frame anterior, agora ? 1.         */
    return (s_curr_state & btn) & (u16)(~s_prev_state & btn);
}

/* ------------------------------------------------------------------ */
/* input_held()                                                        */
/* ------------------------------------------------------------------ */

u16 input_held(u16 btn) {
    return s_curr_state & btn;
}

/* ------------------------------------------------------------------ */
/* input_repeat()                                                      */
/* ------------------------------------------------------------------ */

u16 input_repeat(u16 btn) {
    /*
     * Retorna != 0 em dois casos:
     *   1. Borda de subida (primeiro press neste frame).
     *   2. Bot?o mantido pressionado por >= HOLD_INITIAL_FRAMES frames
     *      E o frame atual ? m?ltiplo de HOLD_REPEAT_FRAMES desde ent?o.
     *
     * Implementa??o: encontra o bit de ?ndice mais baixo de 'btn' e
     * consulta o contador correspondente em s_held_frames[].
     *
     * Para o caso comum de btn = um ?nico bit (BUTTON_UP, etc.),
     * isso ? eficiente. Para m?scaras multi-bit, testa o bit mais baixo.
     */
    u8 i;
    u16 bit;

    /* Primeiro press -- sempre ativa.                                  */
    if (input_pressed(btn)) return btn;

    /* Hold repeat -- encontra o ?ndice do primeiro bit de btn.         */
    for (i = 0u; i < HELD_ARRAY_SIZE; i++) {
        bit = (u16)(1u << i);
        if ((btn & bit) == 0u) continue;
        if ((s_curr_state & bit) == 0u) continue;

        /* Bot?o pressionado -- verifica threshold de hold.             */
        if (s_held_frames[i] >= (u16)HOLD_INITIAL_FRAMES) {
            u16 frames_after_initial =
                s_held_frames[i] - (u16)HOLD_INITIAL_FRAMES;
            if ((frames_after_initial % (u16)HOLD_REPEAT_FRAMES) == 0u) {
                return bit;
            }
        }
        break; /* testa apenas o bit mais baixo para efici?ncia        */
    }
    return 0u;
}

/* ------------------------------------------------------------------ */
/* input_any_pressed()                                                 */
/* ------------------------------------------------------------------ */

u16 input_any_pressed(void) {
    return s_curr_state & (u16)(~s_prev_state);
}

/* ------------------------------------------------------------------ */
/* list_nav_init()                                                     */
/* ------------------------------------------------------------------ */

void list_nav_init(ListNav *nav, u8 count, u8 page_size) {
    nav->selected  = 0u;
    nav->count     = count;
    nav->page_top  = 0u;
    nav->page_size = page_size;
}

/* ------------------------------------------------------------------ */
/* list_nav_update()                                                   */
/* ------------------------------------------------------------------ */

u8 list_nav_update(ListNav *nav) {
    u8 changed = 0u;

    if (nav->count == 0u) return 0u;

    /* Navegar para baixo                                              */
    if (input_repeat(BTN_DOWN) && nav->selected < (u8)(nav->count - 1u)) {
        nav->selected++;
        if (nav->selected >= (u8)(nav->page_top + nav->page_size)) {
            nav->page_top++;
        }
        changed = 1u;
    }

    /* Navegar para cima                                               */
    if (input_repeat(BTN_UP) && nav->selected > 0u) {
        nav->selected--;
        if (nav->selected < nav->page_top) {
            nav->page_top--;
        }
        changed = 1u;
    }

    /* Pular uma p?gina para baixo (Left = Page Down para listas)     */
    if (input_pressed(BTN_RIGHT)) {
        u8 new_sel = nav->selected + nav->page_size;
        if (new_sel >= nav->count) new_sel = (u8)(nav->count - 1u);
        if (new_sel != nav->selected) {
            nav->selected = new_sel;
            /* ajusta page_top para mostrar o selecionado              */
            if (nav->selected >= (u8)(nav->page_top + nav->page_size)) {
                nav->page_top = (u8)(nav->selected - nav->page_size + 1u);
            }
            changed = 1u;
        }
    }

    /* Pular uma p?gina para cima                                      */
    if (input_pressed(BTN_LEFT)) {
        u8 new_sel;
        if (nav->selected >= nav->page_size) {
            new_sel = (u8)(nav->selected - nav->page_size);
        } else {
            new_sel = 0u;
        }
        if (new_sel != nav->selected) {
            nav->selected = new_sel;
            if (nav->selected < nav->page_top) {
                nav->page_top = nav->selected;
            }
            changed = 1u;
        }
    }

    return changed;
}
