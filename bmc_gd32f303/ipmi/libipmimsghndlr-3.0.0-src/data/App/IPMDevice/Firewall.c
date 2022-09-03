/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2005-2006, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        6145-F, Northbelt Parkway, Norcross,                **
 **                                                            **
 **        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************/
/*****************************************************************
 *
 * Firewall.c
 * Firewall Commands Handler
 *
 * Author: Basavaraj Astekar <basavaraja@ami.com>
 *       : Bakka Ravinder Reddy <bakkar@ami.com>
 *
 *****************************************************************/
#define ENABLE_DEBUG_MACROS 0
#include "Types.h"
#include "Firewall.h"
#include "MsgHndlr.h"
#include "Support.h"
#include "IPMIDefs.h"
#include "IPMI_DeviceConfig.h"
#include "IPMI_Firewall.h"
#include "PMConfig.h"
#include "NVRAccess.h"
#include "Session.h"
#include "FFConfig.h"
#include "Util.h"
#include "PDKCmdsAccess.h"

/* Reserved bit macro definitions */
#define RESERVED_BITS_GETNETFNSUP                       0xF0 //(BIT7 | BIT6 | BIT5 |BIT4)

#define RESERVED_BITS_GETCMDSUP_CH                      0xF0 //(BIT7 | BIT6 | BIT5 |BIT4)
#define RESERVED_BITS_GETCMDSUP_LUN                     0xFC //(BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2)
#define RESERVED_BITS_GETCMDSUP_NETFUN					0x80 //BIT7 \BIT6 (11 and 10 are reserved as per IPMI spec)

#define RESERVED_BITS_GETSUBFNSUP_CH                    0xF0 //(BIT7 | BIT6 | BIT5 | BIT4)
#define RESERVED_BITS_GETSUBFNSUP_NETFN                 0xC0 //(BIT7 | BIT6)
#define RESERVED_BITS_GETSUBFNSUP_LUN                   0xFC //(BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2)

#define RESERVED_BITS_GETCFGCMDS_CH                     0xF0 //(BIT7 | BIT6 | BIT5 |BIT4)
#define RESERVED_BITS_GETCFGCMDS_LUN                    0xFC //(BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2)
#define RESERVED_BITS_GETCFGCMDS_NETFUN					0x80 //BIT7 \BIT6 (11 and 10 are reserved as per IPMI spec)

#define RESERVED_BITS_GETCFGSUBFNS_CH                   0xF0 //(BIT7 | BIT6 | BIT5 |BIT4)
#define RESERVED_BITS_GETCFGSUBFNS_NETFN                0xC0 //(BIT7 | BIT6)
#define RESERVED_BITS_GETCFGSUBFNS_LUN                  0xFC //(BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2)

#define RESERVED_BITS_SETCMDEN_CH                       0xF0 //(BIT7 | BIT6 | BIT5 |BIT4)
#define RESERVED_BITS_SETCMDEN_LUN                      0xFC //(BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2)
#define RESERVED_BITS_SETCMDEN_NETFUN                	0x80 //BIT7 \BIT6 (11 and 10 are reserved as per IPMI spec)

#define RESERVED_BITS_GETCMDEN_CH                       0xF0 //(BIT7 | BIT6 | BIT5 |BIT4)
#define RESERVED_BITS_GETCMDEN_LUN                      0xFC //(BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2)
#define RESERVED_BITS_GETCMDEN_NETFUN                	0x80 //BIT7 \BIT6 (11 and 10 are reserved as per IPMI spec)

#define RESERVED_BITS_SETSUBFNEN_CH                     0xF0 //(BIT7 | BIT6 | BIT5 | BIT4)
#define RESERVED_BITS_SETSUBFNEN_NETFN                  0xC0 //(BIT7 | BIT6)
#define RESERVED_BITS_SETSUBFNEN_LUN                    0xFC //(BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2)

#define RESERVED_BITS_GETSUBFNEN_CH                     0xF0 //(BIT7 | BIT6 | BIT5 | BIT4)
#define RESERVED_BITS_GETSUBFNEN_NETFN                  0xC0 //(BIT7 | BIT6)
#define RESERVED_BITS_GETSUBFNEN_LUN                    0xFC //(BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2)

#define RESERVED_BITS_GETOEMNETFNIANASUPPORT_CH         0xF0 //(BIT7 | BIT6 | BIT5 | BIT4)
#define RESERVED_BITS_GETOEMNETFNIANASUPPORT_NETFN      0xC0 //(BIT7 | BIT6)
#define RESERVED_BITS_GETOEMNETFNIANASUPPORT_LISTINDEX  0xC0 //(BIT7 | BIT6)
#if IPM_DEVICE == 1

static int GetRequiredLength (INT8U NetFn, INT8U CmdReqSize);
static int ValidateFFNetFnData (INT8U NetFn, NetFnParams_T* pNetFnParams ,BMCInfo_t *pBMCInfo);
/*** Local Macro definitions ***/
#define LUN_NO_CMD_SUPPORT          0x00
#define LUN_NO_RESTRICTION_SUPPORT  0x01
#define LUN_RESTRICTION_SUPPORT     0x02

#define LUN_00                      LUN_RESTRICTION_SUPPORT
#define LUN_01                      LUN_NO_CMD_SUPPORT << 2
#define LUN_10                      LUN_NO_CMD_SUPPORT << 4
#define LUN_11                      LUN_NO_CMD_SUPPORT << 6

