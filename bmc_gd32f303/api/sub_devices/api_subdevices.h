#ifndef __API_SUB_DEVICES_H
#define	__API_SUB_DEVICES_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include <stdbool.h>
#include <stdint.h>


#define SUB_DEVICES_HAVE_SENSOR_COUNT 2
//计算 存储 网络 通讯 刀片
typedef enum {
    SUB_DEVICE_MODE_MAIN        = 0,
    SUB_DEVICE_MODE_NET         = 1,
    SUB_DEVICE_MODE_SWITCH      = 2,
    SUB_DEVICE_MODE_POWER       = 3,
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
    uint8_t raw;
    uint8_t errCnt;
    float   human;
} SubDevice_Reading_T;

typedef struct
{
    SUB_DEVICE_MODE mode;
    bool isMain;
    bool isOnLine;
    uint8_t     i2c0SlaveAddr; //8bit
    uint8_t     i2c1SlaveAddr; //8bit
    const char     *name;
    SubDevice_Reading_T reading[SUB_DEVICES_HAVE_SENSOR_COUNT];
} SubDeviceMODE_T;

bool SubDevice_Init(void);
bool SubDevice_IsSelfMaster(void);
bool SubDevice_IsOnLine(void);
uint8_t SubDevice_GetMySlaveAddress(uint32_t bus);
bool SubDevice_Management(uint8_t addr);
uint32_t SubDevice_GetBus(void);
SUB_DEVICE_MODE SubDevice_GetMyMode(void);

SubDeviceMODE_T *SubDevice_GetSelf(void);

#ifdef __cplusplus
}
#endif

#endif /* __API_SUB_DEVICES_H */

