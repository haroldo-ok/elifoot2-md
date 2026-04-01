#!/usr/bin/env python3
"""
tools/gen_font.py — Gerador da fonte bitmap para Elifoot II Genesis
====================================================================
Gera gfx/font_cp850.png: fonte 8×8px para os 96 chars ASCII (32–127)
mais 13 tiles de box-drawing CP437, total 109 glifos × 8px = 872px de altura.

Formato de saída:
  - PNG 8×872 pixels
  - Indexado em 2 cores:
      índice 0 = preto      (background / transparente)
      índice 1 = branco     (foreground / pixel do glifo)
  - A cor real em jogo é sempre determinada pela paleta VDP Genesis,
    não pelas cores do PNG — o PNG serve apenas como mapa de bits.

Uso:
    python3 tools/gen_font.py [output_path]
    python3 tools/gen_font.py gfx/font_cp850.png   (default)

Dependências:
    Pillow (pip install pillow)

Como funciona:
    Usa a fonte "font_data" embutida neste script — uma fonte 8×8
    derivada da IBM PC Character Generator ROM (domínio público).
    Cada glifo é definido como 8 bytes, um por linha, bit 7 = coluna 0.

    Os 13 tiles de box-drawing são gerados geometricamente (desenhados
    pixel a pixel com lógica matemática simples) para garantir alinhamento
    perfeito na grelha de 8×8.
"""

import os
import struct
import sys
import zlib


# ---------------------------------------------------------------------------
# Constantes
# ---------------------------------------------------------------------------

TILE_W = 8       # largura de cada glifo em pixels
TILE_H = 8       # altura de cada glifo em pixels
FIRST_ASCII = 32  # primeiro char mapeado (space)
LAST_ASCII  = 127 # último char mapeado (DEL → usa glifo de bloco)
ASCII_COUNT = LAST_ASCII - FIRST_ASCII + 1  # 96 glifos

# Tiles de box-drawing — ordem correspondente ao que render.c espera:
# BOX_BASE_TILE = 97, offsets:
#   Simple: +0=─  +1=│  +2=┌  +3=┐  +4=└  +5=┘
#   Double: +6=═  +7=║  +8=╔  +9=╗  +10=╚ +11=╝  +12=┼
BOX_COUNT = 13

TOTAL_GLYPHS = ASCII_COUNT + BOX_COUNT   # 109
IMG_WIDTH    = TILE_W                     # 8
IMG_HEIGHT   = TOTAL_GLYPHS * TILE_H     # 872


# ---------------------------------------------------------------------------
# Fonte 8×8 embutida (IBM PC ROM Character Set, domínio público)
# Apenas os chars 32–127. Cada entrada = 8 bytes (1 byte por linha, bit7=col0).
# ---------------------------------------------------------------------------