#define DMTF_DEFINING_BODY    0x01

//Group Extension codes
#define GROUPEXTNCODE_PICMG     0x00
#define GROUPEXTNCODE_DMTF      0x01
#define GROUPEXTNCODE_SSI       0x02
#define GROUPEXTNCODE_VSO       0x03
#define GROUPEXTNCODE_DCMI      0xDC
#define Max_GROUPEXTNCODE        5


/* Add IANA No's to the Array */
IANA_T m_IANAList[] = {			/* LS byte first */
   {{0xFF,0xFF,0xFF}}, /* Reserved Oem IANA */
#ifdef IANA_OEM_LIST
   IANA_OEM_LIST
#endif   
};
INT8U GroupExtnCode[Max_GROUPEXTNCODE]={
    GROUPEXTNCODE_PICMG,
    GROUPEXTNCODE_DMTF,
    GROUPEXTNCODE_SSI,
    GROUPEXTNCODE_VSO,
    GROUPEXTNCODE_DCMI,
    };


#if GET_NETFN_SUP != UNIMPLEMENTED
/*---------------------------------------
 * GetNetFnSup
 *---------------------------------------*/
int
GetNetFnSup (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ GetNetFnSupRes_T*  pGetNetFnSupRes = (_NEAR_ GetNetFnSupRes_T*) pRes;
    _FAR_  CmdHndlrMap_T*     pCmdHndlrMap;
    _FAR_  ChannelInfo_T*     pChInfo;
    INT8U              Channel, i,j,curchannel;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    if(pBMCInfo->IpmiConfig.IPMIFirewallSupport == ENABLED)
    {
    	if(pReq[0] & RESERVED_BITS_GETNETFNSUP)
    	{
    		*pRes = CC_INV_DATA_FIELD;
    		return sizeof(*pRes);
    	}

        /* Check for channel number */
        Channel = pReq[0] & 0x0F;
        if (Channel == 0x0E)
        {
            OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
            Channel = curchannel & 0xF;
        }

        pChInfo = getChannelInfo (Channel, BMCInst);
        if (NULL == pChInfo)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof (*pRes);
        }

        pGetNetFnSupRes->CompletionCode = CC_NORMAL;
        pGetNetFnSupRes->LUNSupport     = LUN_00 | LUN_01 | LUN_10 | LUN_11;
        _fmemset (pGetNetFnSupRes->NetFnPair, 0, 16);

        for (i = 0; i < 32; i++)
        {
            if (0 != GetMsgHndlrMap (i*2, &pCmdHndlrMap,BMCInst))
            {
                for (j=0;j<Max_GROUPEXTNCODE;j++)
                {
                    if(0 != GroupExtnGetMsgHndlrMap ( i*2,GroupExtnCode[j],&pCmdHndlrMap,BMCInst))
                    {
                        continue;
                    }
                    else
                    {
                        pGetNetFnSupRes->NetFnPair [i / 8] |= 1 << (i % 8);
                    }
                }
                if(0!=((int(*)(INT8U,CmdHndlrMap_T**,int))g_PDKCmdsHandle[PDKCMDS_GETOEMMSGHNDLRMAP])(i*2,&pCmdHndlrMap,BMCInst))
                {
                continue;
                }
            }
            pGetNetFnSupRes->NetFnPair [i / 8] |= 1 << (i % 8);
        }
        return sizeof (GetNetFnSupRes_T);
    }
    else
    {
        *pRes = CC_INV_CMD;
        return sizeof(*pRes);
    }
}
#endif  /* GET_NETFN_SUP != UNIMPLEMENTED */


#if GET_CMD_SUP != UNIMPLEMENTED
/*---------------------------------------
 * GetCmdSup
 *---------------------------------------*/
