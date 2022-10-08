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
 * LANAlert.h
 * SNMP Trap generation
 *
 * Author: Govind Kothandapani <govindk@ami.com> 
 *       : Rama Bisa <ramab@ami.com>
 *       : Basavaraj Astekar <basavaraja@ami.com>
 *       : Bakka Ravinder Reddy <bakkar@ami.com>
 * 
 *****************************************************************/
#ifndef LANALERT_H
#define LANALERT_H

#include "Types.h"
#include "SELRecord.h"

#pragma pack( 1 )

#define MAX_EVENT_DATA                  0x8
#define MAX_OEM_TRAP_FIELDS             64

/*** Macro Definitions ***/
#define PET_TRAP_DEST_TYPE              0x07
#define TIMESTAMP_1970_TO_1998          883612800
#define EVENT_SOURCE_IPMI               0x20
#define PEF_PET_GUID_USE_MASK           0x01

#define MAX_ENCODED_LEN_SIZE            5
#define MAX_ENCODED_VER_SIZE            5
#define MAX_ENCODED_COM_STR_SIZE        25
#define MAX_ENCODED_TRAP_PDU_SIZE       225
#define MAX_ENCODED_AGENT_ADDR_SIZE     10
#define MAX_ENCODED_GEN_TRAP_SIZE       20
#define MAX_ENCODED_SPEC_TRAP_SIZE      20
#define MAX_ENCODED_TIME_STAMP_SIZE     10
#define MAX_ENCODED_VAR_BIND_SIZE       127


#define UTC_OFFSET_UNSPECIFIED          0xFFFF
#define EVT_SEV_UNSPECIFIED             0x00
#define TRAP_SOURCE_IPMI                0x20
#define SENSOR_DEV_BMC                  0x20
#define SENSOR_NO_UNSPECIFIED           0x00
#define ENTITY_UNSPECIFIED              0x00
#define ENTITY_INSTANCE_UNSPEC          0x00
#define LANGUAGE_CODE_ENGLISH           0x19
#define MANUFACTURER_ID_PET             0x00
#define SYSTEM_ID                       0x00

#define MAX_ENCODE_OID_BUFFER_SIZE      6
#define MAX_ENCODED_OID_SIZE            15
#define MAX_BYTE_DATA                   0xFF
#define MAX_UINT16_DATA                 0xFFFF
#define MAX_UINT24_DATA                 0xFFFFFF

#define HI_UINT32_MASK                  0xFF000000
#define HI_UINT24_MASK                  0xFF0000
#define HI_BYTE_MASK                    0xFF00
#define LO_BYTE_MASK                    0xFF
#define BIT8_SET_MASK                   0x80
#define LO_NIBBLE_MASK                  0x0f

#define HI_UINT32_SHIFT                 24
#define HI_UINT24_SHIFT                 16
#define HI_BYTE_SHIFT                   8
#define BYTE_SHIFT                      8

#define ASN_OID_MULTIPLIER1             40
#define ASN_OID_MULTIPLIER2             128
#define ASN_ID_INTEGER                  0x02
#define ASN_ID_STRING                   0x04
#define ASN_ID_SEQUENCE                 0x30
#define ASN_ID_OBJECT                   0x06
#define ASN_ID_IP_ADDR                  0x40
#define ASN_ID_TRAP_PDU                 0xa4
#define ASN_ID_TIMESTAMP                0x43
#define TIMESTAMPMULTIPLE               100

#define ONE_DATA_ENCODED_ID             0x81
#define TWO_DATA_ENCODED_ID             0x82
#define MAX_LEN_ENCODING_SIZE           5

#define ENTERPRISE_SPECIFIC_GENERIC_TRAP 0x6

#define MAX_AGENT_ADDR_SIZE             4

#define	SNMP_UDP_PORT	                162
#define MAX_SNMP_TRAP_SIZE              256
#define LAN_DEST_TYPE_MASK  0x7
#define LAN_PET_TRAP	    0x0
#define LAN_OEM1_ALERT      0x06
#define LAN_OEM2_ALERT      0x07
#define BACKUP_GATEWAY     0X01


