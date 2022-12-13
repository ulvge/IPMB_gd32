/****************************************************************
 *****************************************************************
 *
 * Storage.c
 * Storage Command Handler
 *
 * Author:   
 *         
 *****************************************************************/
#define UNIMPLEMENTED_AS_FUNC
#include "Oem.h"
#include "MsgHndlr.h"
#include "Support.h"

#include "IPMI_Oem.h"
#include "IPMI_Sensor.h"
#include "fan/api_fan.h"
#include "Sensor.h"
#include "cpu/api_cpu.h"
#include "bsp_gpio.h"


//extern VariableCPUParam g_CPUVariableParam;
//extern FixedCPUParam g_CPUFixedParam;

const CmdHndlrMap_T g_Oem_CmdHndlr[] =
{
/*--------------------- OEM Device Commands ---------------------------------*/
#if OEM_DEVICE == 1
        { CMD_GET_BLADE_ID,           PRIV_OEM,      GET_BLADE_ID,             0x00,   0xAAAA ,0xFFFF},
        { CMD_GET_FAN_RPM,           PRIV_OEM,      GET_FAN_RPM,             0x00,   0xAAAA ,0xFFFF},
        { CMD_SET_FAN_PWM,           PRIV_OEM,      SET_FAN_RPM,             0x00,   0xAAAA ,0xFFFF},
        { CMD_UPDATE_FIRMWARE,   PRIV_OEM,      UPDATE_FIRMWARE,     0x00,   0xAAAA ,0xFFFF},
        {CMD_CPU_INFO, PRIV_OEM, CPU_INFO, 0x00, 0xAAAA, 0xFFFF},
        {CMD_BMC_INFO, PRIV_OEM, BMC_INFO, 0x00, 0xAAAA, 0xFFFF},

#endif /* OEM_DEVICE */

        {0x00, 0x00, 0x00, 0x00, 0x0000, 0x0000}
};

/// @brief get fan's RPM by sensor index,not sensor not sensor not sensor
/// @param pReq 
/// @param ReqLen 
/// @param pRes 
/// @param BMCInst 
/// @return 
static int GetFan(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
	FANPWM_T* pGetFanPWMRes = (FANPWM_T*) pRes;
	pGetFanPWMRes->index = *pReq;
	INT16U fanRpm = 0;
	if ( ReqLen < 0x1 ) 
	{
		*pRes =CC_REQ_INV_LEN;
		return 1;
	}
	INT32U fanSensorNum = 0;
	if(!fan_getFanSensorNum(pGetFanPWMRes->index, &fanSensorNum))
	{
		*pRes = CC_INV_DATA_FIELD;
		return 1;
	}
   fan_get_rotate_rpm(fanSensorNum, &fanRpm);
   pGetFanPWMRes->CompletionCode = CC_NORMAL;
   pGetFanPWMRes->rpm = fanRpm;

	return sizeof(FANPWM_T);
}
int SetFan(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
	if ( ReqLen < sizeof(SetSensorReq_T) ) 
	{
		*pRes =CC_REQ_INV_LEN;
		return 1;
	}
	const SetSensorReq_T *pOemReq = (const SetSensorReq_T *)pReq;
	GetSensorReadingRes_T* sensor_reading_res = (GetSensorReadingRes_T*)pRes;
	int i = 0;

	sensor_reading_res->CompletionCode  = CC_NORMAL;
	sensor_reading_res->SensorReading   = 0;
	sensor_reading_res->Flags           = 0x00;
	sensor_reading_res->ComparisonStatus= 0x00;
	sensor_reading_res->OptionalStatus  = 0x00;
	
	INT32U fanSensorNum = 0;
    if(pOemReq->FanIndex == 0xFF)   // set all of fan
	{
		for(i = 0; i<fan_getFanNum(); i++)
		{
            if(!fan_getFanSensorNum(i, &fanSensorNum)) {
                continue;
            }
			fan_set_duty_percent(fanSensorNum, pOemReq->PwmDuty);
		}
	}
	else
	{
        if(!fan_getFanSensorNum(pOemReq->FanIndex, &fanSensorNum))
        {
            *pRes = CC_INV_DATA_FIELD;
            return 1;
        }
		fan_set_duty_percent(fanSensorNum, pOemReq->PwmDuty);
	}
	return sizeof(GetSensorReadingRes_T);
}

