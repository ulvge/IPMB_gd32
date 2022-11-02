#include <string.h>  
#include <stdlib.h>
#include "debug_print.h"
#include "api_subdevices.h"
#include "bsp_gpio.h"
#include "bsp_i2c.h"
#include "IPMIConf.h"
#include "IPMDevice.h"
#include "OSPort.h"
#include "IPMI_SensorEvent.h"
#include "ipmi_common.h"
#include "sdr.h"
#include "IPMI_Sensor.h"
#include "sensor_helpers.h"
#include "sensor.h"    
#include "api_adc.h"
#include "api_sensor.h"

#define SUB_DEVICES_ADDR_DEFAULT 0xFF
#define SUB_DEVICES_ADDR_PRIFIXED 0x80

#define SUB_DEVICES_FAILED_MAX_COUNT 10
#define SUB_DEVICES_SENDMSG_WAIT_TIMEOUT_XMS 20
#define SUB_DEVICES_TIMER_PERIOD_XMS 2000

typedef struct
{
    uint32_t  busUsed;
} SubDeviceHandler_T;

static TimerHandle_t xTimersIpmiReset = NULL;
static SubDeviceHandler_T g_subDeviceHandler;

static void SubDevice_HeartBeatTimerCallBack(xTimerHandle pxTimer);
static const SubDeviceName_T g_SubDeviceConfigName[] = {
    {SUB_DEVICE_MODE_MAIN,      "main"},
    {SUB_DEVICE_MODE_POWER,     "power"},
    {SUB_DEVICE_MODE_NET,       "net"},
    {SUB_DEVICE_MODE_SWITCH,    "switch"},
    {SUB_DEVICE_MODE_STORAGE0,  "storage0"},
    {SUB_DEVICE_MODE_STORAGE1,  "storage1"},
    {SUB_DEVICE_MODE_STORAGE2,  "storage2"},
};

static char* SubDevice_GetModeName(SUB_DEVICE_MODE mode)
{
    if (mode < SUB_DEVICE_MODE_MAX) {
        for (uint8_t i = 0; i < SUB_DEVICE_MODE_MAX; i++)
        {
			if (mode == g_SubDeviceConfigName[i].mode) {
				return g_SubDeviceConfigName[i].name;
			}
		}
    }
    return "\n";
}
static SubDeviceMODE_T g_AllModes[SUB_DEVICE_MODE_MAX];

static void SubDevice_InitAllMode(void)
{
    for (uint8_t i = 0; i < SUB_DEVICE_MODE_MAX; i++)
    {
        SubDeviceMODE_T *obj = &g_AllModes[i];
        obj->isMain = false;
        obj->isOnLine = false;
        obj->mode = SUB_DEVICE_MODE_MAX;
        obj->name = g_SubDeviceConfigName[i].name;
        obj->i2c0SlaveAddr = obj->i2c1SlaveAddr = SUB_DEVICES_ADDR_DEFAULT;
    }
}

static uint8_t SubDevice_modeConvertSlaveAddr(SUB_DEVICE_MODE mode)
{
    return SUB_DEVICES_ADDR_PRIFIXED | (mode << 1);
}
static void SubDevice_InsertMode(SubDeviceMODE_T *obj, SUB_DEVICE_MODE mode)
{
    if (mode == SUB_DEVICE_MODE_MAIN) {
        obj->isMain = true;
    }
    obj->isOnLine = true;
    obj->mode = mode;
    obj->name = g_SubDeviceConfigName[mode].name;
    obj->i2c0SlaveAddr = obj->i2c1SlaveAddr = SubDevice_modeConvertSlaveAddr(mode);
}           
static SubDeviceMODE_T *pSubDeviceSelf = NULL;
bool SubDevice_CheckAndPrintMode(void)
{
    SUB_DEVICE_MODE mode = (SUB_DEVICE_MODE)get_board_addr();
    if (mode >= SUB_DEVICE_MODE_MAX) {
        printf("This borad ID is not support, id=%d\n", mode);
        printf("\tBelow are supported\n");
        for (uint32_t i = 0; i < ARRARY_SIZE(g_SubDeviceConfigName); i++)
        {
            printf("\t\tid %d----- [%s]\n", i, SubDevice_GetModeName((SUB_DEVICE_MODE)i));
        }
        return false;
    }
    printf("This borad as [%s]\n", SubDevice_GetModeName(mode));
    SubDevice_InitAllMode();

    SubDevice_InsertMode(&g_AllModes[mode], mode);
    pSubDeviceSelf = &g_AllModes[mode];
    return true;
}
/// @brief read my slave address from GPIO switch,then init all buff if I'm a main(master)
/// @param  
/// @return 
bool SubDevice_Init(void)
{
    g_subDeviceHandler.busUsed = NM_SECONDARY_IPMB_BUS;
	BaseType_t xReturn = pdPASS; 
	if (SubDevice_IsSelfMaster()) {
        xTimersIpmiReset = xTimerCreate("SubDeviceHeartBeat", SUB_DEVICES_TIMER_PERIOD_XMS/portTICK_RATE_MS, pdTRUE, 
                                        0, SubDevice_HeartBeatTimerCallBack);
		if (xTimersIpmiReset == NULL) {
			printf("SubDevice_Init xTimerCreate failed\n");     
			return false;
		}
		
        BaseType_t xReturn = xTimerStart(xTimersIpmiReset, portMAX_DELAY);
		if (pdFAIL != pdPASS) {
			printf("SubDevice_Init xTimerStart failed %ld\n", xReturn); 
		}
	}
	return true;
}

