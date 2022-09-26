#ifndef __API_CPU_H
#define	__API_CPU_H


#ifdef __cplusplus
 extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "Types.h"
//#include "core_cm3.h"

/* para definitions */


#define CPUVariInfoCmd     0x03
#define CPUFixedInfoCmd    0x04
#define CPUPowerOffCmd     0x05
#define CPUPowerUpCmd      0x06
#define CPUResetCmd        0x07

#define	SERIAL_COMM_REQUEST				1
#define	SERIAL_SNMP_REQUEST				2
#define	SERIAL_CONNECT_TO_MODEM			4
#define	SERIAL_ALERT_REQUEST			5
#define	SERIAL_CLOSE_SESS_REQUEST		6
#define	SERIAL_INIT_CALLBACK_REQUEST	7
#define	SERIAL_PING_REQUEST				8
#define	SERIAL_TERM_MODE_REQUEST		9

#define BASIC_MODE_CALLBACK				3
#define	PPP_MODE_CALLBACK				4

#define START_BYTE              0xA0
#define STOP_BYTE               0xA5
#define HANDSHAKE_BYTE          0xA6
#define DATA_ESCAPE_BYTE             0xAA
				  
#define	BYTE_ESC					0x1B

#define		ENCODED_START_BYTE			0xB0
#define		ENCODED_STOP_BYTE			0xB5
#define		ENCODED_HAND_SHAKE_BYTE		0xB6
#define		ENCODED_DATA_ESCAPE			0xBA
#define		ENCODED_BYTE_ESCAPE			0x3B

#define STATE_IDLE              0x00
#define STATE_RECEIVING         0x01
#define STATE_PKT_RECEIVED      0x02
#define PREV_BYTE_NOT_ESC       0x00
#define PREV_BYTE_ESC           0x01

#define ASCII_ESC_BYTE          0x1b
#define ASCII_ESC_ENCODE_BYTE   0x3b

#pragma pack(1)
typedef struct {
    uint8_t CPURate;
    uint8_t MemRate;
    uint32_t UsedSSDSize;
    uint8_t Eth0Rate;
    uint8_t Eth1Rate;
    uint8_t Eth2Rate;
    uint8_t Eth3Rate;
    uint8_t Eth4Rate;
    uint8_t Eth5Rate;
    uint8_t Eth0Status;
    uint8_t Eth1Status;
    uint8_t Eth2Status;
    uint8_t Eth3Status;
    uint8_t Eth4Status;
    uint8_t Eth5Status;

} VariableCPUParam;

typedef struct {
    uint8_t BiosVersion;
    uint8_t KernelVersion;
    uint8_t SevicePackVersion;
    uint32_t CPUModel;
    uint32_t RamSpeed;
    uint32_t RamSize;
    uint32_t SSDSize;
    uint32_t CPUStartTime;
} FixedCPUParam;

typedef struct {
    uint32_t ID;
    uint32_t Len;
    uint16_t Cmd;
} CPUMsgHdr;
#pragma pack()

extern void cpuGetInfoTask(void *arg);
extern int GetEncodeCmd(INT8U cmd, INT8U *pReq);
extern uint16_t GetBmcFirmwareVersion(char* str);
extern uint32_t GetBmcRunTime(void);

// __STATIC_INLINE uint32_t __get_LR(void)
// {
//   register uint32_t __regStackPointer;
//   __ASM volatile ("MRS %0, lr\n" : "=r" (__regStackPointer) );
//   return(__regStackPointer);
// }


#ifdef __cplusplus
}
#endif

#endif /* __API_CPU_H */

