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
#include "MsgHndlr.h"
#include "Support.h"
#include "Storage.h"
#include "SDR.h"
#include "SEL.h"
#include "FRU.h"
#include "IPMI_SDR.h"
#include "IPMI_SEL.h"
#include "IPMI_FRU.h"
#include "SELRecord.h"

const CmdHndlrMap_T g_Storage_CmdHndlr [] =
{
    /*--------------------- FRU Device Commands ---------------------------------*/
#if FRU_DEVICE == 1
                                                                                            
    { CMD_FRU_INVENTORY_AREA_INFO,  PRIV_USER,      GET_FRU_INVENTORY_AREA_INFO,    sizeof (FRUInventoryAreaInfoReq_T), 0xAAAA ,0xFFFF},
    { CMD_READ_FRU_DATA,            PRIV_USER,      READ_FRU_DATA,                  sizeof (FRUReadReq_T),  0xAAAA  ,0xFFFF},
    { CMD_WRITE_FRU_DATA,           PRIV_OPERATOR,  WRITE_FRU_DATA,                 0xFF,   0xAAAA  ,0xFFFF},

#endif  /* FRU_DEVICE */

    /*--------------------- SDR Device Commands ---------------------------------*/
#if SDR_DEVICE == 1
    { CMD_GET_SDR_REPOSITORY_INFO,              PRIV_USER,      GET_SDR_REPOSITORY_INFO,                0x00,   0xAAAA ,0xFFFF},
    { CMD_GET_SDR_REPOSITORY_ALLOCATION_INFO,   PRIV_USER,      GET_SDR_REPOSITORY_ALLOCATION_INFO,     0x00,   0xAAAA ,0xFFFF},
    { CMD_RESERVE_SDR_REPOSITORY,               PRIV_USER,      RESERVE_SDR_REPOSITORY,                 0x00,   0xAAAA ,0xFFFF},
    { CMD_GET_SDR,                              PRIV_USER,      GET_SDR,                                sizeof(GetSDRReq_T),    0xAAAA ,0xFFFF},
    { CMD_ADD_SDR,                              PRIV_OPERATOR,  ADD_SDR,                                0xFF,   0xAAAA ,0xFFFF},
    { CMD_PARTIAL_ADD_SDR,                      PRIV_OPERATOR,  PARTIAL_ADD_SDR,                        0xFF,   0xAAAA ,0xFFFF},
    { CMD_DELETE_SDR,                           PRIV_OPERATOR,  DELETE_SDR,                             sizeof(DeleteSDRReq_T),   0xAAAA ,0xFFFF},
    { CMD_CLEAR_SDR_REPOSITORY,                 PRIV_OPERATOR,  CLEAR_SDR_REPOSITORY,                   sizeof(ClearSDRReq_T),  0xAAAA ,0xFFFF},
    { CMD_GET_SDR_REPOSITORY_TIME,              PRIV_USER,      GET_SDR_REPOSITORY_TIME,                0x00,   0xAAAA ,0xFFFF},
    { CMD_SET_SDR_REPOSITORY_TIME,              PRIV_OPERATOR,  SET_SDR_REPOSITORY_TIME,                sizeof(SetSDRRepositoryTimeReq_T),  0xAAAA ,0xFFFF},
    { CMD_ENTER_SDR_REPOSITORY_UPDATE_MODE,     PRIV_OPERATOR,  ENTER_SDR_REPOSITORY_UPDATE_MODE,       0x00,   0xAAAA ,0xFFFF},
    { CMD_EXIT_SDR_REPOSITORY_UPDATE_MODE,      PRIV_OPERATOR,  EXIT_SDR_REPOSITORY_UPDATE_MODE,        0x00,   0xAAAA ,0xFFFF},
    { CMD_RUN_INITIALIZATION_AGENT,             PRIV_OPERATOR,  RUN_INITIALIZATION_AGENT,               sizeof(RunInitAgentReq_T),  0xAAAA ,0xFFFF},

#endif /* SDR_DEVICE */

    /*--------------------- SEL Device Commands ----------------------------------*/
#if SEL_DEVICE == 1
    { CMD_GET_SEL_INFO,                         PRIV_USER,      GET_SEL_INFO,                           0x00,   0xAAAA ,0xFFFF},
    { CMD_GET_SEL_ALLOCATION_INFO,              PRIV_USER,      GET_SEL_ALLOCATION_INFO,                0x00,   0xAAAA ,0xFFFF},
    { CMD_RESERVE_SEL,                          PRIV_USER,      RESERVE_SEL,                            0x00,   0xAAAA ,0xFFFF},
    { CMD_GET_SEL_ENTRY,                        PRIV_USER,      GET_SEL_ENTRY,                          sizeof(GetSELReq_T),    0xAAAA ,0xFFFF},
    { CMD_ADD_SEL_ENTRY,                        PRIV_OPERATOR,  ADD_SEL_ENTRY,                          sizeof(SELEventRecord_T),   0xAAAA ,0xFFFF},
    { CMD_PARTIAL_ADD_SEL_ENTRY,                PRIV_OPERATOR,  PARTIAL_ADD_SEL_ENTRY,                  0xFF,   0xAAAA ,0xFFFF},
    { CMD_DELETE_SEL_ENTRY,                     PRIV_OPERATOR,  DELETE_SEL_ENTRY,                       sizeof(DeleteSELReq_T),   0xAAAA ,0xFFFF},
    { CMD_CLEAR_SEL,                            PRIV_OPERATOR,  CLEAR_SEL,                              sizeof(ClearSELReq_T),  0xAAAA ,0xFFFF},
    { CMD_GET_SEL_TIME,                         PRIV_USER,      GET_SEL_TIME,                           0x00,   0xAAAA ,0xFFFF},
    { CMD_SET_SEL_TIME,                         PRIV_OPERATOR,  SET_SEL_TIME,                           sizeof(SetSELTimeReq_T),    0xAAAA ,0xFFFF},
    { CMD_GET_AUXILIARY_LOG_STATUS,             PRIV_USER,      GET_AUXILIARY_LOG_STATUS,               0xFF,   0xAAAA ,0xFFFF},
    { CMD_SET_AUXILIARY_LOG_STATUS,             PRIV_ADMIN,     SET_AUXILIARY_LOG_STATUS,               0xFF,   0xAAAA ,0xFFFF},
    { CMD_GET_SEL_TIME_UTC_OFFSET,              PRIV_USER,      GET_SEL_TIME_UTC_OFFSET,                0x00,   0xAAAA ,0xFFFF},
    { CMD_SET_SEL_TIME_UTC_OFFSET,              PRIV_OPERATOR,  SET_SEL_TIME_UTC_OFFSET,                sizeof(SetSELTimeUTCOffsetReq_T),   0xAAAA ,0xFFFF},

#endif /* SEL_DEVICE */
    { 0x00,                     0x00,           0x00,                 0x00, 0x0000  ,  0x0000}
};