bool SubDevice_IsSelfMaster(void)
{
    if (pSubDeviceSelf == NULL) {
        return false;
    }
    return pSubDeviceSelf->isMain;
}
SUB_DEVICE_MODE SubDevice_GetMyMode(void)
{
    if (pSubDeviceSelf == NULL) {
        return SUB_DEVICE_MODE_MAX;
    }
    return pSubDeviceSelf->mode;
}
bool SubDevice_IsOnLine(void)
{
    if (pSubDeviceSelf == NULL) {
        return false;
    }
    return pSubDeviceSelf->isOnLine;
}
/// @brief master and slave i2c init will be called
/// @param bus 
/// @return 
uint8_t SubDevice_GetMySlaveAddress(uint32_t bus)
{
    if (pSubDeviceSelf == NULL) {
        return 0;
    }
    switch (bus)
    {
        case I2C0:
        case NM_PRIMARY_IPMB_BUS:
            return pSubDeviceSelf->i2c0SlaveAddr;
        case I2C1:
        case NM_SECONDARY_IPMB_BUS:
            return pSubDeviceSelf->i2c1SlaveAddr;
        default:
            return 0;
    }
}
SubDeviceMODE_T *SubDevice_GetSelf(void)
{
    return pSubDeviceSelf;
}
// master called only
static void SubDevice_SetOnLine(SUB_DEVICE_MODE mode, bool isOnline)
{
    for (uint8_t i = 0; i < SUB_DEVICE_MODE_MAX; i++)
    {
        if (g_AllModes[i].mode == mode) {
            pSubDeviceSelf->isOnLine = isOnline;
            return;
        }
    }
}
    