int
GetCmdSup (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ GetCmdSupReq_T*  pGetCmdSupReq = (_NEAR_ GetCmdSupReq_T*) pReq;
    _NEAR_ GetCmdSupRes_T*  pGetCmdSupRes = (_NEAR_ GetCmdSupRes_T*) pRes;
    _FAR_  ChannelInfo_T*   pChInfo;
    INT8U            Channel, Start ,RetVal,curchannel;
    INT16U FFConfig;
    int  i;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    if(pBMCInfo->IpmiConfig.IPMIFirewallSupport == ENABLED)
    {
        if (GetRequiredLength((pGetCmdSupReq->OpNetFn & 0x3E), sizeof(GetCmdSupReq_T)) != ReqLen)
        {
            *pRes = CC_REQ_INV_LEN;
            return  sizeof (*pRes);
        }
     
        if(pGetCmdSupReq->ChannelNum & RESERVED_BITS_GETCMDSUP_CH)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }

        if(pGetCmdSupReq->LUN & RESERVED_BITS_GETCMDSUP_LUN)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }
        
        if(pGetCmdSupReq->OpNetFn & RESERVED_BITS_GETCMDSUP_NETFUN)
        {
        	*pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }


        if (ValidateFFNetFnData (pGetCmdSupReq->OpNetFn & 0x3E, &pGetCmdSupReq->NetFnParams,pBMCInfo))
        {
            *pRes = CC_INV_DATA_FIELD;
            return  sizeof (*pRes);
        }
        /* Check for channel number */
        Channel = pGetCmdSupReq->ChannelNum & 0x0F;
        if (Channel == 0x0E) 
        {
            OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
            Channel = curchannel & 0xF; 
        }

        pChInfo = getChannelInfo (Channel, BMCInst);

        pGetCmdSupRes->CompletionCode = CC_INV_DATA_FIELD;

        if ((NULL == pChInfo) || (0 != (pGetCmdSupReq->LUN & 0x03)))  
        {
            return sizeof (*pRes); 
        }

            switch (pGetCmdSupReq->OpNetFn >> 6)
        {
            case 0: Start = 0;     break;

            case 1: Start = 0x80;  break;

            default:
                return sizeof (*pRes);
        }

        _fmemset (pGetCmdSupRes->SupMask, 0xFF, 16);

        for (i = Start; i <= (Start + 0x7F); i++)
        {
            FFConfig = 0;

            RetVal = GetCmdSupCfgMask (pGetCmdSupReq->OpNetFn & 0x3E, i, &FFConfig,pGetCmdSupReq->NetFnParams.DefBodyCode,BMCInst);
            if (FF_NETFN_ERR == RetVal)
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(*pRes);
            }
            else if(FF_CMD_ERR == RetVal)
            {
                continue;
            }

            if ((FFConfig >> (pChInfo->ChannelIndex * 2 + 1)) & 0x01)
            {
                pGetCmdSupRes->SupMask[(i-Start)/8] &= ~( 1 << (i % 8));
            }
        }

        pGetCmdSupRes->CompletionCode = CC_NORMAL;
        return sizeof (GetCmdSupRes_T);
    }
    else
    {
        *pRes = CC_INV_CMD;
        return sizeof(*pRes);
    }
}
#endif  /* GET_CMD_SUP != UNIMPLEMENTED */


#if GET_SUBFN_SUP != UNIMPLEMENTED
/*---------------------------------------
 * GetSubFnSup
 *---------------------------------------*/
int
GetSubFnSup (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ GetSubFnSupReq_T*  pGetSubFnSupReq = (_NEAR_ GetSubFnSupReq_T*) pReq;
    _NEAR_ GetSubFnSupRes_T*  pGetSubFnSupRes = (_NEAR_ GetSubFnSupRes_T*) pRes;
    _FAR_  ChannelInfo_T*     pChInfo;
    INT8U              Channel,curchannel;
    INT32U             SubFnMask;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    if(pBMCInfo->IpmiConfig.IPMIFirewallSupport == ENABLED)
    {
        if (GetRequiredLength((pGetSubFnSupReq->NetFn & 0x3E), sizeof(GetSubFnSupReq_T)) != ReqLen)
        {
            *pRes = CC_REQ_INV_LEN;
            return  sizeof (*pRes);
        }

        if(pGetSubFnSupReq->ChannelNum & RESERVED_BITS_GETSUBFNSUP_CH)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }

        if(pGetSubFnSupReq->NetFn & RESERVED_BITS_GETSUBFNSUP_NETFN)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }

        if(pGetSubFnSupReq->LUN & RESERVED_BITS_GETSUBFNSUP_LUN)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }

         if (ValidateFFNetFnData (pGetSubFnSupReq->NetFn & 0x3E, &pGetSubFnSupReq->NetFnParams,pBMCInfo))
         {
             *pRes = CC_INV_DATA_FIELD;
             return  sizeof (*pRes);
         }

        /* Check for channel number */
        Channel = pGetSubFnSupReq->ChannelNum & 0x0F;
        if (Channel == 0x0E) 
        {
            OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel)
            Channel = curchannel & 0xF; 
        }
        
        pChInfo = getChannelInfo (Channel, BMCInst);

        pGetSubFnSupRes->CompletionCode = CC_INV_DATA_FIELD;

        if ((NULL == pChInfo) || (0 != (pGetSubFnSupReq->LUN & 0x03))) 
        { 
            return sizeof (*pRes); 
        }

        pGetSubFnSupRes->CompletionCode     = CC_NORMAL;
        pGetSubFnSupRes->SpecType           = 0x04;
        pGetSubFnSupRes->SpecVer            = 0x20;
        pGetSubFnSupRes->SpecRev            = 0x10;

        pGetSubFnSupRes->SupMask = 0xFFFFFFFF;

        if (FF_SUCCESS == GetSubFnMask (Channel, pGetSubFnSupReq->NetFn, pGetSubFnSupReq->Cmd, &SubFnMask,BMCInst))
        {
            pGetSubFnSupRes->SupMask = ~(htoipmi_u32(SubFnMask));
        }

        if ((pGetSubFnSupReq->NetFn == NETFN_TRANSPORT) && (pGetSubFnSupReq->Cmd == CMD_SET_SERIAL_MODEM_CONFIG))
        {
            if (FF_SUCCESS == GetSubFnMaskAdditional(Channel, pGetSubFnSupReq->NetFn, pGetSubFnSupReq->Cmd, &SubFnMask,BMCInst))
            {
                pGetSubFnSupRes->SupMaskAdditional = ~(htoipmi_u32(SubFnMask));
            }
        return sizeof (GetSubFnSupRes_T);
        }

        return (sizeof (GetSubFnSupRes_T) - 4);
    }
    else
    {
        *pRes = CC_INV_CMD;
        return sizeof(*pRes);
    }
}
#endif  /* GET_SUBFN_SUP != UNIMPLEMENTED */


