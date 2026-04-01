/*
 * render.c -- Sistema de renderiza??o textual para Elifoot II Genesis
 *
 * SGDK 1.70 / m68k-elf-gcc caveats respeitados:
 *   - int = 16 bits: todos os contadores de loop e ?ndices usam u16/u8.
 *   - VDP_loadTileSet() usa DMA -- n?o chamar dentro de VBlank.
 *   - VDP_setTileMapXY() escreve diretamente no tilemap -- nenhuma
 *     chamada a SYS_doVBlankProcess() dentro de loops de render.
 *   - VDP_clearTileMapRect(WINDOW, 0, 0, 40, 28) ? obrigat?rio ao trocar de tela
 *     para evitar res?duos de texto.
 *   - PAL_setColors() com DMA em vez de PAL_setPalette() para arrays
 *     de u16 -- PAL_setPalette() espera estrutura Palette do SGDK.
 */

#include <genesis.h>
#include "render.h"

/* Declara??o externa da estrutura de game state m?nima necess?ria
 * para render_status_bar(). Definida em game/data.h.
 * Inclu?da aqui apenas para os campos de leitura (season, round, etc.). */
extern u8  g_season_num;       /* n?mero da temporada atual             */
extern u8  g_round;            /* jornada atual                         */
extern u8  g_player_team_idx;  /* ?ndice da equipe do jogador           */
extern u8  g_division;         /* divis?o atual da equipe do jogador    */
extern long g_money;           /* dinheiro atual da equipe do jogador   */

/* Forward-declared from res/resources.h (gerado pelo rescomp).
 * TILESET font_tiles -- carregada via resources.res */
extern const TileSet font_tiles;

/* ------------------------------------------------------------------ */
/* Dados de paleta (formato Genesis: 0x0BGR, 9 bits por cor)           */
/*                                                                      */
/* ATEN??O: o format word do VDP Genesis ? 0x0BGR (Blue nos bits       */
/* altos), N?O 0x0RGB. Azul escuro (#0000AA) = 0x0600 (B=6).          */
/* ------------------------------------------------------------------ */

static const u16 pal0_data[16] = {
    0x0000,  /* [0]  preto          -- background transparente padr?o    */
    0x0600,  /* [1]  azul escuro    -- background alternativo (t?tulo)   */
    0x0EEE,  /* [2]  branco         -- texto padr?o                      */
    0x02EE,  /* [3]  amarelo        -- t?tulos, cabe?alhos               */
    0x0EE2,  /* [4]  ciano brilhante-- subt?tulos, informa??es           */
    0x02E2,  /* [5]  verde brilhante-- valores positivos                 */
    0x022E,  /* [6]  vermelho br.   -- valores negativos                 */
    0x0666,  /* [7]  cinza claro    -- texto secund?rio                  */
    0x0606,  /* [8]  magenta        -- alertas                           */
    0x0660,  /* [9]  ciano escuro   -- bordas de caixa, separadores      */
    0x0222,  /* [10] cinza escuro   -- coment?rios muito discretos       */
    0x0E2E,  /* [11] magenta br.    -- reserva                           */
    0x0026,  /* [12] marrom         -- reserva                           */
    0x0060,  /* [13] verde escuro   -- reserva                           */
    0x0E00,  /* [14] azul brilhante -- reserva                           */
    0x0EEE,  /* [15] branco (dup.)  -- cursor animado                    */
};

static const u16 pal1_data[16] = {
    0x0660,  /* [0]  ciano escuro   -- background dominante (cursor/bar) */
    0x0000,  /* [1]  preto          -- texto sobre ciano (invers?o)      */
    0x0EEE,  /* [2]  branco         -- texto secund?rio sobre ciano      */
    0x02EE,  /* [3]  amarelo        -- destaque sobre ciano              */
    0x02E2,  /* [4]  verde brilhante-- valores positivos sobre ciano     */
    0x022E,  /* [5]  vermelho br.   -- valores negativos sobre ciano     */
    0x0EE2,  /* [6]  ciano brilhante-- reserva                           */
    0x0EEE,  /* [7]  branco         -- reserva                           */
    /* [8..15] espelha [0..7] para uso futuro                           */
    0x0660, 0x0000, 0x0EEE, 0x02EE,
    0x02E2, 0x022E, 0x0EE2, 0x0EEE,
};

/* PAL2 e PAL3 s?o inicializadas como c?pias de PAL0 e podem ser       */
/* personalizadas por equipe. Definidas como mut?veis (n?o const).     */
static u16 pal2_data[16];
static u16 pal3_data[16];