static bool SubDevice_QueryModeByAddr(uint8_t addrBit8, SUB_DEVICE_MODE *mode)
{
    uint8_t addrHi = addrBit8 & 0xF0;
    uint8_t addrLow = (addrBit8 & 0x0F) >> 1;

    if (addrHi != SUB_DEVICES_ADDR_PRIFIXED) {
        return false;
    }

    if (addrLow > SUB_DEVICE_MODE_MAX) {
        return false;
    }
    *mode = (SUB_DEVICE_MODE)addrLow;
    return true;
}
bool SubDevice_Management(uint8_t addr)
{
    SUB_DEVICE_MODE newMode;
    if (SubDevice_QueryModeByAddr(addr, &newMode) == false) {
        return false;
    }
    uint8_t i = 0;
    for (i = 0; i < SUB_DEVICE_MODE_MAX; i++)
    {
        if (g_AllModes[i].mode == newMode) {
            return false; //already exist
        }
        if (g_AllModes[i].mode == SUB_DEVICE_MODE_MAX) {// get a new buff
            SubDevice_InsertMode(&g_AllModes[i], newMode);
            return true;
        }
    }
    return false; // full
}
uint32_t SubDevice_GetBus(void)
{
    return g_subDeviceHandler.busUsed;
}
static bool SubDevice_readingSensorForeach(SUB_DEVICE_MODE dev, uint8_t sensorNum, MsgPkt_T *requestPkt, SubDevice_Reading_T *pDeviceReading)
{
    MsgPkt_T recvReq;
    IPMIMsgHdr_T *hdr = (IPMIMsgHdr_T *)&(requestPkt->Data);
    
    int len = sizeof(IPMIMsgHdr_T);
    requestPkt->Data[len++] = sensorNum;
    requestPkt->Data[len++] = dev; // OEMFiled

    requestPkt->Data[len++] = CalculateCheckSum(requestPkt->Data, len);
    requestPkt->Size = len;

    GetSensorReadingRes_T *pSensorReading = (GetSensorReadingRes_T *)(&recvReq.Data[sizeof(IPMIMsgHdr_T)]);
    if (dev == SUB_DEVICE_MODE_MAIN) { // skip master self
        requestPkt->Data[0] = sensorNum;
        requestPkt->Data[1] = dev;
        GetSensorReading(requestPkt->Data, 2, (INT8U *)&recvReq.Data[sizeof(IPMIMsgHdr_T)], 0);
    }else {
        if (SendMsgAndWait(requestPkt, &recvReq, SUB_DEVICES_SENDMSG_WAIT_TIMEOUT_XMS) == pdFALSE)
        {
            return false;
        }
        IPMIMsgHdr_T *recHdr = (IPMIMsgHdr_T *)&recvReq.Data;
        // pSensorReadRes->OptionalStatus = pSensorReadReq->SensorNum;
        if ((recHdr->ReqAddr != hdr->ResAddr) || (pSensorReading->OptionalStatus != sensorNum)) {
                return false;
        }
    }

    if (pSensorReading->CompletionCode == CC_NORMAL) {
        pDeviceReading->raw = pSensorReading->SensorReading;
        pDeviceReading->ComparisonStatus = pSensorReading->ComparisonStatus;
        return true;
    }
    return false;

}
extern xQueueHandle ResponseDatMsg_Queue;
static void SubDevice_HeartBeatTimerCallBack(xTimerHandle pxTimer)
{
	//uint32_t cmd = (CHASSIS_CMD_CTRL)((uint32_t)pvTimerGetTimerID(pxTimer));

    BaseType_t xReturn = xTimerStop(xTimersIpmiReset, portMAX_DELAY);
    if (xReturn != pdPASS) {
        return; 
    }

    MsgPkt_T requestPkt;
    IPMIMsgHdr_T *hdr = (IPMIMsgHdr_T *)&(requestPkt.Data);
    requestPkt.Param = IPMB_SUB_DEVICE_HEARTBEAT_REQUEST;
    requestPkt.Channel = SubDevice_GetBus();
	uint8_t sensorNum;                                        
												 
    for (SUB_DEVICE_MODE dev = (SUB_DEVICE_MODE)0; dev < SUB_DEVICE_MODE_MAX; dev++)
    {
        const Sensor_Handler *pHandler = api_getSensorHandler(SubDevice_GetMyMode());
        if (pHandler == NULL){
            continue;
        }
        //hdr->ResAddr = 0x20;      //  main ipmb
        hdr->ResAddr = SubDevice_modeConvertSlaveAddr(dev);
        hdr->NetFnLUN = NETFN_SENSOR << 2; // RAW
        hdr->ChkSum = CalculateCheckSum((INT8U*)hdr, 2);
        
        hdr->ReqAddr = SubDevice_GetMySlaveAddress(requestPkt.Channel);
        hdr->RqSeqLUN = 0x01;
        hdr->Cmd = CMD_GET_SENSOR_READING;

       for (uint8_t numIdex = 0; numIdex < pHandler->sensorCfgSize; numIdex++)
       {
           SubDevice_Reading_T *pDeviceReading = &pHandler->val[numIdex];
           //sensorNum = 0x23;     // P1V8 standby
           sensorNum = api_sensorGetSensorNumByIdex(numIdex);
           if (SubDevice_readingSensorForeach(dev, sensorNum, &requestPkt, pDeviceReading)) {
               if (api_sensorConvert2HumanVal(dev, sensorNum, pDeviceReading->raw, &pDeviceReading->human) == true) {
                   pDeviceReading->errCnt = 0;
                   continue; //success
               }
           }
           if (pDeviceReading->errCnt++ > SUB_DEVICES_FAILED_MAX_COUNT) {
               pDeviceReading->errCnt = 0;
               pDeviceReading->raw = 0;
               pDeviceReading->human = 0;
           }
       }
    }

    xReturn = xTimerStart(xTimersIpmiReset, portMAX_DELAY);
}