#if GET_CONFIG_CMDS != UNIMPLEMENTED
/*---------------------------------------
 * GetConfigCmds
 *---------------------------------------*/
int
GetConfigCmds (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ GetConfigCmdsReq_T*  pGetConfigCmdsReq = (_NEAR_ GetConfigCmdsReq_T*) pReq;
    _NEAR_ GetConfigCmdsRes_T*  pGetConfigCmdsRes = (_NEAR_ GetConfigCmdsRes_T*) pRes;
    _FAR_  ChannelInfo_T*       pChInfo;
    INT8U                Channel, Start , RetVal,curchannel;
    INT16U FFConfig;
    int  i;
    _FAR_   INT8U* pCmdCfg;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    if(pBMCInfo->IpmiConfig.IPMIFirewallSupport == ENABLED)
    {
        if (GetRequiredLength((pGetConfigCmdsReq->OpNetFn & 0x3E), sizeof(GetConfigCmdsReq_T)) != ReqLen)
        {
            *pRes = CC_REQ_INV_LEN;
            return  sizeof (*pRes);
        }
        
        if(pGetConfigCmdsReq->ChannelNum & RESERVED_BITS_GETCFGCMDS_CH)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }

        if(pGetConfigCmdsReq->LUN & RESERVED_BITS_GETCFGCMDS_LUN)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }
        
        if (pGetConfigCmdsReq->OpNetFn & RESERVED_BITS_GETCFGCMDS_NETFUN)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }

        if (ValidateFFNetFnData (pGetConfigCmdsReq->OpNetFn & 0x3E, &pGetConfigCmdsReq->NetFnParams,pBMCInfo))
        {
            *pRes = CC_INV_DATA_FIELD;
            return  sizeof (*pRes);
        }

        /* Check for channel number */
        Channel = pGetConfigCmdsReq->ChannelNum & 0x0F;
        if (Channel == 0x0E) 
        {
            OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
            Channel = curchannel & 0xF; 
        }

        pChInfo = getChannelInfo (Channel, BMCInst); 

        pGetConfigCmdsRes->CompletionCode = CC_INV_DATA_FIELD;

        if ((NULL == pChInfo) || (0 != (pGetConfigCmdsReq->LUN & 0x03))) 
        { 
            return sizeof (*pRes); 
        }

        switch (pGetConfigCmdsReq->OpNetFn >> 6)
        {
            case 0: Start = 0; break;
            case 1: Start = 0x80; break;
            default:
                return sizeof (*pRes);
        }

        _fmemset (pGetConfigCmdsRes->SupMask, 0, 16);

        for (i = Start; i <= (Start + 0x7F); i++)
        {
            FFConfig = 0;

            RetVal = GetCmdSupCfgMask (pGetConfigCmdsReq->OpNetFn & 0x3E, i, &FFConfig,pGetConfigCmdsReq->NetFnParams.DefBodyCode,BMCInst);
            if (FF_NETFN_ERR == RetVal)
            { 
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(*pRes);
            }
            else if(FF_CMD_ERR == RetVal)
            {
               continue;
            }

            /* If supported but not configurable 
             * if not present in par file f/w Fire wall configuration entries
             * Then treated as non configurable */
            pCmdCfg = GetCmdCfgAddr (pGetConfigCmdsReq->OpNetFn & 0x3E, i, BMCInst);
            if ( pCmdCfg != NULL)
            {
                pGetConfigCmdsRes->SupMask[(i-Start)/8] |= (1 << (i % 8));
            }
        }

        pGetConfigCmdsRes->CompletionCode = CC_NORMAL;

        return sizeof (GetConfigCmdsRes_T);
    }
    else
    {
        *pRes = CC_INV_CMD;
        return sizeof(*pRes);
    }
}
#endif  /* GET_CONFIG_CMDS != UNIMPLEMENTED */


#if GET_CONFIG_SUB_FNS != UNIMPLEMENTED
/*---------------------------------------
 * GetConfigSubFns
 *---------------------------------------*/
