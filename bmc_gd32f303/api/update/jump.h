
#ifndef UPDATE_H
#define UPDATE_H

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <debug_print.h>

#define ADDRESS_START_BOOTLOADER  0x08000000

#define ADDRESS_START_APP         0x08008000
#define ADDRESS_END_APP           0x08020000

#pragma pack(1)

#pragma pack()

#define ENTRY_UPDATE_MODE_CMD   0x01
#define GET_CHIP_INFO_CMD       0x02
#define ERASE_FLASH_CMD         0x03
#define UPDATING_CMD            0x04
#define RESTART_BOARD_CMD       0x05

#define UPDATING_CMD_SYS_BOOT   "bt"
#define APP_WANTTO_UPDATE_KEYS   0x98761234
#define APP_WANTTO_UPDATE_KEYS_ADDR (SRAM_BASE + SRAM_BASE_LEN - 0x10)

void JumpToBootloader(void);

#endif /* UPDATE_H */