/* ------------------------------------------------------------------ */
/* Tile de fundo s?lido                                                 */
/* ------------------------------------------------------------------ */

/*
 * SOLID_TILE_IDX: tile 0 da VRAM -- por conven??o do SGDK, o tile 0 ?
 * sempre o tile "vazio" (todos os pixels em cor 0). Usar como background
 * transparente. Para tile s?lido com cor, usamos o tile 0 com paleta
 * diferente, aproveitando que cor [0] de cada paleta ? o background.
 */
#define SOLID_TILE_IDX  0u

/* ------------------------------------------------------------------ */
/* Implementa??o interna                                               */
/* ------------------------------------------------------------------ */

/*
 * write_tile() -- Escreve um ?nico tile no tilemap.
 * Macro inline para performance -- evita overhead de call em loops.
 */
#define write_tile(plane, tile_idx, pal, prio, cx, cy) \
    VDP_setTileMapXY((plane), \
        ELF_TILE((tile_idx), (pal), (prio)), \
        (cx), (cy))

/* ------------------------------------------------------------------ */
/* render_init()                                                       */
/* ------------------------------------------------------------------ */

void render_init(void) {
    KLog("render_init: START");
    /* Inicializa PAL2/PAL3 como c?pias de PAL0.                       */
    u16 i;
    for (i = 0; i < 16; i++) {
        pal2_data[i] = pal0_data[i];
        pal3_data[i] = pal0_data[i];
    }

    /* Carrega paletas no hardware via DMA.                             */
    PAL_setColors(0,  pal0_data, 16, DMA);
    PAL_setColors(16, pal1_data, 16, DMA);
    PAL_setColors(32, pal2_data, 16, DMA);
    PAL_setColors(48, pal3_data, 16, DMA);

    /* Carrega fonte na VRAM a partir do tile FONT_BASE_TILE.           */
    /* font_tiles ? definida em res/resources.h pelo rescomp.           */
    KLog_U1("render_init: loading font at tile", (u32)FONT_BASE_TILE);
    VDP_loadTileSet(&font_tiles, FONT_BASE_TILE, DMA);
    KLog_U1("render_init: font numTile=", (u32)font_tiles.numTile);

    /* Configura o WINDOW plane para aparecer sobre BG_A.              */
    /* Posi??o do WINDOW: cobre linhas 0-2 (topo) e 25-27 (rodap?).   */
    /* O SGDK permite configurar onde o WINDOW come?a verticalmente.   */
    /* N?o setamos aqui pois o padr?o do SGDK j? cobre o WINDOW toda   */
    /* a tela -- renderizaremos nas coordenadas corretas diretamente.   */

    /* Limpa todos os planes.                                           */
    VDP_fillTileMapRect(BG_A, ELF_TILE(FONT_BASE_TILE, PAL0, 0), 0, 0, 40, 28);
    VDP_fillTileMapRect(BG_B, ELF_TILE(FONT_BASE_TILE, PAL0, 0), 0, 0, 40, 28);
    VDP_clearTileMapRect(WINDOW, 0, 0, 40, 28);

    /* Push WINDOW off-screen -- by default SGDK sets WINDOW to cover
     * the full display, which overlays BG_A and hides all text drawn there.
     * VDP_setWindowVPos(TRUE, 28) moves the WINDOW start to row 28
     * (below visible area), making BG_A fully visible for rows 0-27.
     * All UI text (status bar, help bar, content) goes directly on BG_A. */
    VDP_setWindowVPos(TRUE, 28);
}

/* ------------------------------------------------------------------ */
/* render_text()                                                       */
/* ------------------------------------------------------------------ */

void render_text(VDPPlane plane, const char *str, u16 x, u16 y, u16 pal_idx) {
    u16 cx = x;
    const char *p = str;
    u8 c;

    while (*p != '\0') {
        c = (u8)*p++;

        /* Pula chars fora do range da fonte (32-127).                 */
        /* Avan?a x para manter alinhamento com o comprimento da string.*/
        if (c < 32u || c > 127u) {
            if (cx < SCREEN_COLS) cx++;
            continue;
        }

        if (cx >= SCREEN_COLS) break;  /* n?o escrever al?m da borda  */

        write_tile(plane, CHAR_TO_TILE(c), pal_idx, 0, cx, y);
        cx++;
    }
}

/* ------------------------------------------------------------------ */
/* render_textf()                                                      */
/* ------------------------------------------------------------------ */