int
GetConfigSubFns (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ GetConfigSubFnsReq_T*  pGetConfigSubFnsReq = (_NEAR_ GetConfigSubFnsReq_T*) pReq;
    _NEAR_ GetConfigSubFnsRes_T*  pGetConfigSubFnsRes = (_NEAR_ GetConfigSubFnsRes_T*) pRes;
    _FAR_  ChannelInfo_T*         pChInfo;
    INT8U                  Channel,curchannel;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
	INT32U             SubFnMask;

    if(pBMCInfo->IpmiConfig.IPMIFirewallSupport == ENABLED)
    {
        if (GetRequiredLength((pGetConfigSubFnsReq->NetFn & 0x3E), sizeof(GetConfigSubFnsReq_T)) != ReqLen)
        {
            *pRes = CC_REQ_INV_LEN;
            return  sizeof (*pRes);
        }
        
        if(pGetConfigSubFnsReq->ChannelNum & RESERVED_BITS_GETCFGSUBFNS_CH)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }

        if(pGetConfigSubFnsReq->NetFn & RESERVED_BITS_GETCFGSUBFNS_NETFN)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }

        if(pGetConfigSubFnsReq->LUN & RESERVED_BITS_GETCFGSUBFNS_LUN)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }

        if (ValidateFFNetFnData (pGetConfigSubFnsReq->NetFn & 0x3E, &pGetConfigSubFnsReq->NetFnParams ,pBMCInfo))
        {
            *pRes = CC_INV_DATA_FIELD;
            return  sizeof (*pRes);
        }

        /* Check for channel number */
        Channel = pGetConfigSubFnsReq->ChannelNum & 0x0F;
        if (Channel == 0x0E)
        {
            OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
            Channel = curchannel & 0xF; 
        }

        pChInfo = getChannelInfo (Channel, BMCInst);
        pGetConfigSubFnsRes->CompletionCode = CC_INV_DATA_FIELD;

        if ((NULL == pChInfo) || (0 != (pGetConfigSubFnsReq->LUN & 0x03))) 
        { 
            return sizeof (*pRes); 
        }

        pGetConfigSubFnsRes->CompletionCode = CC_NORMAL;
        pGetConfigSubFnsRes->SupMask        = 0;

        if (FF_SUCCESS == GetSubFnMask (Channel, pGetConfigSubFnsReq->NetFn, pGetConfigSubFnsReq->Cmd, &SubFnMask,BMCInst))
        {
            pGetConfigSubFnsRes->SupMask = htoipmi_u32(SubFnMask);
        }

        if ((pGetConfigSubFnsReq->NetFn == NETFN_TRANSPORT) && (pGetConfigSubFnsReq->Cmd == CMD_SET_SERIAL_MODEM_CONFIG))
        {
            if (FF_SUCCESS == GetSubFnMaskAdditional(Channel, pGetConfigSubFnsReq->NetFn, pGetConfigSubFnsReq->Cmd, &SubFnMask,BMCInst))
            {
                pGetConfigSubFnsRes->SupMaskAdditional = htoipmi_u32(SubFnMask);
            }
        return sizeof (GetConfigSubFnsRes_T);
        }

        return (sizeof (GetConfigSubFnsRes_T) - 4);
    }
    else
    {
        *pRes = CC_INV_CMD;
        return sizeof(*pRes);
    }
}
#endif  /* GET_CONFIG_SUB_FNS != UNIMPLEMENTED */


#if SET_CMD_ENABLES != UNIMPLEMENTED
/*---------------------------------------
 * SetCmdEnables
 *---------------------------------------*/
int
SetCmdEnables (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ SetCmdEnablesReq_T*  pSetCmdEnablesReq = (_NEAR_ SetCmdEnablesReq_T*) pReq;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    _FAR_  ChannelInfo_T*       pChInfo;
    INT8U                Channel, Start, RetVal;
    int                  i;
    INT8U                SuccessFlag = 0x01,curchannel;
    INT16U FFConfig;
    _FAR_   INT8U* pCmdCfg;
    INT8U  EnDisMask;


    if(pBMCInfo->IpmiConfig.IPMIFirewallSupport == ENABLED)
    {
        if (GetRequiredLength((pSetCmdEnablesReq->OpNetFn & 0x3E), sizeof(SetCmdEnablesReq_T)) != ReqLen)
        {
            *pRes = CC_REQ_INV_LEN;
            return  sizeof (*pRes);
        }

        *pRes = CC_INV_DATA_FIELD;
        if(pSetCmdEnablesReq->ChannelNum & RESERVED_BITS_SETCMDEN_CH) 
        {
            return sizeof(*pRes);
        }
        if(pSetCmdEnablesReq->LUN & RESERVED_BITS_SETCMDEN_LUN)
        {
            return sizeof(*pRes);
        }
        if(pSetCmdEnablesReq->OpNetFn & RESERVED_BITS_SETCMDEN_NETFUN) //As per IPMI SPEC we should block the 7 and 6 bit by this combination 11 ans 10//
        {
            return sizeof(*pRes);
        }

        if (ValidateFFNetFnData (pSetCmdEnablesReq->OpNetFn & 0x3E, &pSetCmdEnablesReq->NetFnParams , pBMCInfo))
        {
            *pRes = CC_INV_DATA_FIELD;
            return  sizeof (*pRes);
        }

        /* Check for channel number */
        Channel = pSetCmdEnablesReq->ChannelNum & 0x0F;
        if (Channel == 0x0E) 
        {
            OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
            Channel = curchannel & 0xF; 
        }
        
        pChInfo = getChannelInfo (Channel, BMCInst); 

        if ((NULL == pChInfo) || (0 != (pSetCmdEnablesReq->LUN & 0x03))) 
        {
            return sizeof (*pRes); 
        }

        switch (pSetCmdEnablesReq->OpNetFn >> 6)
        {
            case 0: Start = 0; break;
            case 1: Start = 0x80; break;
            default:
                return sizeof (*pRes);
        }

        for (i = Start; i <= (Start + 0x7F); i++)
        {
            RetVal = GetCmdSupCfgMask (pSetCmdEnablesReq->OpNetFn & 0x3E, i, &FFConfig,pSetCmdEnablesReq->NetFnParams.DefBodyCode,BMCInst);
            if (FF_NETFN_ERR == RetVal)
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(*pRes);
            }
            else if(FF_CMD_ERR == RetVal)
            {
                FFConfig = 0x0000; 
            }

            FFConfig >>= pChInfo->ChannelIndex * 2;

            EnDisMask = (pSetCmdEnablesReq->EnDisMask [(i-Start)/8] >> (i % 8)) & 0x01;

            //if disabled statically. i.e not supported
            if (0 == (FFConfig & 0x02))
            {
                if (1 == EnDisMask )
                {
                    SuccessFlag = 0;
                    break;
                }
                else
                {
                    continue;
                }
            }

            pCmdCfg = GetCmdCfgAddr (pSetCmdEnablesReq->OpNetFn & 0x3E, i, BMCInst);
            /* If supported but not configurable 
             * if not present in par file f/w Fire wall configuration entries
             * Then treated as non configurable */
            if (NULL == pCmdCfg) 
            {
                //if supported but non configurable then set value always have to 1
                if (0 == EnDisMask )
                {
                    SuccessFlag = 0;
                    break;
                }
                else
                {
                    continue;
                }
            }

            *pCmdCfg &= ~(1 << pChInfo->ChannelIndex);
            *pCmdCfg |=  EnDisMask << pChInfo->ChannelIndex;
            FlushIPMI((INT8U*)&pBMCInfo->FFCmdConfigTbl[0],pCmdCfg,pBMCInfo->IPMIConfLoc.FFCmdConfigTblAddr,
                              sizeof(INT8U),BMCInst);
            SuccessFlag = 1;
        }

        if (0 == SuccessFlag)
        {
            *pRes = CC_CMD_UNSUPPORTED_UNCONFIGURABLE;
        }
        else
        {
            *pRes = CC_NORMAL;
        }
        return sizeof (*pRes);
    }
    else
    {
        *pRes = CC_INV_CMD;
        return sizeof(*pRes);
    }
}
#endif  /* SET_CMD_ENABLES != UNIMPLEMENTED */


