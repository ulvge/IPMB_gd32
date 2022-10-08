/*****************************************************************
 *****************************************************************
 ***                                                            **
 ***    (C)Copyright 2005-2006, American Megatrends Inc.        **
 ***                                                            **
 ***            All Rights Reserved.                            **
 ***                                                            **
 ***        6145-F, Northbelt Parkway, Norcross,                **
 ***                                                            **
 ***        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 ***                                                            **
 *****************************************************************
 ******************************************************************
 *
 * FFConfig.c
 * Firmware firewall configuration.
 *
 *  Author: Basavaraj Astekar<basavarja@ami.com>
 *          Ravinder Reddy<bakkar@ami.com>
 ******************************************************************/
#define UNIMPLEMENTED_AS_FUNC
#include "FFConfig.h"
#include "IPMIDefs.h"
#include "App.h"
#include "Chassis.h"
#include "SensorEvent.h"
#include "Storage.h"
#include "DeviceConfig.h"
#include "NVRAccess.h"
#include "Session.h"
#include "PDKCmdsAccess.h"
#include "PDKCmds.h"
#include "cmdselect.h"

/*** Module Variables ***/
const FFSubFnTbl_T  m_FFSubFnTbl [] =
{/**** NetFn ********** Command ******************* IPMB **** SystemIfc **** LAN ***** Serial ***** ICMB ***** SMBUS ***** SecondaryIPMB ****  LAN1 ****  LAN2 ****  LAN3 ****  SMM ****/
    /*-------------------------------- APP Device Command's SubFn Configuration -----------------------------*/
    { NETFN_APP,    CMD_SET_WDT,                {0x0000000F, 0x0000000F, 0x0000000F, 0x0000000F, 0x0000000F, 0x0000000F, 0x0000000F, 0x0000000F, 0x0000000F, 0x0000000F, 0x0000000F} },
    { NETFN_APP,    CMD_SET_BMC_GBL_ENABLES,    {0x000000EF, 0x000000EF, 0x000000EF, 0x000000EF, 0x000000EF, 0x000000EF, 0x000000EF, 0x000000EF, 0x000000EF, 0x000000EF, 0x000000EF} },
    { NETFN_APP,    CMD_CLR_MSG_FLAGS,          {0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF} },
    { NETFN_APP,    CMD_ENBL_MSG_CH_RCV,        {0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF} },
    { NETFN_APP,    CMD_SEND_MSG,               {0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF} },
    { NETFN_APP,    CMD_CLOSE_SESSION,          {0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF} },
    { NETFN_APP,    CMD_SET_CH_ACCESS,          {0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF} },
    { NETFN_APP,    CMD_MASTER_WRITE_READ,      {0x000FFFFF, 0x000FFFFF, 0x000FFFFF, 0x000FFFFF, 0x000FFFFF, 0x000FFFFF, 0x000FFFFF, 0x000FFFFF, 0x000FFFFF, 0x000FFFFF, 0x000FFFFF} },

    /*------------------------------ Chassis Device Command's SubFn Configuration ----------------------------*/
    { NETFN_CHASSIS, CMD_CHASSIS_CONTROL,       {0x0000003F, 0x0000003F, 0x0000003F, 0x0000003F, 0x0000003F, 0x0000003F, 0x0000003F, 0x0000003F, 0x0000003F, 0x0000003F, 0x0000003F} },
    { NETFN_CHASSIS, CMD_CHASSIS_IDENTIFY,      {0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001} },
    { NETFN_CHASSIS, CMD_SET_SYSTEM_BOOT_OPTIONS, {0x00001FFF, 0x00001FFF, 0x00001FFF, 0x00001FFF, 0x00001FFF, 0x00001FFF, 0x00001FFF, 0x00001FFF, 0x00001FFF, 0x00001FFF, 0x00001FFF} },
    { NETFN_CHASSIS, CMD_SET_FP_BTN_ENABLES, {0x0000000F, 0x0000000F, 0x0000000F, 0x0000000F, 0x0000000F, 0x0000000F, 0x0000000F, 0x0000000F, 0x0000000F, 0x0000000F, 0x0000000F} },

    /*------------------------------ Sensor Device Command's SubFn Configuration ----------------------------*/
    { NETFN_SENSOR, CMD_ARM_PEF_POSTPONE_TIMER, {0x00000007, 0x00000007, 0x00000007, 0x00000007, 0x00000007, 0x00000007, 0x00000007, 0x00000007, 0x00000007, 0x00000007, 0x00000007} },
    { NETFN_SENSOR, CMD_SET_PEF_CONFIG_PARAMS,  {0x00003FFF, 0x00003FFF, 0x00003FFF, 0x00003FFF, 0x00003FFF, 0x00003FFF, 0x00003FFF, 0x00003FFF, 0x00003FFF, 0x00003FFF, 0x00003FFF} },
    { NETFN_SENSOR, CMD_ALERT_IMMEDIATE,        {0x00001FFF, 0x00001FFF, 0x00001FFF, 0x00001FFF, 0x00001FFF, 0x00001FFF, 0x00001FFF, 0x00001FFF, 0x00001FFF, 0x00001FFF, 0x00001FFF} },

    /*------------------------------ Storage Device Command's SubFn Configuration ---------------------------*/
    { NETFN_STORAGE, CMD_SET_AUXILIARY_LOG_STATUS, {0x00000007, 0x00000007, 0x00000007, 0x00000007, 0x00000007, 0x00000007, 0x00000007, 0x00000007, 0x00000007, 0x00000007, 0x00000007} },

    /*------------------------------ Transport Device Command's SubFn Configuration --------------------------*/
    { NETFN_TRANSPORT, CMD_SET_LAN_CONFIGURATION_PARAMETERS, {0x07FFFFFF, 0x07FFFFFF, 0x07FFFFFF, 0x07FFFFFF, 0x07FFFFFF, 0x07FFFFFF, 0x07FFFFFF, 0x07FFFFFF, 0x07FFFFFF, 0x07FFFFFF, 0x07FFFFFF} },
    { NETFN_TRANSPORT, CMD_SUSPEND_BMC_ARPS, {0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF} },
    { NETFN_TRANSPORT, CMD_SET_SERIAL_MODEM_CONFIG, {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF}, {0x00000007, 0x00000007, 0x00000007, 0x00000007, 0x00000007, 0x00000007, 0x00000007, 0x00000007, 0x00000007, 0x00000007, 0x00000007} },
    { NETFN_TRANSPORT, CMD_SET_SERIAL_MODEM_MUX,    {0x000001FF, 0x000001FF, 0x000001FF, 0x000001FF, 0x000001FF, 0x000001FF, 0x000001FF, 0x000001FF, 0x000001FF, 0x000001FF, 0x000001FF} },
    { NETFN_TRANSPORT, CMD_CALLBACK,				{0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF} },
    { NETFN_TRANSPORT, CMD_SET_USER_CALLBACK_OPTION,{0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF, 0x00000FFF} },
    { NETFN_TRANSPORT, CMD_SET_SOL_CONFIGURATION,{0x0003FFFF, 0x0003FFFF, 0x0003FFFF, 0x0003FFFF, 0x0003FFFF, 0x0003FFFF, 0x0003FFFF, 0x0003FFFF, 0x0003FFFF, 0x0003FFFF, 0x0003FFFF} },

};

