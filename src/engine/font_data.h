/* src/engine/font_data.h
 * Genesis 4bpp font tile data declarations.
 */
#ifndef FONT_DATA_H
#define FONT_DATA_H

#include <genesis.h>

/* 109 tiles: ASCII 32-127 (96) + 13 box-drawing glyphs.
 * 8 u32s per tile = 872 u32s total.
 * Load with: VDP_loadTileData(font_tile_data, FONT_BASE_TILE, 109, DMA) */
extern const u32 font_tile_data[109 * 8];

#endif /* FONT_DATA_H */