# fmt: off
_FONT_RAW = (
    # 32 space
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    # 33 !
    0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00,
    # 34 "
    0x36,0x36,0x00,0x00,0x00,0x00,0x00,0x00,
    # 35 #
    0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00,
    # 36 $
    0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00,
    # 37 %
    0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00,
    # 38 &
    0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00,
    # 39 '
    0x06,0x06,0x03,0x00,0x00,0x00,0x00,0x00,
    # 40 (
    0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0x00,
    # 41 )
    0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0x00,
    # 42 *
    0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00,
    # 43 +
    0x00,0x0C,0x0C,0x3F,0x0C,0x0C,0x00,0x00,
    # 44 ,
    0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x06,
    # 45 -
    0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00,
    # 46 .
    0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00,
    # 47 /
    0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00,
    # 48 0
    0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00,
    # 49 1
    0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0x00,
    # 50 2
    0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0x00,
    # 51 3
    0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0x00,
    # 52 4
    0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0x00,
    # 53 5
    0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0x00,
    # 54 6
    0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0x00,
    # 55 7
    0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0x00,
    # 56 8
    0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00,
    # 57 9
    0x1E,0x33,0x33,0x3E,0x30,0x18,0x0E,0x00,
    # 58 :
    0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x00,
    # 59 ;
    0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x06,
    # 60 <
    0x18,0x0C,0x06,0x03,0x06,0x0C,0x18,0x00,
    # 61 =
    0x00,0x00,0x3F,0x00,0x00,0x3F,0x00,0x00,
    # 62 >
    0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00,
    # 63 ?
    0x1E,0x33,0x30,0x18,0x0C,0x00,0x0C,0x00,
    # 64 @
    0x3E,0x63,0x7B,0x7B,0x7B,0x03,0x1E,0x00,
    # 65 A
    0x0C,0x1E,0x33,0x33,0x3F,0x33,0x33,0x00,
    # 66 B
    0x3F,0x66,0x66,0x3E,0x66,0x66,0x3F,0x00,
    # 67 C
    0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0x00,
    # 68 D
    0x1F,0x36,0x66,0x66,0x66,0x36,0x1F,0x00,
    # 69 E
    0x7F,0x46,0x16,0x1E,0x16,0x46,0x7F,0x00,
    # 70 F
    0x7F,0x46,0x16,0x1E,0x16,0x06,0x0F,0x00,
    # 71 G
    0x3C,0x66,0x03,0x03,0x73,0x66,0x7C,0x00,
    # 72 H
    0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0x00,
    # 73 I
    0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00,
    # 74 J
    0x78,0x30,0x30,0x30,0x33,0x33,0x1E,0x00,
    # 75 K
    0x67,0x66,0x36,0x1E,0x36,0x66,0x67,0x00,
    # 76 L
    0x0F,0x06,0x06,0x06,0x46,0x66,0x7F,0x00,
    # 77 M
    0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0x00,
    # 78 N
    0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0x00,
    # 79 O
    0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00,
    # 80 P
    0x3F,0x66,0x66,0x3E,0x06,0x06,0x0F,0x00,
    # 81 Q
    0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0x00,
    # 82 R
    0x3F,0x66,0x66,0x3E,0x36,0x66,0x67,0x00,
    # 83 S
    0x1E,0x33,0x07,0x0E,0x38,0x33,0x1E,0x00,
    # 84 T
    0x3F,0x2D,0x0C,0x0C,0x0C,0x0C,0x1E,0x00,
    # 85 U
    0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0x00,
    # 86 V
    0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0x00,
    # 87 W
    0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00,
    # 88 X
    0x63,0x63,0x36,0x1C,0x1C,0x36,0x63,0x00,
    # 89 Y
    0x33,0x33,0x33,0x1E,0x0C,0x0C,0x1E,0x00,
    # 90 Z
    0x7F,0x63,0x31,0x18,0x4C,0x66,0x7F,0x00,
    # 91 [
    0x1E,0x06,0x06,0x06,0x06,0x06,0x1E,0x00,
    # 92 backslash
    0x03,0x06,0x0C,0x18,0x30,0x60,0x40,0x00,
    # 93 ]
    0x1E,0x18,0x18,0x18,0x18,0x18,0x1E,0x00,
    # 94 ^
    0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00,
    # 95 _
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,
    # 96 `
    0x0C,0x0C,0x18,0x00,0x00,0x00,0x00,0x00,
    # 97 a
    0x00,0x00,0x1E,0x30,0x3E,0x33,0x6E,0x00,
    # 98 b
    0x07,0x06,0x06,0x3E,0x66,0x66,0x3B,0x00,
    # 99 c
    0x00,0x00,0x1E,0x33,0x03,0x33,0x1E,0x00,
    # 100 d
    0x38,0x30,0x30,0x3e,0x33,0x33,0x6E,0x00,
    # 101 e
    0x00,0x00,0x1E,0x33,0x3f,0x03,0x1E,0x00,
    # 102 f
    0x1C,0x36,0x06,0x0f,0x06,0x06,0x0F,0x00,
    # 103 g
    0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x1F,
    # 104 h
    0x07,0x06,0x36,0x6E,0x66,0x66,0x67,0x00,
    # 105 i
    0x0C,0x00,0x0E,0x0C,0x0C,0x0C,0x1E,0x00,
    # 106 j
    0x30,0x00,0x30,0x30,0x30,0x33,0x33,0x1E,
    # 107 k
    0x07,0x06,0x66,0x36,0x1E,0x36,0x67,0x00,
    # 108 l
    0x0E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00,
    # 109 m
    0x00,0x00,0x33,0x7F,0x7F,0x6B,0x63,0x00,
    # 110 n
    0x00,0x00,0x1F,0x33,0x33,0x33,0x33,0x00,
    # 111 o
    0x00,0x00,0x1E,0x33,0x33,0x33,0x1E,0x00,
    # 112 p
    0x00,0x00,0x3B,0x66,0x66,0x3E,0x06,0x0F,
    # 113 q
    0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x78,
    # 114 r
    0x00,0x00,0x3B,0x6E,0x66,0x06,0x0F,0x00,
    # 115 s
    0x00,0x00,0x3E,0x03,0x1E,0x30,0x1F,0x00,
    # 116 t
    0x08,0x0C,0x3E,0x0C,0x0C,0x2C,0x18,0x00,
    # 117 u
    0x00,0x00,0x33,0x33,0x33,0x33,0x6E,0x00,
    # 118 v
    0x00,0x00,0x33,0x33,0x33,0x1E,0x0C,0x00,
    # 119 w
    0x00,0x00,0x63,0x6B,0x7F,0x7F,0x36,0x00,
    # 120 x
    0x00,0x00,0x63,0x36,0x1C,0x36,0x63,0x00,
    # 121 y
    0x00,0x00,0x33,0x33,0x33,0x3E,0x30,0x1F,
    # 122 z
    0x00,0x00,0x3F,0x19,0x0C,0x26,0x3F,0x00,
    # 123 {
    0x38,0x0C,0x0C,0x07,0x0C,0x0C,0x38,0x00,
    # 124 |
    0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00,
    # 125 }
    0x07,0x0C,0x0C,0x38,0x0C,0x0C,0x07,0x00,
    # 126 ~
    0x6E,0x3B,0x00,0x00,0x00,0x00,0x00,0x00,
    # 127 DEL → bloco sólido (usado como tile de debug)
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
)
# fmt: on