#if GET_CMD_ENABLES != UNIMPLEMENTED
/*---------------------------------------
 * GetCmdEnables
 *---------------------------------------*/
int
GetCmdEnables (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ GetCmdEnablesReq_T*  pGetCmdEnablesReq = (_NEAR_ GetCmdEnablesReq_T*) pReq;
    _NEAR_ GetCmdEnablesRes_T*  pGetCmdEnablesRes = (_NEAR_ GetCmdEnablesRes_T*) pRes;
    _FAR_  ChannelInfo_T*       pChInfo;
    INT8U                Channel, Start,RetVal;
    INT16U FFConfig;
    _FAR_   INT8U* pCmdCfg;
    INT8U  ResetFlag = 0,curchannel;
    int  i;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    if(pBMCInfo->IpmiConfig.IPMIFirewallSupport == ENABLED)
    {
        if (GetRequiredLength((pGetCmdEnablesReq->OpNetFn & 0x3E), sizeof(GetCmdEnablesReq_T)) != ReqLen)
        {
            *pRes = CC_REQ_INV_LEN;
            return  sizeof (*pRes);
        }
     
        if(pGetCmdEnablesReq->ChannelNum & RESERVED_BITS_GETCMDEN_CH)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }

        if(pGetCmdEnablesReq->LUN & RESERVED_BITS_GETCMDEN_LUN)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }
        
        if(pGetCmdEnablesReq->OpNetFn & RESERVED_BITS_GETCMDEN_NETFUN)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }

        if (ValidateFFNetFnData (pGetCmdEnablesReq->OpNetFn & 0x3E, &pGetCmdEnablesReq->NetFnParams ,pBMCInfo))
        {
            *pRes = CC_INV_DATA_FIELD;
            return  sizeof (*pRes);
        }

        /* Check for channel number */
        Channel = pGetCmdEnablesReq->ChannelNum & 0x0F;
        if (Channel == 0x0E)
        {
            OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
            Channel = curchannel & 0xF; 
        }

        pChInfo = getChannelInfo (Channel, BMCInst);
        pGetCmdEnablesRes->CompletionCode = CC_INV_DATA_FIELD;

        if ((NULL == pChInfo) || (0 != (pGetCmdEnablesReq->LUN & 0x03))) 
        { 
            return sizeof (*pRes); 
        }

        switch (pGetCmdEnablesReq->OpNetFn >> 6)
        {
            case 0: Start = 0; break;
            case 1: Start = 0x80; break;
            default:
                return sizeof (*pRes);
        }

        _fmemset (pGetCmdEnablesRes->EnDisMask, 0xFF, 16);

        /* set the bit if command is enabled else reset */
        for (i = Start; i <= (Start + 0x7F); i++)
        {
            FFConfig = 0;

            RetVal = GetCmdSupCfgMask (pGetCmdEnablesReq->OpNetFn & 0x3E, i, &FFConfig,pGetCmdEnablesReq->NetFnParams.DefBodyCode,BMCInst);
            if(FF_NETFN_ERR == RetVal)
            {
                 *pRes = CC_INV_DATA_FIELD;
                 return sizeof(*pRes);
            }
            else if (FF_CMD_ERR == RetVal)
            {
                ResetFlag = 1; 
            }
            else
            {
                FFConfig >>= pChInfo->ChannelIndex * 2; 
                /* if not supported */
                if (0 == (FFConfig & 0x02)) 
                {
                    ResetFlag = 1; 
                }
                else
                {
                    pCmdCfg = GetCmdCfgAddr (pGetCmdEnablesReq->OpNetFn & 0x3E, i, BMCInst);
                    /* if supported check if configurable
                     * if not present in par file f/w Fire wall configuration entries
                     * Then treated as non configurable 
                     * if supported and configurable check if disabled */
                    if ( NULL != pCmdCfg )
                    {
                        if( 0 == ((*pCmdCfg >> pChInfo->ChannelIndex) & 0x01) )
                        {
                            ResetFlag = 1; 
                        }
                    }
                }
            }

            if (ResetFlag == 1)
            {
                pGetCmdEnablesRes->EnDisMask[(i-Start)/8] &= ~(1 << (i % 8)); 
                ResetFlag = 0;
            }
        }

        pGetCmdEnablesRes->CompletionCode = CC_NORMAL;

        return (sizeof (GetCmdEnablesRes_T) - sizeof (NetFnParams_T));
    }
    else
    {
        *pRes = CC_INV_CMD;
        return sizeof(*pRes);
    }
}
#endif  /* GET_CMD_ENABLES != UNIMPLEMENTED */