//Group Extension codes
#define GROUPEXTNCODE_PICMG     0x00
#define GROUPEXTNCODE_DMTF      0x01
#define GROUPEXTNCODE_SSI       0x02
#define GROUPEXTNCODE_VSO       0x03
#define GROUPEXTNCODE_DCMI      0xDC
#define Max_GROUPEXTNCODE        5


/*------------------
 * CheckCmdCfg
 *------------------*/
int
CheckCmdCfg (CmdHndlrMap_T* pCmdHndlrMap, INT8U ChannelNum, 
                                INT8U NetFn, INT8U Cmd,int BMCInst)
{

    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    FFCmdConfigTbl_T* pFFCmdCfgTbl = &pBMCInfo->FFCmdConfigTbl[0];
    ChannelInfo_T*	pChInfo = getChannelInfo (ChannelNum, BMCInst);
    int i;
    INT8U   FFConfig, Index;

    if (NULL == pChInfo)
    {
        return 0;
    }
    Index = pChInfo->ChannelIndex;

    FFConfig = (pCmdHndlrMap->FFConfig >> (Index * 2)) & 0x03;

    /* Check suported or not */
    if (0 == (FFConfig & 0x02))
    {
        return 0;
    }

    /* Check the command is enabled or disabled */
    for (i = 0; i < MAX_FF_CMD_CFGS; i++)
    {
        if ((NetFn == pFFCmdCfgTbl [i].NetFn) && (Cmd == pFFCmdCfgTbl [i].Cmd))
        {
            return ((pFFCmdCfgTbl [i].Config >> Index) & 0x01);
        }
    }

    /* Check configurable or not
     * If not present in par package's f/w firewall configuration
     * then treated as non configurable
     */
    if (i == MAX_FF_CMD_CFGS )
    {
        return 1;
    }

    return 0;
}