void render_textf(VDPPlane plane, u16 x, u16 y, u16 pal_idx,
                  const char *fmt, ...) {
    char buf[64];
    va_list args;
    va_start(args, fmt);
    (void)vsprintf(buf, fmt, args);
    va_end(args);
    buf[63] = '\0';  /* garantia de NUL-termination                    */
    render_text(plane, buf, x, y, pal_idx);
}

/* ------------------------------------------------------------------ */
/* render_text_pad()                                                   */
/* ------------------------------------------------------------------ */

void render_text_pad(VDPPlane plane, const char *str,
                     u16 x, u16 y, u16 width, u16 pal_idx) {
    u16 cx = x;
    const char *p = str;
    u8 c;
    u16 end = x + width;
    if (end > SCREEN_COLS) end = SCREEN_COLS;

    /* Escreve a string                                                 */
    while (*p != '\0' && cx < end) {
        c = (u8)*p++;
        if (c < 32u || c > 127u) { cx++; continue; }
        write_tile(plane, CHAR_TO_TILE(c), pal_idx, 0, cx, y);
        cx++;
    }
    /* Preenche o restante com espa?o (tile do char 32)                */
    while (cx < end) {
        write_tile(plane, CHAR_TO_TILE(32u), pal_idx, 0, cx, y);
        cx++;
    }
}

/* ------------------------------------------------------------------ */
/* render_number()                                                     */
/* ------------------------------------------------------------------ */

void render_number(VDPPlane plane, long value,
                   u16 x, u16 y, u16 width, u16 pal_idx) {
    /*
     * Formata n?mero com separadores de milhar (ponto), alinhado ?
     * direita em 'width' colunas. N?meros negativos mostram '-'.
     * Ex: 1234567 -> "1.234.567"
     *
     * Estrat?gia: formata de tr?s para frente num buffer local,
     * depois renderiza centralizado/alinhado.
     */
    char buf[16];
    u8 pos = 15;
    u8 digit_count = 0;
    u8 negative = 0;
    long v = value;

    buf[pos] = '\0';

    if (v < 0L) {
        negative = 1;
        v = -v;
    }
    if (v == 0L) {
        buf[--pos] = '0';
    } else {
        while (v > 0L && pos > 0u) {
            if (digit_count > 0u && (digit_count % 3u) == 0u) {
                buf[--pos] = '.';
            }
            buf[--pos] = (char)('0' + (u8)(v % 10L));
            v /= 10L;
            digit_count++;
        }
    }
    if (negative && pos > 0u) {
        buf[--pos] = '-';
    }

    /* Calcula comprimento da string formatada                          */
    const char *num_str = &buf[pos];
    u16 slen = 0;
    const char *p = num_str;
    while (*p++) slen++;

    /* Alinha ? direita dentro de 'width'                              */
    u16 pad = (slen < width) ? (u16)(width - slen) : 0u;
    render_clear_rect(plane, x, y, pad, 1u);
    render_text(plane, num_str, (u16)(x + pad), y, pal_idx);
}

/* ------------------------------------------------------------------ */
/* render_clear_rect()                                                 */
/* ------------------------------------------------------------------ */

void render_clear_rect(VDPPlane plane, u16 x, u16 y, u16 w, u16 h) {
    /* Fill with SPACE tile (FONT_BASE_TILE) instead of tile 0.
     * Tile 0 contains SGDK system data and would show as visual noise.
     * FONT_BASE_TILE (16) is our blank space glyph -- looks transparent. */
    VDP_fillTileMapRect(plane, ELF_TILE(FONT_BASE_TILE, PAL0, 0), x, y, w, h);
}

/* ------------------------------------------------------------------ */
/* render_fill_rect()                                                  */
/* ------------------------------------------------------------------ */

void render_fill_rect(VDPPlane plane, u16 x, u16 y, u16 w, u16 h,
                      u16 pal_idx, u16 tile_idx) {
    /* VDP_fillTileMapRect() existe no SGDK 1.70 -- usa DMA interno.   */
    VDP_fillTileMapRect(plane, ELF_TILE(tile_idx, pal_idx, 0), x, y, w, h);
}

/* ------------------------------------------------------------------ */
/* render_box()                                                        */
/* ------------------------------------------------------------------ */