#if SET_SUBFN_ENABLES != UNIMPLEMENTED
/*---------------------------------------
 * SetSubFnEnables
 *---------------------------------------*/
int
SetSubFnEnables (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ SetSubFnEnablesReq_T*  pSetSubFnEnablesReq = (_NEAR_ SetSubFnEnablesReq_T*) pReq;
    _FAR_  ChannelInfo_T*         pChInfo;
    INT8U                  Channel,curchannel;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    if(pBMCInfo->IpmiConfig.IPMIFirewallSupport == ENABLED)
    {
        if(pSetSubFnEnablesReq->ChannelNum & RESERVED_BITS_SETSUBFNEN_CH)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }

        if(pSetSubFnEnablesReq->NetFn & RESERVED_BITS_SETSUBFNEN_NETFN)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }

        if(pSetSubFnEnablesReq->LUN & RESERVED_BITS_SETSUBFNEN_LUN)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }

        /* Check for channel number */
        Channel = pSetSubFnEnablesReq->ChannelNum & 0x0F;
        if (Channel == 0x0E)
        {
            OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
            Channel = curchannel & 0xF; 
        }
        
        pChInfo = getChannelInfo (Channel, BMCInst);
        
        *pRes = CC_INV_DATA_FIELD;

        if ((NULL == pChInfo) || (0 != (pSetSubFnEnablesReq->LUN & 0x03))) 
        {
            return sizeof (*pRes); 
        }

        *pRes = CC_PARAM_NOT_SUPPORTED;
        return sizeof (*pRes);
    }
    else
    {
        *pRes = CC_INV_CMD;
        return sizeof(*pRes);
    }
}
#endif  /* SET_SUBFN_ENABLES != UNIMPLEMENTED */


#if GET_SUBFN_ENABLES != UNIMPLEMENTED
/*---------------------------------------
 * GetSubFnEnables
 *---------------------------------------*/
int
GetSubFnEnables (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ GetSubFnEnablesReq_T*  pGetSubFnEnablesReq = (_NEAR_ GetSubFnEnablesReq_T*) pReq;
    _NEAR_ GetSubFnEnablesRes_T*  pGetSubFnEnablesRes = (_NEAR_ GetSubFnEnablesRes_T*) pRes;
    _FAR_  ChannelInfo_T*         pChInfo;
    INT8U                  Channel,curchannel;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    if(pBMCInfo->IpmiConfig.IPMIFirewallSupport == ENABLED)
    {
        if(pGetSubFnEnablesReq->ChannelNum & RESERVED_BITS_GETSUBFNEN_CH)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }

        if(pGetSubFnEnablesReq->NetFn & RESERVED_BITS_GETSUBFNEN_NETFN)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }

        if(pGetSubFnEnablesReq->LUN & RESERVED_BITS_GETSUBFNEN_LUN)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }

        /* Check for channel number */
        Channel = pGetSubFnEnablesReq->ChannelNum & 0x0F;
        if (Channel == 0x0E)
        {
            OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
            Channel = curchannel & 0xF;
        }
        
        pChInfo = getChannelInfo (Channel, BMCInst);
        
        *pRes = CC_INV_DATA_FIELD;

        if ((NULL == pChInfo) || (0 != (pGetSubFnEnablesReq->LUN & 0x03)))
        {
            return sizeof (*pRes); 
        }

        pGetSubFnEnablesRes->Enables        = 0;
        pGetSubFnEnablesRes->CompletionCode = CC_NORMAL;
        return sizeof (GetSubFnEnablesRes_T);
    }
    else
    {
        *pRes = CC_INV_CMD;
        return sizeof(*pRes);
    }
}

#endif  /* GET_SUBFN_ENABLES != UNIMPLEMENTED */

#if GET_OEM_NETFN_IANA_SUPPORT != UNIMPLEMENTED
/*---------------------------------------
 * GetOEMNetFnIANASupport
 *---------------------------------------*/
