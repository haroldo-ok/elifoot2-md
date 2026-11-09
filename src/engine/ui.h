#ifndef ELF_UI_H
#define ELF_UI_H

#include <genesis.h>

/*
 * engine/ui.h -- Texto em 40x28 com cores CGA arbitrarias.
 *
 * O original desenha tudo em cores CGA (texto 0-15 sobre fundo 0-15), e os
 * nomes dos clubes nas cores do clube. Aqui cada combinacao
 * (caracter, cor de texto, cor de fundo) gera um tile em VRAM na primeira
 * vez que e usada; a cache e esvaziada em ui_cls().
 * Tudo usa a PAL0, que contem as 16 cores CGA.
 */

#define UI_COLS      40
#define UI_ROWS      28

/* Cores CGA */
#define C_BLACK    0u
#define C_BLUE     1u
#define C_GREEN    2u
#define C_CYAN     3u
#define C_RED      4u
#define C_MAGENTA  5u
#define C_BROWN    6u
#define C_LGRAY    7u
#define C_DGRAY    8u
#define C_LBLUE    9u
#define C_LGREEN  10u
#define C_LCYAN   11u
#define C_LRED    12u
#define C_LMAGENTA 13u
#define C_YELLOW  14u
#define C_WHITE   15u

/* Caracteres de desenho de caixas (glifos 128..140 da fonte). */
#define CH_H     ((char)128)   /* ─ */
#define CH_V     ((char)129)   /* │ */
#define CH_TL    ((char)130)   /* ┌ */
#define CH_TR    ((char)131)   /* ┐ */
#define CH_BL    ((char)132)   /* └ */
#define CH_BR    ((char)133)   /* ┘ */
#define CH_DH    ((char)134)   /* ═ */
#define CH_DV    ((char)135)   /* ║ */
#define CH_DTL   ((char)136)   /* ╔ */
#define CH_DTR   ((char)137)   /* ╗ */
#define CH_DBL   ((char)138)   /* ╚ */
#define CH_DBR   ((char)139)   /* ╝ */
#define CH_X     ((char)140)   /* ┼ */
#define CH_BLOCK ((char)141)   /* bloco cheio (barras do estadio) */

/* Modo de teste: compilado com -DELF_AUTOPLAY, o jogo joga-se sozinho
 * (todas as perguntas recebem a resposta por omissao). */
extern u8 g_autoplay;

void ui_init(void);

/* Limpa o ecra com a cor de fundo bg e esvazia a cache de tiles. */
void ui_cls(u8 bg);

void ui_putc_c(u16 x, u16 y, u8 fg, u8 bg, char c);
void ui_text(u16 x, u16 y, u8 fg, u8 bg, const char *s);
/* Escreve s em exactamente w colunas (corta ou completa com espacos). */
void ui_textw(u16 x, u16 y, u16 w, u8 fg, u8 bg, const char *s);
void ui_textf(u16 x, u16 y, u8 fg, u8 bg, const char *fmt, ...);
/* Formata para buffer (mesmos formatos: %s %u %d %lu %ld, largura, '-'). */
void ui_sprintf(char *out, u16 cap, const char *fmt, ...);

void ui_fill(u16 x, u16 y, u16 w, u16 h, u8 bg);
/* Caixa com moldura simples (fg) e interior bg. */
void ui_box(u16 x, u16 y, u16 w, u16 h, u8 fg, u8 bg);
/* Titulo centrado numa caixa de 3 linhas. */
void ui_title(u16 y, const char *s, u8 fg, u8 bg, u8 frame);
void ui_hbar(u16 x, u16 y, u16 len, u8 fg, u8 bg);

/* Nome do clube t nas cores do clube, em w colunas. */
void ui_team(u16 x, u16 y, u16 w, u8 team);
u8   ui_team_fg(u8 team);
u8   ui_team_bg(u8 team);

/* Barra de ajuda na ultima linha. */
void ui_help(const char *s);

void ui_wait_vblank(void);
/* Espera n frames (sai mais cedo se carregar A/B/C/START e skip=1). */
u8   ui_delay(u16 frames, u8 skip);
/* Espera A, B, C ou START. Devolve o botao. */
u16  ui_wait_key(void);
/* Pergunta S/N: A = sim (1), B = nao (0). */
u8   ui_ask_yn(u16 y, u8 fg, u8 bg, const char *q);

/* ---- Compatibilidade com o codigo antigo ---------------------------- */
#define UI_PAL_NORMAL   0u    /* branco sobre azul    */
#define UI_PAL_SELECT   1u    /* preto sobre ciano    */
void ui_clear(void);
void ui_puts(u16 x, u16 y, u16 pal, const char *str);
void ui_putc(u16 x, u16 y, u16 pal, char c);
void ui_printf(u16 x, u16 y, u16 pal, const char *fmt, ...);
void ui_fill_row(u16 y, u16 pal);
void ui_hline(u16 x, u16 y, u16 len, u16 pal);

#endif /* ELF_UI_H */