void render_box(VDPPlane plane, u16 x, u16 y, u16 w, u16 h,
                u16 style, u16 pal_idx) {
    /*
     * Box-drawing tile layout a partir de BOX_BASE_TILE:
     *   Simple (+0..+5):  ? ? ? ? ? ?
     *   Double (+6..+11): ? ? ? ? ? ?
     *   Crossings (+12):  ?
     */
    u16 base = (style == BOX_DOUBLE) ? (BOX_BASE_TILE + 6u) : BOX_BASE_TILE;
    /* Offsets dentro do bloco de estilo:
     *   0=?/?  1=?/?  2=?/?  3=?/?  4=?/?  5=?/?  */
    u16 cx, cy;
    u16 x2 = x + w - 1u;
    u16 y2 = y + h - 1u;

    /* Cantos                                                           */
    write_tile(plane, base + 2u, pal_idx, 0, x,  y);   /* ?/? */
    write_tile(plane, base + 3u, pal_idx, 0, x2, y);   /* ?/? */
    write_tile(plane, base + 4u, pal_idx, 0, x,  y2);  /* ?/? */
    write_tile(plane, base + 5u, pal_idx, 0, x2, y2);  /* ?/? */

    /* Linhas horizontais (superior e inferior)                        */
    for (cx = x + 1u; cx < x2; cx++) {
        write_tile(plane, base + 0u, pal_idx, 0, cx, y);   /* ?/? */
        write_tile(plane, base + 0u, pal_idx, 0, cx, y2);  /* ?/? */
    }

    /* Linhas verticais (esquerda e direita)                           */
    for (cy = y + 1u; cy < y2; cy++) {
        write_tile(plane, base + 1u, pal_idx, 0, x,  cy);  /* ?/? */
        write_tile(plane, base + 1u, pal_idx, 0, x2, cy);  /* ?/? */
    }
}

/* ------------------------------------------------------------------ */
/* render_hline() / render_vline()                                     */
/* ------------------------------------------------------------------ */

void render_hline(VDPPlane plane, u16 x, u16 y, u16 len,
                  u16 style, u16 pal_idx) {
    u16 base = (style == BOX_DOUBLE) ? (BOX_BASE_TILE + 6u) : BOX_BASE_TILE;
    u16 cx;
    for (cx = x; cx < x + len; cx++) {
        write_tile(plane, base + 0u, pal_idx, 0, cx, y);  /* ?/? */
    }
}

void render_vline(VDPPlane plane, u16 x, u16 y, u16 len,
                  u16 style, u16 pal_idx) {
    u16 base = (style == BOX_DOUBLE) ? (BOX_BASE_TILE + 6u) : BOX_BASE_TILE;
    u16 cy;
    for (cy = y; cy < y + len; cy++) {
        write_tile(plane, base + 1u, pal_idx, 0, x, cy);  /* ?/? */
    }
}

/* ------------------------------------------------------------------ */
/* render_set_bg_color()                                               */
/* ------------------------------------------------------------------ */

void render_set_bg_color(u16 bg_pal0_entry) {
    /*
     * Preenche BG_B com tile 0 usando a paleta PAL0.
     * A cor de fundo vem de PAL0[bg_pal0_entry].
     * Como o tile 0 ? totalmente transparente/vazio, todos os seus
     * pixels usam a "cor 0" da paleta selecionada para o tile.
     *
     * Mas espera -- para ter uma cor s?lida no BG_B, precisamos de um
     * tile cujos pixels sejam da cor desejada, ou usar a cor de borda
     * do VDP (VDP_setBackgroundColor). Usamos VDP_setBackgroundColor
     * para a cor de fundo real do VDP (cor "exterior"), e preenchemos
     * BG_B com SOLID_TILE_IDX para garantir cobertura total.
     *
     * Na pr?tica: a cor de fundo do VDP (reg 0x07) ? o ?ndice global
     * na paleta combinada (0-63). PAL0 ocupa ?ndices 0-15.
     */
    /* VDP_setBackgroundColor sets the border/backdrop colour index.
     * bg_pal0_entry is a PAL0 colour index (0-15) for the backdrop.   */
    VDP_setBackgroundColor((u8)bg_pal0_entry);

    /* Fill BG_B with our SPACE tile (FONT_BASE_TILE = tile 16, ASCII 32).
     * Tile 16 is the first user tile -- a blank 8x8 glyph from our font.
     * We use PAL0 so colour[0] of PAL0 is the background pixel colour.
     * This avoids tile 0 which contains SGDK system/SEGA logo pixel data
     * and would produce the red/blue stripe artefact.                  */
    render_fill_rect(BG_B, 0u, 0u, SCREEN_COLS, SCREEN_ROWS,
                     PAL0, (u16)FONT_BASE_TILE);
}

/* ------------------------------------------------------------------ */
/* render_status_bar()                                                 */
/* ------------------------------------------------------------------ */