/*** Type Definitions ***/
/**
 * @struct VarBindings_T
 * @brief Structure for the SNMP Trap varible bindings
 **/
typedef struct
{
    INT8U   GUid[16];           /**< Global unique ID */
    INT16U  SeqCookie;          /**< Sequence Cookie */
    INT32U  TimeStamp;          /**< Platform Local Time Stamp */
    INT16U  UTCOffset;          /**< UTC offset in minutes */
    INT8U   TrapSourceType;     /**< Trap Source Type */
    INT8U   EventSourceType;    /**< Event Source Type */
    INT8U   EventSeverity;      /**< Event Severity */
    INT8U   SensorDevice;       /**< Identifies instance of a Device */
    INT8U   SensorNumber;       /**< Identifies instance of a Sensor */
    INT8U   Entity;             /**< Platform Entity ID */
    INT8U   EntityInstance;     /**< Entity instance */
    INT8U   EvtData[MAX_EVENT_DATA]; /**< Maxmimum event data */
    INT8U   LanguageCode;       /**<Language code */
    INT32U  ManufacturerID;     /**< Manufacturer ID per IANA.*/
    INT16U  SystemID;           /**< System ID */
    INT8U   Oem[MAX_OEM_TRAP_FIELDS]; /**< Oem specific trap fields */

}   PACKED VarBindings_T;


typedef struct
{
    INT8U Type;
    INT8U  Length;
    INT8U RecType;
    INT8U  RecData[61];
}PACKED OemPETField_T;

/*** Type Definitions ***/

/**
 * @struct ASNHeader_T
 * @brief Structure for the ASN header in SNMP Trap
 **/
typedef struct
{
    INT8U Identifier;    /**< Identifier */
    INT8U Length;        /**< Length of the Variable bindings */

} PACKED ASNHeader_T;


#pragma pack ()



/*** Extern Declarations ***/

/*** Function Prototypes ***/

/**
 * @brief   Generates SNMP Trap through LAN
 * @param   EvtRecord Pointer to Event record
 * @param   DestSel Destination selector
 * @param   AlertImmFlag Alert Immediate flag
 * @param   Event Severity 
 * @return  TRUE if success else FALSE
 **/
extern INT8U LANAlert (SELEventRecord_T*   EvtRecord,
                               INT8U               DestSel,
                               INT8U               EventSeverity,
                               INT8U               AlertImmFlag,INT8U EthIndex,INT8U  *AlertStr, int BMCInst);

extern int EncodeLength     (INT8U*              EncodedLength,
                                    INT8U               Length);

extern int AddEncodedOID    (INT8U*              VarbindData,
                             INT8U*              EncodedOID,
                                    INT8U               Length);
extern int ASNOIDEncode     (const INT16U*        OID,
                                    INT8U               Length,
                             INT8U*              EncodedOID);

extern INT8U ASNEncodeBYTE  (INT8U*              EncodedOID,
                                    INT8U               Length,
                             INT8U*              EncodedData,
                                    INT8U               Data);

extern int ASNEncodeUINT16  (INT8U*              EncodedOID,
                                    INT8U               Length,
                            INT8U*              EncodedData,
                                    INT16U              Data);

extern int ASNEncodeUINT32  (INT8U*              EncodedOID,
                                    INT8U               Length,
                            INT8U*              EncodedData,
                                    INT32U              Data);

extern int ASNEncodeTimestamp(INT8U*             EncodedOID,
                                    INT8U               Length,
                            INT8U*              EncodedData,
                                    INT32U              Data);
extern int ASNEncodeString (INT8U*              EncodedOID,
                                    INT8U               Length,
                            INT8U*              EncodedData,
                            INT8U*              Str,
                                    INT8U               StrLength);
extern int ConstructTrapPDU (SELEventRecord_T*   EvtRecord,
                             INT8U*              EncodedTrapPDU,
                             INT8U*              IPAddr,
                                    INT8U               EventSeverity,INT8U *AlertStr,int BMCInst);

extern int ConstructVarBind (SELEventRecord_T*   EvtRecord,
                             INT8U*              EncodedVarBindings,
                                    INT8U               EventSeverity,INT8U *AlertStr,int BMCInst);
#endif /* LANALERT_H */
