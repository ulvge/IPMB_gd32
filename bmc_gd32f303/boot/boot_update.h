#ifndef __BOOT_UPDATE_H
#define	__BOOT_UPDATE_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include <stdbool.h>
#include <stdint.h>   
#include "OSPort.h"
#include "boot_update.h"


#define Authorize_CMD   0x01
#define UpdateRom_CMD   0x02

#define HEX_Record       0x00
#define HEX_End          0x01
#define HEX_Ext          0x02
#define HEX_Start        0x03
#define HEX_Ext_Linear   0x04
#define HEX_Start_Linear 0x05


#define CC_ENTOR_UPDATE       0x01
#define CC_UPDATING_NORMAL    0x02
#define CC_UPDATING_FAIL      0x03
#define CC_RESTART            0x04
#define CC_AUTHORIZE_PASS     0x05
#define CC_AUTHORIZE_FAIL     0x06

#define CC_ERROR              0xFF



#define XMODEM_PKTBUFLEN 128
#define BOOT_OK     0
#define BOOT_ERROR  1
#define BOOT_TIMEOUT  2
#define BOOT_POINTEREMPTY  3
#define BOOT_ARRAYBOUND  4
#define BOOT_CONTINUE  5

 typedef enum {
     UPDATE_EVENT_START = 1,
     UPDATE_EVENT_FINISHED,
     UPDATE_EVENT_ERROR_TRYAGAIN,
     UPDATE_EVENT_CANCEL,
     UPDATE_EVENT_CONTINUE
 } UPDATE_EVENT;

 typedef struct {
     uint16_t cmd1;
     uint8_t cmd2;
     uint8_t sq;
     uint8_t addr;
     uint8_t len;
     char data[64];
     uint8_t crc;
}UpdateMsgReq;

typedef struct{
    uint16_t cmd1;
    uint8_t  cmd2;
    uint8_t  sq;
    uint8_t  addr;
    uint8_t  complete_code;  // 00:normal
    uint8_t  crc;
}UpdateMsgRes;

typedef struct{
    uint8_t   len;
		uint16_t  addr;
		uint8_t   type;
    char      data[16];
    uint8_t   sum;
}HexRomMsg;

typedef struct{
    uint8_t   head;
    uint8_t   pn; //start 1
    uint8_t   xpn;
    uint8_t   data[XMODEM_PKTBUFLEN];
    uint16_t  crc;
}xmodemMsg;

typedef struct
{
    INT8U       Size;                      /* Command that needs to be processed*/
    INT8U       Data [150];             /* Data */
} PACKED BootPkt_T;

#define XMODEM_SOH 0x01   /* Xmodem数据头 */
#define XMODEM_STX 0x02   /* 1K-Xmodem数据头 */
#define XMODEM_EOT 0x04   /* 发送结束 */
#define XMODEM_ACK 0x06   /* 认可响应 */
#define XMODEM_NAK 0x15   /* 不认可响应 */
#define XMODEM_CANCEL 0x18   /* 撤销传送 */
#define XMODEM_CTRLZ 0x1A /* 填充数据包 */


extern TaskHandle_t jump_task_handle;
extern xQueueHandle updateDatMsg_Queue;

void JumpToAPP(void);
void updateTask(void *arg);


#ifdef __cplusplus
}
#endif

#endif /* __BOOT_UPDATE_H */




