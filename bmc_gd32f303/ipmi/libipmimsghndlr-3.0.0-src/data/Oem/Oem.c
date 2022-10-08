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


extern VariableCPUParam g_CPUVariableParam;
extern FixedCPUParam g_CPUFixedParam;
extern __IO bool g_CPUStatus;

const CmdHndlrMap_T g_Oem_CmdHndlr[] =
{
/*--------------------- OEM Device Commands ---------------------------------*/
#if OEM_DEVICE == 1
        { CMD_SET_FAN,           PRIV_OEM,      SET_FAN,             0x00,   0xAAAA ,0xFFFF},
        // { CMD_UPDATE_FIRMWARE,   PRIV_OEM,      UPDATE_FIRMWARE,     0x00,   0xAAAA ,0xFFFF},
        {CMD_CPU_INFO, PRIV_OEM, CPU_INFO, 0x00, 0xAAAA, 0xFFFF},
        {CMD_BMC_INFO, PRIV_OEM, BMC_INFO, 0x00, 0xAAAA, 0xFFFF},

#endif /* OEM_DEVICE */

        {0x00, 0x00, 0x00, 0x00, 0x0000, 0x0000}
};


int GetFan(_NEAR_ INT8U *pReq, INT8U ReqLen, _NEAR_ INT8U *pRes, _NEAR_ int BMCInst)
{
	
	_NEAR_  FANPWM_T* pGetFanPWMRes = (_NEAR_ FANPWM_T*) pRes;
	pGetFanPWMRes->channel = *pReq;
	INT16U fanRpm = 0;
	if ( ReqLen < 0x1 ) 
	{
		*pRes =CC_REQ_INV_LEN;
		return 1;
	}
	if( pGetFanPWMRes->channel > SENSOR_FAN_NUM)
	{
		*pRes = CC_INV_DATA_FIELD;
		return 1;
	}
   fan_get_rotate_rpm(pGetFanPWMRes->channel, &fanRpm);
   pGetFanPWMRes->CompletionCode = CC_NORMAL;
   pGetFanPWMRes->rpm = fanRpm;

	return sizeof(FANPWM_T);
}
int SetFan(_NEAR_ INT8U *pReq, INT8U ReqLen, _NEAR_ INT8U *pRes, _NEAR_ int BMCInst)
{
	const SetSensorReq_T *pOemReq = (const SetSensorReq_T *)pReq;
	GetSensorReadingRes_T* sensor_reading_res = (GetSensorReadingRes_T*)pRes;
	int i = 0;

	sensor_reading_res->CompletionCode  = CC_NORMAL;
	sensor_reading_res->SensorReading   = 0;
	sensor_reading_res->Flags           = 0x00;
	sensor_reading_res->ComparisonStatus= 0x00;
	sensor_reading_res->OptionalStatus  = 0x00;
	
	if(pOemReq->FanNum == 0xFF)   // set all of fan
	{
		for(i=0; i<SENSOR_FAN_NUM; i++)
		{
			fan_set_duty_percent(i, pOemReq->PwmDuty);
		}
	}
	else
	{
		fan_set_duty_percent(pOemReq->FanNum-1, pOemReq->PwmDuty);
	}
	
	
	return sizeof(GetSensorReadingRes_T);
}

int UpdateFirmware(_NEAR_ INT8U *pReq, INT8U ReqLen, _NEAR_ INT8U *pRes, _NEAR_ int BMCInst)
{


	return 0;
}

int GetCPUInfo(_NEAR_ INT8U *pReq, INT8U ReqLen, _NEAR_ INT8U *pRes, _NEAR_ int BMCInst)
{
    GetCPUInfoRes_T *cpu_res = (GetCPUInfoRes_T *)pRes;

    cpu_res->CompletionCode  = CC_NORMAL;

    cpu_res->DeviceInfo = 0xFF;
    cpu_res->BoardType = 0x10 + get_board_addr();
    cpu_res->BIOSVersion = g_CPUFixedParam.BiosVersion;
    cpu_res->OSVersion = g_CPUFixedParam.KernelVersion;
    cpu_res->SevicePackVersion = g_CPUFixedParam.SevicePackVersion;
    cpu_res->CPUModel = g_CPUFixedParam.CPUModel;
    cpu_res->RamSpeed = g_CPUFixedParam.RamSpeed;
    cpu_res->RamSize = g_CPUFixedParam.RamSize;
    cpu_res->SSDSize = g_CPUFixedParam.SSDSize;
    cpu_res->CPURate = g_CPUVariableParam.CPURate;
    cpu_res->MemRate = g_CPUVariableParam.MemRate;
    cpu_res->UsedSSDSize = g_CPUVariableParam.UsedSSDSize;
    cpu_res->Eth0Rate = g_CPUVariableParam.Eth0Rate;
    cpu_res->Eth1Rate = g_CPUVariableParam.Eth1Rate;
    cpu_res->Eth2Rate = g_CPUVariableParam.Eth2Rate;
    cpu_res->Eth3Rate = g_CPUVariableParam.Eth3Rate;
    cpu_res->Eth4Rate = g_CPUVariableParam.Eth4Rate;

    cpu_res->Eth5Rate = g_CPUVariableParam.Eth5Rate;
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
extern uint16_t g_bmc_firmware_version;

int GetBMCInfo(_NEAR_ INT8U *pReq, INT8U ReqLen, _NEAR_ INT8U *pRes, _NEAR_ int BMCInst)
{
    GetBMCInfoRes_T *bmc_res = (GetBMCInfoRes_T *)pRes;

    bmc_res->CompletionCode  = CC_NORMAL;

    bmc_res->BMCFirmwareVersion = g_bmc_firmware_version;
    bmc_res->BMCFirmwareTime = g_utc_time_bmc_firmware_build;
	  bmc_res->BMCRunTime  = GetBmcRunTime();
    bmc_res->CPUStatus = g_CPUStatus;
    bmc_res->Eth0Status = g_CPUVariableParam.Eth0Status;
    bmc_res->Eth1Status = g_CPUVariableParam.Eth1Status;
    bmc_res->Eth2Status = g_CPUVariableParam.Eth2Status;
    bmc_res->Eth3Status = g_CPUVariableParam.Eth3Status;
    bmc_res->Eth4Status = g_CPUVariableParam.Eth4Status;
    bmc_res->Eth5Status = g_CPUVariableParam.Eth5Status;
    bmc_res->CPUstartupTime = g_CPUFixedParam.CPUStartTime;

    return sizeof(GetBMCInfoRes_T);
}