void render_status_bar(void) {
    /*
     * Desenha as barras fixas no WINDOW:
     *   Linha 0: borda superior (? repetida)
     *   Linha 1: " ELIFOOT II  Jornada:XX  DivX  $XXXXXXX "
     *   Linha 2: borda separadora (? repetida)
     *
     * Usa PAL1 (ciano escuro como bg, texto preto/amarelo).
     */

    /* Linha 0: borda dupla superior (? repetida)                      */
    render_fill_rect(BG_A, 0u, 0u, SCREEN_COLS, 1u, PAL_SELECTED, 0u);
    render_hline(BG_A, 0u, 0u, SCREEN_COLS, BOX_DOUBLE, PAL_SELECTED);

    /* Linha 1: conte?do da status bar                                 */
    render_fill_rect(BG_A, 0u, STATUS_ROW, SCREEN_COLS, 1u,
                     PAL_SELECTED, 0u);

    /* Texto principal da status bar (preto sobre ciano -- PAL1[1])    */
    render_text(BG_A, "ELIFOOT II", 1u, STATUS_ROW, PAL_SELECTED);

    /* Jornada: amarelo sobre ciano (PAL1[3])                          */
    render_textf(BG_A, 14u, STATUS_ROW, PAL_SELECTED,
                 "Jornada:%02u", (u16)g_round);

    /* Divis?o                                                          */
    render_textf(BG_A, 25u, STATUS_ROW, PAL_SELECTED,
                 "Div%u", (u16)(g_division + 1u));

    /* Dinheiro: amarelo sobre ciano -- usamos PAL1 aqui porque o texto
     * j? aparece sobre o fundo ciano e PAL1[3]=amarelo d? destaque.  */
    render_textf(BG_A, 30u, STATUS_ROW, PAL_SELECTED,
                 "$%ld", g_money);

    /* Linha 2: borda separadora (?)                                   */
    render_fill_rect(BG_A, 0u, 2u, SCREEN_COLS, 1u, PAL_SELECTED, 0u);
    render_hline(BG_A, 0u, 2u, SCREEN_COLS, BOX_DOUBLE, PAL_SELECTED);

    /* Linha 25: borda separadora antes do help bar                    */
    render_fill_rect(BG_A, 0u, 25u, SCREEN_COLS, 1u, PAL_SELECTED, 0u);
    render_hline(BG_A, 0u, 25u, SCREEN_COLS, BOX_DOUBLE, PAL_SELECTED);

    /* Linha 27: borda inferior                                        */
    render_fill_rect(BG_A, 0u, 27u, SCREEN_COLS, 1u, PAL_SELECTED, 0u);
    render_hline(BG_A, 0u, 27u, SCREEN_COLS, BOX_DOUBLE, PAL_SELECTED);
}

/* ------------------------------------------------------------------ */
/* render_help_bar()                                                   */
/* ------------------------------------------------------------------ */

void render_help_bar(const char *line1, const char *line2) {
    /* Linha 26: help bar com hints de controle                        */
    render_fill_rect(BG_A, 0u, HELP_ROW, SCREEN_COLS, 1u,
                     PAL_SELECTED, 0u);
    if (line1 != (const char *)0) {
        render_text_pad(BG_A, line1, 1u, HELP_ROW,
                        (u16)(SCREEN_COLS - 2u), PAL_SELECTED);
    }
    if (line2 != (const char *)0) {
        render_fill_rect(BG_A, 0u, 27u, SCREEN_COLS, 1u,
                         PAL_SELECTED, 0u);
        /* linha 27 j? ? borda -- exibimos em linha 26 apenas, linha 27
         * permanece como borda inferior desenhada por render_status_bar */
        (void)line2;  /* reservado para uso futuro com 2 linhas de help */
    }
}

/* ------------------------------------------------------------------ */
/* render_clear_content()                                              */
/* ------------------------------------------------------------------ */

void render_clear_content(void) {
    /*
     * CR?TICO: VDP_clearTileMapRect(WINDOW, 0, 0, 40, 28) ? obrigat?rio ao trocar
     * de tela -- res?duos de texto no WINDOW persistem se n?o limpar.
     * Aqui limpamos apenas a ?rea de conte?do para preservar a
     * status bar j? renderizada.
     *
     * Estrat?gia: limpar BG_A (conte?do principal) inteiro, e depois
     * redesenhar a status bar no WINDOW para garantir estado limpo.
     */
    VDP_fillTileMapRect(BG_A, ELF_TILE(FONT_BASE_TILE, PAL0, 0), 0, 0, 40, 28);
    VDP_clearTileMapRect(WINDOW, 0, 0, 40, 28);

    /* Redesenha barras fixas ap?s limpar o WINDOW.                    */
    render_status_bar();
}