assert len(_FONT_RAW) == ASCII_COUNT * TILE_H, \
    f"Font data size mismatch: {len(_FONT_RAW)} vs {ASCII_COUNT * TILE_H}"


# ---------------------------------------------------------------------------
# Box-drawing tiles (gerados geometricamente)
# ---------------------------------------------------------------------------

def _make_box_tiles() -> list[list[int]]:
    """
    Gera os 13 tiles de box-drawing como listas de 8 bytes.
    Centro do tile: pixel (3,3) e (4,4) numa grelha 0–7.
    Linhas passam pelo centro: horizontal em y=3 e y=4 (dupla),
    vertical em x=3 e x=4 (dupla). Single usa apenas y=3 / x=3.
    """

    def blank() -> list[int]:
        return [0] * TILE_H

    def set_px(tile: list[int], x: int, y: int) -> None:
        if 0 <= x < 8 and 0 <= y < 8:
            tile[y] |= (1 << (7 - x))

    # ── linha horizontal simples (y=3)
    def hline_s(tile):
        for x in range(8): set_px(tile, x, 3)

    # ══ linha horizontal dupla (y=2 e y=5)
    def hline_d(tile):
        for x in range(8):
            set_px(tile, x, 2)
            set_px(tile, x, 5)

    # │ linha vertical simples (x=3)
    def vline_s(tile):
        for y in range(8): set_px(tile, 3, y)

    # ║ linha vertical dupla (x=2 e x=5)
    def vline_d(tile):
        for y in range(8):
            set_px(tile, 2, y)
            set_px(tile, 5, y)

    tiles = []

    # +0  ─  horizontal simples
    t = blank(); hline_s(t); tiles.append(t)

    # +1  │  vertical simples
    t = blank(); vline_s(t); tiles.append(t)

    # +2  ┌  canto superior esquerdo simples (direita + baixo)
    t = blank()
    for x in range(3, 8): set_px(t, x, 3)   # horizontal →
    for y in range(3, 8): set_px(t, 3, y)   # vertical ↓
    tiles.append(t)

    # +3  ┐  canto superior direito simples (esquerda + baixo)
    t = blank()
    for x in range(0, 4): set_px(t, x, 3)   # horizontal ←
    for y in range(3, 8): set_px(t, 3, y)   # vertical ↓
    tiles.append(t)

    # +4  └  canto inferior esquerdo simples (direita + cima)
    t = blank()
    for x in range(3, 8): set_px(t, x, 3)   # horizontal →
    for y in range(0, 4): set_px(t, 3, y)   # vertical ↑
    tiles.append(t)

    # +5  ┘  canto inferior direito simples (esquerda + cima)
    t = blank()
    for x in range(0, 4): set_px(t, x, 3)   # horizontal ←
    for y in range(0, 4): set_px(t, 3, y)   # vertical ↑
    tiles.append(t)

    # +6  ═  horizontal dupla
    t = blank(); hline_d(t); tiles.append(t)

    # +7  ║  vertical dupla
    t = blank(); vline_d(t); tiles.append(t)

    # +8  ╔  canto duplo superior esquerdo
    t = blank()
    # linha superior horizontal (y=2): da col 2 até 7
    for x in range(2, 8): set_px(t, x, 2)
    # linha inferior horizontal (y=5): da col 5 até 7
    for x in range(5, 8): set_px(t, x, 5)
    # linha esquerda vertical (x=2): da linha 2 até 7
    for y in range(2, 8): set_px(t, 2, y)
    # linha direita vertical (x=5): da linha 5 até 7
    for y in range(5, 8): set_px(t, 5, y)
    tiles.append(t)

    # +9  ╗  canto duplo superior direito
    t = blank()
    for x in range(0, 6): set_px(t, x, 2)   # horizontal superior ←
    for x in range(0, 3): set_px(t, x, 5)   # horizontal inferior ←
    for y in range(2, 8): set_px(t, 5, y)   # vertical direita ↓
    for y in range(5, 8): set_px(t, 2, y)   # vertical esquerda ↓
    tiles.append(t)

    # +10 ╚  canto duplo inferior esquerdo
    t = blank()
    for x in range(2, 8): set_px(t, x, 5)   # horizontal inferior →
    for x in range(5, 8): set_px(t, x, 2)   # horizontal superior →
    for y in range(0, 6): set_px(t, 2, y)   # vertical esquerda ↑
    for y in range(0, 3): set_px(t, 5, y)   # vertical direita ↑
    tiles.append(t)

    # +11 ╝  canto duplo inferior direito
    t = blank()
    for x in range(0, 6): set_px(t, x, 5)   # horizontal inferior ←
    for x in range(0, 3): set_px(t, x, 2)   # horizontal superior ←
    for y in range(0, 6): set_px(t, 5, y)   # vertical direita ↑
    for y in range(0, 3): set_px(t, 2, y)   # vertical esquerda ↑
    tiles.append(t)

    # +12 ┼  cruzamento simples
    t = blank(); hline_s(t); vline_s(t); tiles.append(t)

    assert len(tiles) == BOX_COUNT, f"Expected {BOX_COUNT} box tiles, got {len(tiles)}"
    return tiles