int
GetOEMNetFnIANASupport (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ GetOEMNetFnIANASupportReq_T*  pGetOEMNetFnIANASupportReq = (_NEAR_ GetOEMNetFnIANASupportReq_T*) pReq;
    _NEAR_ GetOEMNetFnIANASupportRes_T*  pGetOEMNetFnIANASupportRes = (_NEAR_ GetOEMNetFnIANASupportRes_T*) pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    INT8U i;
    if(pBMCInfo->IpmiConfig.IPMIFirewallSupport == ENABLED)
    {
        _fmemset(pGetOEMNetFnIANASupportRes, 0x00, sizeof(GetOEMNetFnIANASupportRes_T));

        if(pGetOEMNetFnIANASupportReq->ChannelNum & RESERVED_BITS_GETOEMNETFNIANASUPPORT_CH)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }

        if(pGetOEMNetFnIANASupportReq->NetFn & RESERVED_BITS_GETOEMNETFNIANASUPPORT_NETFN)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }

        if(pGetOEMNetFnIANASupportReq->ListIndex & RESERVED_BITS_GETOEMNETFNIANASUPPORT_LISTINDEX)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }

        pGetOEMNetFnIANASupportRes->CompletionCode = CC_NORMAL;

        pGetOEMNetFnIANASupportRes->LUNSupport = LUN_00 | LUN_01 |LUN_10 | LUN_11;

        if(NETFN_AMI == pGetOEMNetFnIANASupportReq->NetFn)
        {
            pGetOEMNetFnIANASupportRes->CompletionCode = CC_NORMAL; 
            pGetOEMNetFnIANASupportRes->IANACode[0] = (INT8U) DMTF_DEFINING_BODY;
            return (sizeof(GetOEMNetFnIANASupportRes_T) -2);
        }
        else if(NETFN_PICMG == pGetOEMNetFnIANASupportReq->NetFn )
        {
            for (i = 0; i < sizeof (pBMCInfo->GroupExtnMsgHndlrTbl) / sizeof (pBMCInfo->GroupExtnMsgHndlrTbl [0]); i++)
            {
                if (pBMCInfo->GroupExtnMsgHndlrTbl [i].NetFn == pGetOEMNetFnIANASupportReq->NetFn) 
                {
                	pGetOEMNetFnIANASupportRes->IANACode[0] = pBMCInfo->GroupExtnMsgHndlrTbl [i].GroupExtnCode;
                	return (sizeof(GetOEMNetFnIANASupportRes_T) -2);
                }
            }
        }
        else if (NETFN_OEM == pGetOEMNetFnIANASupportReq->NetFn)
        {
        	if (pGetOEMNetFnIANASupportReq->ListIndex  < (sizeof(m_IANAList)/ sizeof(m_IANAList[0])))
        	{
        		_fmemcpy(pGetOEMNetFnIANASupportRes->IANACode, m_IANAList[pGetOEMNetFnIANASupportReq->ListIndex].IANA, sizeof(IANA_T));
        		if ( (pGetOEMNetFnIANASupportReq->ListIndex+1) == (sizeof(m_IANAList)/ sizeof(m_IANAList[0])))
        		{
        			pGetOEMNetFnIANASupportRes->IANAIndex = 1 << 7;
        		}
        		return (sizeof(GetOEMNetFnIANASupportRes_T));
            }
        }
        pGetOEMNetFnIANASupportRes->CompletionCode = CC_INV_DATA_FIELD;
        return(sizeof(pGetOEMNetFnIANASupportRes->CompletionCode));
    }
    else
    {
        *pRes = CC_INV_CMD;
        return sizeof(*pRes);
    }
}

#endif	//GET_OEM_NETFN_IANA_SUPPORT != UNIMPLEMENTED

static int GetRequiredLength(INT8U NetFn, INT8U CmdReqSize)
{
    INT8U    RequiredLength;

    switch (NetFn)
    {
        case NETFN_OEM:
            RequiredLength = CmdReqSize;
            break;
        case NETFN_AMI:
            RequiredLength = CmdReqSize - sizeof(NetFnParams_T) + 1;
            break;
        case NETFN_PICMG:
            RequiredLength = CmdReqSize - sizeof(NetFnParams_T) + 1;
            break;
        default:
            RequiredLength = CmdReqSize - sizeof(NetFnParams_T);
            break;
    }

    return RequiredLength;
}

static int ValidateFFNetFnData (INT8U NetFn, NetFnParams_T* pNetFnParams , BMCInfo_t *pBMCInfo)
{
    INT8U i;
    
    // If NetFn is 0x2C, check for Defining Body code
    if (NETFN_PICMG == NetFn )
    {
        for (i = 0; i < sizeof (pBMCInfo->GroupExtnMsgHndlrTbl) / sizeof (pBMCInfo->GroupExtnMsgHndlrTbl [0]); i++)
        {
            if (pBMCInfo->GroupExtnMsgHndlrTbl [i].NetFn == NetFn) 
                return (pBMCInfo->GroupExtnMsgHndlrTbl [i].GroupExtnCode != pNetFnParams->DefBodyCode);
        }
    }
    else if (NETFN_OEM == NetFn) 	// If NetFn is 0x2E, check for OEM IANA
    {
        for (i=0; i <(sizeof(m_IANAList)/ sizeof(m_IANAList[0])); i++)
        {
            if (0 == _fmemcmp( pNetFnParams->IANA, m_IANAList[i].IANA, sizeof(IANA_T)))
            {
                break;
            }
        }

        if (i >= (sizeof(m_IANAList)/ sizeof(m_IANAList[0])))
        {
            // Given IANA in Req is not matching any of the IANA in the IANA list
            return  1;
        }
    }
    return 0;
}

#endif  /* IPM_DEVICE */
