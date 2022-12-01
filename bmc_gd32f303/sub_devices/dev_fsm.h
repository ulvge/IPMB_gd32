#ifndef __DEV_FSM_H
#define	__DEV_FSM_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    DEV_ST_POWEROFF = 0,
    DEV_ST_P12VEN,
    DEV_ST_P5V_P3VEN,
    DEV_ST_POWERON,

    DEV_ST_P12VDIS,
    DEV_ST_P5VP3V_DIS,
} FSM_State;


typedef struct {
    FSM_State state;
    const char *alias;
} FSM_StateST;
typedef enum {
    DEV_EVENT_NULL = 0,
    DEV_EVENT_KEY_RELEASED = 1,
} FSM_EventID;

typedef bool (*FSM_Action)(void *pSM, FSM_EventID eventId);
typedef void (*FSM_PrintState)(FSM_State curState);
typedef struct {
    FSM_State curState;  //当前状态
    FSM_EventID eventId; //事件ID
    FSM_State nextState; //下个状态
    FSM_Action action;       //具体表现
} FSM_StateTransform;

typedef struct {
    FSM_State curState;
    UINT8 transNum;
    UINT32 lastHandlerTimeStamp;
    const FSM_StateTransform *transform;
    FSM_PrintState printState;
} FSM_StateMachine;

void fsm_Handler(FSM_StateMachine *pSM, const FSM_EventID evt);

#ifdef __cplusplus
}
#endif

#endif /* __API_SUB_DEVICES_H */

