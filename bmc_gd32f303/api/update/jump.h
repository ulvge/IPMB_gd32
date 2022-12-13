
#ifndef UPDATE_H
#define UPDATE_H

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <debug_print.h>
#include "Types.h"

/*   **********   xmode   start   ***********/
#define XMODEM_SOH 0x01       /* Xmodem数据头 */
#define XMODEM_STX 0x02       /* 1K-Xmodem数据头 */
#define XMODEM_EOT 0x04       /* 发送结束 */
#define XMODEM_ACK 0x06       /* 认可响应 */
#define XMODEM_NAK 0x15       /* 不认可响应 */
#define XMODEM_CANCEL 0x18    /* 撤销传送 */
#define XMODEM_CTRLZ 0x1A     /* 填充数据包 */
#define XMODEM_HANDSHAKECRC 0x43 /* 握手 C  当接收方一开始启动传输时发送的是字符“C”，表示它希望以CRC方式校验*/

#define XMODEM_CTRLC 0x03 /* abandon startup ,and prepare to upload */

#define XMODEM_PKTBUFLEN 128
#define BOOT_I2C_SPECIAL_IDENTIFICATION  0x5A

typedef __packed struct {
    uint8_t head;
    uint8_t pn; // start 1
    uint8_t xpn;
    uint8_t data[XMODEM_PKTBUFLEN];
    uint16_t crc;
} XMODEM_Msg;

typedef enum {
    XMODEM_CHECK_SUM = 0,
    XMODEM_CHECK_CRC16,
} XMODEM_CHECK_TYPE;

#define BOOT_I2C_BUS  I2C_BUS_0

/*   **********   xmode   end   ***********/

#define ADDRESS_BOOTLOADER_START  0x08000000

#define ADDRESS_APP_START         0x08008000
#define ADDRESS_APP_END           0x08020000

#define XMODEM_PAKGE_LENGTH 128

#define ENTRY_UPDATE_MODE_CMD   0x01
#define GET_CHIP_INFO_CMD       0x02
#define ERASE_FLASH_CMD         0x03
#define UPDATING_CMD            0x04
#define RESTART_BOARD_CMD       0x05

#define APP_WANTTO_UPDATE_KEYS_ADDR     BKP_DATA_0
#define APP_WANTTO_UPDATE_KEYS   0xA55A

#define I2C_UPDATE_KEYS_ADDR    BKP_DATA_1
#define I2C_UPDATE_KEYS         0x5F5F

#define I2C_UPDATE_MODE_ADDR    BKP_DATA_2


void update_JumpToRun(uint32_t jumpCodeAddr);
void update_BkpDateWrite(bkp_data_register_enum register_number, uint16_t data);
uint16_t update_BkpDateRead(bkp_data_register_enum register_number);

#endif /* UPDATE_H */