# ---------------------------------------------------------------------------
# Construção do bitmap (array 2D de índices de paleta)
# ---------------------------------------------------------------------------

def _build_bitmap() -> list[list[int]]:
    """
    Constrói a imagem como lista de linhas, cada linha = lista de índices (0 ou 1).
    Dimensões: IMG_HEIGHT × IMG_WIDTH.
    """
    pixels = [[0] * IMG_WIDTH for _ in range(IMG_HEIGHT)]

    # ASCII 32–127 (96 glifos)
    for glyph_idx in range(ASCII_COUNT):
        base_y = glyph_idx * TILE_H
        for row in range(TILE_H):
            byte = _FONT_RAW[glyph_idx * TILE_H + row]
            for col in range(TILE_W):
                bit = (byte >> (7 - col)) & 1
                pixels[base_y + row][col] = bit

    # Box-drawing (13 glifos)
    box_tiles = _make_box_tiles()
    box_start_y = ASCII_COUNT * TILE_H
    for glyph_idx, tile_bytes in enumerate(box_tiles):
        base_y = box_start_y + glyph_idx * TILE_H
        for row in range(TILE_H):
            byte = tile_bytes[row]
            for col in range(TILE_W):
                bit = (byte >> (7 - col)) & 1
                pixels[base_y + row][col] = bit

    return pixels


# ---------------------------------------------------------------------------
# Escrita do PNG (implementação manual — sem dependência de Pillow)
# ---------------------------------------------------------------------------