/*------------------
 * GetCmdSupCfgMask
 *------------------*/
int
GetCmdSupCfgMask (INT8U NetFn, INT8U Cmd, INT16U* pFFConfig,INT8U GroupExtCode,int BMCInst)
{
    CmdHndlrMap_T*    pCmdHndlrMap;

    if (0 != GetMsgHndlrMap (NetFn, &pCmdHndlrMap,BMCInst))
    {
        if( 0 != GroupExtnGetMsgHndlrMap(NetFn,GroupExtCode, &pCmdHndlrMap, BMCInst))
        {
            if(0!=((int(*)(INT8U,CmdHndlrMap_T**,int))g_PDKCmdsHandle[PDKCMDS_GETOEMMSGHNDLRMAP])(NetFn,&pCmdHndlrMap,BMCInst))
            {
                return FF_NETFN_ERR;
            }
        }
    }

    while (pCmdHndlrMap->CmdHndlr)
    {
        if (pCmdHndlrMap->Cmd == Cmd)
        {
            /* Check if command has been implemented */
            if( 0 != GetCommandEnabledStatus(NetFn,&GroupExtCode, Cmd, BMCInst))
            {
                    *pFFConfig = 0x0000;
                }
                else
                {
                    *pFFConfig = pCmdHndlrMap->FFConfig;
            }
            return FF_SUCCESS;
        }
        pCmdHndlrMap++;
    }

    return FF_CMD_ERR;
}


/*------------------
 * GetCmdCfgAddr
 *------------------*/
INT8U*
GetCmdCfgAddr (INT8U NetFn, INT8U Cmd, int BMCInst)
{
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    FFCmdConfigTbl_T* pFFCmdCfgTbl = &pBMCInfo->FFCmdConfigTbl[0];
      int				i;

    /* Check the command is enabled or disabled */
    for (i = 0; i < MAX_FF_CMD_CFGS; i++)
    {
        if ((NetFn == pFFCmdCfgTbl [i].NetFn) && (Cmd == pFFCmdCfgTbl [i].Cmd))
        {
            return &pFFCmdCfgTbl [i].Config;
        }
    }

    return NULL;
}


/*------------------
 * GetSubFnMask
 *------------------*/
int
GetSubFnMask (INT8U ChannelNum, INT8U NetFn, INT8U Cmd, INT32U* pSubFnMask,int BMCInst)
{
    ChannelInfo_T*    pChInfo;
    int               i;

    pChInfo = getChannelInfo (ChannelNum, BMCInst);
    if (NULL == pChInfo) 
    {
        return FF_CHANNEL_ERR; 
    }

    for (i = 0; i < sizeof (m_FFSubFnTbl)/sizeof (FFSubFnTbl_T); i++)
    {
        if ((m_FFSubFnTbl [i].NetFn == NetFn) && (m_FFSubFnTbl [i].Cmd == Cmd))
        {
            *pSubFnMask = m_FFSubFnTbl [i].SubFn[pChInfo->ChannelIndex];
            return FF_SUCCESS;
        }
    }
    return FF_CMD_ERR;
}

/*------------------
 * GetSubFnMaskAdditional
 *------------------*/
int
GetSubFnMaskAdditional (INT8U ChannelNum, INT8U NetFn, INT8U Cmd, INT32U* pSubFnMask,int BMCInst)
{
    ChannelInfo_T*    pChInfo;
    int               i;

    pChInfo = getChannelInfo (ChannelNum, BMCInst);
    if (NULL == pChInfo) 
    {
        return FF_CHANNEL_ERR; 
    }

    for (i = 0; i < sizeof (m_FFSubFnTbl)/sizeof (FFSubFnTbl_T); i++)
    {
        if ((m_FFSubFnTbl [i].NetFn == NetFn) && (m_FFSubFnTbl [i].Cmd == Cmd))
        {
            *pSubFnMask = m_FFSubFnTbl [i].SubFnAdditional[pChInfo->ChannelIndex];
            return FF_SUCCESS;
        }
    }
    return FF_CMD_ERR;
}

