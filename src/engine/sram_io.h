#ifndef ELIFOOT_SRAM_IO_H
#define ELIFOOT_SRAM_IO_H

#include <genesis.h>

/*
 * engine/sram_io.h — Save/Load em SRAM (cartucho com bateria, 8 KB)
 *
 * Layout dos 8 KB de SRAM:
 *   0x0000–0x0003  Magic "ELF2" (valida presença de dados)
 *   0x0004–0x0005  Versão do formato (u16, big-endian)
 *   0x0006–0x07FF  Slot 0 (2042 bytes)
 *   0x0800–0x0FFF  Slot 1 (2048 bytes)
 *   0x1000–0x17FF  Slot 2 (2048 bytes)
 *   0x1800–0x1FFF  Reserva
 *
 * Conteúdo de cada slot (1824 bytes usados de 2048 disponíveis):
 *   [0..1]   CRC16 do conteúdo do slot (bytes 2..1823)
 *   [2]      season_num
 *   [3]      round
 *   [4]      player_team_idx
 *   [5]      cup_phase
 *   [6..411] 29 equipas × 14 bytes cada
 *   [412..1803] 464 jogadores × 3 bytes cada
 *   [1804..1823] 5 entradas de palmares × 4 bytes cada
 *
 * SGDK 1.70: SRAM_readBuffer/writeBuffer não existem.
 * Usar SRAM_readByte/writeByte em loop (implementado aqui como estático).
 *
 * CRÍTICO: sempre usar o par SRAM_enable() / SRAM_disable().
 * Em hardware real, esquecer SRAM_disable() pode corromper dados.
 */

#define SRAM_SAVE_VERSION  0x0001u
#define SRAM_SLOT_SIZE     0x0800u   /* 2048 bytes por slot            */

/*
 * sram_init() — Verifica se a SRAM tem o magic "ELF2" e a versão correta.
 * Se não tiver, escreve o cabeçalho (inicializa SRAM virgem).
 * Deve ser chamada uma vez após data_init().
 */
void sram_init(void);

/*
 * sram_save() — Serializa o estado actual do jogo no slot (0, 1 ou 2).
 * Calcula CRC16 e escreve. Retorna 1 se OK, 0 se slot inválido.
 */
u8 sram_save(u8 slot);

/*
 * sram_load() — Lê e deserializa o slot. Valida CRC16.
 * Retorna 1 se OK, 0 se CRC inválido ou slot vazio.
 */
u8 sram_load(u8 slot);

/*
 * sram_slot_valid() — Retorna 1 se o slot tem dados com CRC válido.
 * Não altera o estado do jogo. Útil para a tela de save/load.
 */
u8 sram_slot_valid(u8 slot);

/*
 * sram_slot_season() — Retorna o número da temporada gravada no slot,
 * sem carregar o estado completo. Retorna 0 se slot inválido.
 */
u8 sram_slot_season(u8 slot);

#endif /* ELIFOOT_SRAM_IO_H */
