# res/resources.res - Recursos da ROM do Elifoot II Genesis
#
# Processado pelo rescomp 3.39b (SGDK 1.70).
# Sintaxe: rescomp resource.res [out.s] [-noheader]
#
# PRE-REQUISITOS antes de rodar o make:
#   1. python3 tools/gen_font.py gfx/font_cp850.png
#   2. python3 tools/pack_data.py original/EQUIPAS.EF2 \
#                                  original/TREINAD.EF2 \
#                                  data/
#
# gfx/font_cp850.png deve ser:
#   PNG 8x872 pixels, indexado em 2 cores (0=background, 1=foreground).
#   109 glifos de 8x8px: ASCII 32-127 (96) + box-drawing CP437 (13).
#   Gerado por: python3 tools/gen_font.py gfx/font_cp850.png
#
# ---------------------------------------------------------------------
# Fonte de texto (TILESET)
# ---------------------------------------------------------------------
# Carregada em VRAM a partir do tile FONT_BASE_TILE (1) via
# VDP_loadTileSet(&font_tiles, FONT_BASE_TILE, DMA) em render_init().

TILESET font_tiles "gfx/font_cp850.png" NONE NONE

# ---------------------------------------------------------------------
# Dados binarios (BIN)
# ---------------------------------------------------------------------
# teams.bin   : 4 + 29 x (26 + 16x24) = 11894 bytes
# coaches.bin : 2 + 50 x 20            = 1002 bytes
#
# BIN name file align salign fill compression
#   align   = 2  (word-aligned para acesso eficiente no m68k)
#   salign  = 2  (tamanho alinhado em word)
#   fill    = 0  (byte de preenchimento)
#   FAST    = compressao LZ4W
#   far     = TRUE por defeito para BIN (dados no fim da ROM)

BIN teams_data   "data/teams.bin"   2 2 0 FAST
BIN coaches_data "data/coaches.bin" 2 2 0 FAST

# ---------------------------------------------------------------------
# NOTA: Paletas NAO sao definidas aqui via PALETTE resource.
# As paletas PAL0 e PAL1 sao arrays estaticos const u16[] em render.c
# com os valores exatos RGB333 para maxima fidelidade as cores CGA.
# PAL2 e PAL3 sao inicializadas em runtime como copias de PAL0.
# ---------------------------------------------------------------------