typedef struct
{
    INT8U               CompletionCode;
    INT8U               bladeID;
    INT8U               rackID;
} PACKED BladeSlotIDRes_T;
int GetBladId(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
    UNUSED(pReq);
    UNUSED(ReqLen);
    BladeSlotIDRes_T *pbladeSlotIDRes_T = (BladeSlotIDRes_T *)pRes;
    pbladeSlotIDRes_T->CompletionCode = CC_NORMAL;
    INT8U ID = get_board_addr();
    pbladeSlotIDRes_T->bladeID = get_board_addr();
    pbladeSlotIDRes_T->rackID = 0;
    //printf("CmdGetBladeID bladeID=%#x, rackID=%#x, line=%d \n", pbladeSlotIDRes_T->bladeID,  pAMIBladeSlotIDRes_T->rackID, __LINE__);
    return (sizeof(BladeSlotIDRes_T));
}
int UpdateFirmware(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{


	return 0;
}

int GetCPUInfo(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
    GetCPUInfoRes_T *cpu_res = (GetCPUInfoRes_T *)pRes;

    cpu_res->CompletionCode  = CC_NORMAL;

    cpu_res->DeviceInfo = 0xFF;
    cpu_res->BoardType = 0x10 + get_board_addr();
//    cpu_res->BIOSVersion = g_CPUFixedParam.BiosVersion;
//    cpu_res->OSVersion = g_CPUFixedParam.KernelVersion;
//    cpu_res->SevicePackVersion = g_CPUFixedParam.SevicePackVersion;
//    cpu_res->CPUModel = g_CPUFixedParam.CPUModel;
//    cpu_res->RamSpeed = g_CPUFixedParam.RamSpeed;
//    cpu_res->RamSize = g_CPUFixedParam.RamSize;
//    cpu_res->SSDSize = g_CPUFixedParam.SSDSize;
    cpu_res->OutletTemp = 0xFF;
    cpu_res->InletTemp = 0xFF;
    cpu_res->CPUTemp = 0xFF;
    cpu_res->GPUTemp = 0xFF;
    cpu_res->BoardTemp = 0xFF;
    cpu_res->DIMMTemp = 0xFF;
    cpu_res->BoardConsumption = 0xFF;

    cpu_res->P12VVoltage = 0xFF;
    cpu_res->P5VVoltage = 0xFF;
    cpu_res->P3V3Voltage = 0xFF;
    cpu_res->VCCINVoltage = 0xFF;
    cpu_res->P1V05Voltage = 0xFF;
    cpu_res->P1V5Voltage = 0xFF;
    cpu_res->VCCIOVoltage = 0xFF;
    cpu_res->VPP_ABVoltage = 0xFF;

    cpu_res->VPP_CDVoltage = 0xFF;
    cpu_res->VDDQ_AB_Voltage = 0xFF;
    cpu_res->VDDQ_CD_Voltage = 0xFF;
    cpu_res->VTT_AB_Voltage = 0xFF;
    cpu_res->P1V05_SUS_Voltage = 0xFF;
    cpu_res->P3V3_AUX_Voltage = 0xFF;
    cpu_res->VTT_CD_Voltage = 0xFF;
    cpu_res->WatchdogStatus = 0xFF;

    cpu_res->FRUState = 0xFF;
    cpu_res->AMP_12 = 0xFF;
    cpu_res->AMP_3V3 = 0xFF;
    cpu_res->TSI721_1V = 0xFF;
    cpu_res->XL710_0V85 = 0xFF;
    cpu_res->Reserved1 = 0xFF;
    cpu_res->Reserved2 = 0xFF;
    cpu_res->Reserved3 = 0xFF;

    return sizeof(GetCPUInfoRes_T);
}

extern uint64_t g_utc_time_bmc_firmware_build;

int GetBMCInfo(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
    GetBMCInfoRes_T *bmc_res = (GetBMCInfoRes_T *)pRes;

    bmc_res->CompletionCode  = CC_NORMAL;

    bmc_res->BMCFirmwareVersion = GetBmcFirmwareVersion(BMC_VERSION);
    bmc_res->BMCFirmwareTime = g_utc_time_bmc_firmware_build;
    bmc_res->BMCRunTime  = GetBmcRunTime();

    return sizeof(GetBMCInfoRes_T);
}
