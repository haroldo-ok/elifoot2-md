# res/resources.res — Recursos da ROM do Elifoot II Genesis
#
# Processado pelo rescomp 3.39b (SGDK 1.70).
# Sintaxe: rescomp resource.res [out.s] [-noheader]
#
# ATENÇÃO — rescomp gera res/resources.h automaticamente com as
# declarações extern para cada recurso. Os nomes usados aqui (ex:
# "font_tiles", "teams_data") devem corresponder EXATAMENTE ao que
# o código C referencia. Uma discrepância gera "undefined reference"
# no link — não no compile, o que pode confundir.
#
# Pré-requisitos antes de rodar o make:
#   1. python3 tools/pack_data.py original/EQUIPAS.EF2 \
#                                  original/TREINAD.EF2 \
#                                  data/
#      → gera data/teams.bin e data/coaches.bin
#
#   2. gfx/font_cp850.png deve existir:
#      PNG de 8×768 pixels (8px × 8px × 96 glifos), indexado em 2 cores:
#        índice 0 = transparente/background
#        índice 1 = foreground (pixel do glifo)
#      Glifos: chars ASCII 32 (space) até 127 (DEL/~) em ordem sequencial.
#      Após os 96 glifos ASCII, 13 glifos de box-drawing CP437:
#        ─ │ ┌ ┐ └ ┘ ═ ║ ╔ ╗ ╚ ╝ ┼
#      Total: 109 glifos × 8px altura = 872px.
#      Ajustar aqui se a altura real for diferente.
#
# ---------------------------------------------------------------------
# Fonte de texto (TILESET)
# ---------------------------------------------------------------------
# TILESET name  img_file              compression  opt
#   compression NONE = sem compressão (fonte é pequena, não vale)
#   opt         NONE = sem deduplicação (cada glifo é único por definição)
#
# A fonte é carregada em VRAM a partir de FONT_BASE_TILE (tile 1)
# via VDP_loadTileSet(&font_tiles, FONT_BASE_TILE, DMA) em render_init().
# O rescomp apenas empacota o PNG em formato TileSet do SGDK —
# o posicionamento em VRAM é responsabilidade do código.

TILESET font_tiles "gfx/font_cp850.png" NONE NONE

# ---------------------------------------------------------------------
# Dados binários (BIN)
# ---------------------------------------------------------------------
# BIN name  file                align  salign  fill  compression  far
#   align   = 2 (word-aligned — obrigatório para acesso eficiente no m68k)
#   salign  = 2 (tamanho alinhado em word)
#   fill    = 0 (byte de preenchimento para alinhamento de tamanho)
#   FAST    = compressão LZ4W (boa taxa, rápida para descompactar em Z80)
#   far     = default TRUE para BIN — dados no final da ROM
#
# teams.bin: 4 + 29 × (26 + 16 × 24) = 11.898 bytes
# coaches.bin: 2 + 50 × 20 = 1.002 bytes

BIN teams_data   "data/teams.bin"   2 2 0 FAST
BIN coaches_data "data/coaches.bin" 2 2 0 FAST

# ---------------------------------------------------------------------
# NOTA: Paletas NÃO são definidas aqui via PALETTE resource.
# ---------------------------------------------------------------------
# As paletas PAL0 e PAL1 são arrays estáticos const u16[] em render.c,
# com os valores exatos calculados para máxima fidelidade às cores CGA.
# Isso dá controle preciso sobre os valores RGB333 sem depender de
# como o rescomp converte pixels de PNG para valores Genesis.
#
# PAL2 e PAL3 são inicializadas em runtime como cópias de PAL0 e podem
# ser customizadas por equipe durante o jogo.
#
# Se no futuro quiser usar PNGs para as paletas, descomentar:
# PALETTE pal_main      "gfx/palette_main.png"
# PALETTE pal_selection "gfx/palette_selection.png"
