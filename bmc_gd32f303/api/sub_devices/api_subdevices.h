#ifndef __API_SUB_DEVICES_H
#define	__API_SUB_DEVICES_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include <stdbool.h>
#include <stdint.h>


#define SUB_DEVICES_BUFF_SIZE  0x05
//计算 存储 网络 通讯 刀片
typedef enum {
    SUB_DEVICE_MODE_MAIN        = 0,
    SUB_DEVICE_MODE_POWER       = 1,
    SUB_DEVICE_MODE_NET         = 2,
    SUB_DEVICE_MODE_SWITCH      = 3,
    SUB_DEVICE_MODE_STORAGE0    = 4,
    SUB_DEVICE_MODE_STORAGE1    = 5,
    SUB_DEVICE_MODE_STORAGE2    = 6,
    SUB_DEVICE_MODE_MAX,
}SUB_DEVICE_MODE ;


typedef enum {
    SUB_DEVICE_REG_ERR_EXIST  = -10,
    SUB_DEVICE_REG_ERR_,
}SUB_DEVICE_REG_ERR_CODE ;


typedef struct
{
    SUB_DEVICE_MODE  mode;
    char     *name;
} SubDeviceName_T;

typedef struct
{
    SUB_DEVICE_MODE mode;
    bool isMain;
    bool isRegistered;
    uint8_t     slaveAddrI2cBus1; //8bit
    uint8_t     slaveAddrI2cBus2;
    const char     *name;
    uint8_t buff[SUB_DEVICES_BUFF_SIZE];
} SubDeviceMODE_T;

bool SubDevice_Init(void);
bool SubDevice_IsMain(void);
bool SubDevice_Management(uint8_t addr);
SubDeviceMODE_T *SubDevice_GetSlef(void);

#ifdef __cplusplus
}
#endif

#endif /* __API_SUB_DEVICES_H */

