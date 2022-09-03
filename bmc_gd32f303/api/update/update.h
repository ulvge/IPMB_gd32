
#ifndef UPDATE_H
#define UPDATE_H

#pragma pack(1)

typedef struct{
    uint16_t cmd1;
    uint8_t  cmd2;
    uint8_t  sq;
    uint8_t  addr;
    uint8_t  len;
    char     dat[64];
    uint8_t  crc;
}UpdateMsgReq;

typedef struct{
    uint16_t cmd1;
    uint8_t  cmd2;
    uint8_t  sq;
    uint8_t  addr;
    uint8_t  err_code;  // 00:normal
}UpdateMsgRes;

#pragma pack()

#define ENTRY_UPDATE_MODE_CMD   0x01
#define GET_CHIP_INFO_CMD       0x02
#define ERASE_FLASH_CMD         0x03
#define UPDATING_CMD            0x04
#define RESTART_BOARD_CMD       0x05

void updateTask(void *arg);

#endif /* UPDATE_H */
