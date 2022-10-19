/****************************************************************
 ****************************************************************

 ****************************************************************
 ****************************************************************
 *****************************************************************
 *
 * IPMI_App.h
 * Application Command numbers
 *
 *****************************************************************/
#ifndef IPMI_OEM_H
#define IPMI_OEM_H

#include "Types.h"

/*** Application Commands ***/
#define         CMD_GET_BLADE_ID    0x01
#define         CMD_UPDATE_FIRMWARE   0x02
#define         CMD_GET_FAN_RPM     0x03
#define         CMD_SET_FAN_PWM     0x04
#define       CMD_MCU_RESET         0x05
#define       CMD_CPU_INFO          0x10    //get_board_addr
#define       CMD_BMC_INFO          0x11

#pragma pack( 1 )
/* SetSensorReq_T */
typedef struct
{
    INT8U   FanIndex;
    INT8U   PwmDuty;

} SetSensorReq_T;

/* GetCPUInfoRes_T */
typedef struct
{
    INT8U CompletionCode;     /* Completion Code */
    INT8U DeviceInfo;
    INT8U BoardType;
    INT8U BIOSVersion;
    INT8U OSVersion;
    INT8U SevicePackVersion;
    INT32U CPUModel;
    INT32U RamSpeed;
    INT32U RamSize;
    INT32U SSDSize;
    INT8U CPURate;
    INT8U MemRate;
    INT32U UsedSSDSize;
    INT8U Eth0Rate;
    INT8U Eth1Rate;
    INT8U Eth2Rate;
    INT8U Eth3Rate;
    INT8U Eth4Rate;

    INT8U Eth5Rate;
    INT8U OutletTemp;
    INT8U InletTemp;
    INT8U CPUTemp;
    INT8U GPUTemp;
    INT8U BoardTemp;
    INT8U DIMMTemp;
    INT8U BoardConsumption;

    INT8U P12VVoltage;
    INT8U P5VVoltage;
    INT8U P3V3Voltage;
    INT8U VCCINVoltage;
    INT8U P1V05Voltage;
    INT8U P1V5Voltage;
    INT8U VCCIOVoltage;
    INT8U VPP_ABVoltage;

    INT8U VPP_CDVoltage;
    INT8U VDDQ_AB_Voltage;
    INT8U VDDQ_CD_Voltage;
    INT8U VTT_AB_Voltage;
    INT8U P1V05_SUS_Voltage;
    INT8U P3V3_AUX_Voltage;
    INT8U VTT_CD_Voltage;
    INT8U WatchdogStatus;

    INT8U FRUState;
    INT8U AMP_12;
    INT8U AMP_3V3;
    INT8U TSI721_1V;
    INT8U XL710_0V85;
    INT8U Reserved1;
    INT8U Reserved2;
    INT8U Reserved3;

} GetCPUInfoRes_T;

/* GetBMCInfoRes_T */
typedef struct
{
    INT8U CompletionCode;     /* Completion Code */
    INT16U BMCFirmwareVersion;
    INT32U BMCFirmwareTime;
    INT32U BMCRunTime;
    INT8U CPUStatus;
    INT8U Eth0Status;
    INT8U Eth1Status;
    INT8U Eth2Status;
    INT8U Eth3Status;
    INT8U Eth4Status;
    INT8U Eth5Status;
    INT32U CPUstartupTime;

} GetBMCInfoRes_T;

#pragma pack( )


#endif  /* IPMI_OEM_H */