def _write_png_indexed(path: str, pixels: list[list[int]]) -> None:
    """
    Escreve um PNG indexado em 2 cores sem Pillow.
    Formato: 8 bits por pixel indexado (type 3), paleta de 2 entradas.
    1 bit/pixel seria mais compacto mas 8bpp é mais simples e suficiente.

    Estrutura PNG:
      Signature (8B) + IHDR + PLTE + tRNS + IDAT + IEND
    """
    w = len(pixels[0])
    h = len(pixels)

    def chunk(name: bytes, data: bytes) -> bytes:
        c = name + data
        return struct.pack('>I', len(data)) + c + struct.pack('>I', zlib.crc32(c) & 0xFFFFFFFF)

    # IHDR: width, height, bit_depth=8, color_type=3(indexed), compress=0, filter=0, interlace=0
    ihdr = struct.pack('>IIBBBBB', w, h, 8, 3, 0, 0, 0)

    # PLTE: 2 cores — índice 0=preto, índice 1=branco
    plte = bytes([0, 0, 0,    # cor 0: preto
                  255, 255, 255])  # cor 1: branco

    # tRNS: transparência — índice 0 é transparente (alpha=0), índice 1 opaco
    # Necessário para que o VDP do Genesis use o índice 0 como "cor transparente"
    trns = bytes([0, 255])

    # IDAT: dados de imagem comprimidos com zlib
    raw_rows = bytearray()
    for row in pixels:
        raw_rows.append(0)      # filter byte = None (0)
        raw_rows.extend(row)    # pixels da linha (1 byte por pixel, valor 0 ou 1)

    idat_data = zlib.compress(bytes(raw_rows), level=9)

    # IEND
    iend = b''

    png = (
        b'\x89PNG\r\n\x1a\n'   # PNG signature
        + chunk(b'IHDR', ihdr)
        + chunk(b'PLTE', plte)
        + chunk(b'tRNS', trns)
        + chunk(b'IDAT', idat_data)
        + chunk(b'IEND', iend)
    )

    with open(path, 'wb') as f:
        f.write(png)


def _write_png_pillow(path: str, pixels: list[list[int]]) -> None:
    """Versão com Pillow (melhor qualidade de compressão)."""
    from PIL import Image
    img = Image.new('P', (IMG_WIDTH, IMG_HEIGHT))
    palette = [0, 0, 0,       # índice 0: preto
               255, 255, 255]  # índice 1: branco
    palette += [0] * (256 * 3 - len(palette))
    img.putpalette(palette)
    flat = [p for row in pixels for p in row]
    img.putdata(flat)
    img.save(path, optimize=True)


# ---------------------------------------------------------------------------
# Verificação visual (ASCII art no terminal)
# ---------------------------------------------------------------------------

def _preview_glyph(glyph_idx: int, pixels: list[list[int]]) -> None:
    base_y = glyph_idx * TILE_H
    for row in range(TILE_H):
        line = ''.join('█' if pixels[base_y + row][col] else '·'
                       for col in range(TILE_W))
        print(f'  {line}')


def _preview_all_box(pixels: list[list[int]]) -> None:
    names = ['─','│','┌','┐','└','┘','═','║','╔','╗','╚','╝','┼']
    box_start = ASCII_COUNT
    print('\nBox-drawing tiles:')
    for i, name in enumerate(names):
        print(f'  +{i} {name}:')
        _preview_glyph(box_start + i, pixels)
        print()


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> int:
    output_path = sys.argv[1] if len(sys.argv) > 1 else 'gfx/font_cp850.png'

    os.makedirs(os.path.dirname(output_path) or '.', exist_ok=True)

    print(f'Gerando fonte: {TOTAL_GLYPHS} glifos × {TILE_W}×{TILE_H}px')
    print(f'  ASCII 32–127: {ASCII_COUNT} glifos')
    print(f'  Box-drawing:  {BOX_COUNT} glifos')
    print(f'  Dimensões PNG: {IMG_WIDTH}×{IMG_HEIGHT}px')

    pixels = _build_bitmap()

    # Tenta Pillow primeiro, cai para implementação manual
    try:
        _write_png_pillow(output_path, pixels)
        print(f'Escrito com Pillow: {output_path}')
    except ImportError:
        _write_png_indexed(output_path, pixels)
        print(f'Escrito sem Pillow (PNG manual): {output_path}')

    # Preview no terminal dos tiles de box-drawing
    if '--preview' in sys.argv or '-p' in sys.argv:
        _preview_all_box(pixels)

        print('Amostra de letras:')
        for ch in 'AaBb0189!@':
            idx = ord(ch) - FIRST_ASCII
            print(f'  {ch!r} (idx {idx}):')
            _preview_glyph(idx, pixels)

    print(f'\nOK — {output_path} pronto para uso com rescomp.')
    print('Verificar visualmente no emulador com a tela de teste de paleta')
    print('(pressionar C na tela de título).')
    return 0


if __name__ == '__main__':
    sys.exit(main())
