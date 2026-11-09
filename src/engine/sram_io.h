#ifndef ELIFOOT_SRAM_IO_H
#define ELIFOOT_SRAM_IO_H

#include <genesis.h>

/*
 * engine/sram_io.h -- Gravacoes em SRAM (Alt+F1 GRAVAR O JOGO /
 * CONTINUAR UM JOGO). 3 gravacoes com nome, 8 KB cada, CRC-16.
 * O estado e escrito/lido directamente da SRAM (sem buffer em RAM).
 */

#define SRAM_SAVE_VERSION  0x0003u
#define SAVE_SLOTS         3u
#define SAVE_NAME_LEN      13u

void sram_init(void);
u8   sram_save(u8 slot, const char *name);
u8   sram_load(u8 slot);
u8   sram_slot_valid(u8 slot);
/* Nome e epoca de uma gravacao valida (para listar). */
u8   sram_slot_info(u8 slot, char *name, u8 *season);

#endif
