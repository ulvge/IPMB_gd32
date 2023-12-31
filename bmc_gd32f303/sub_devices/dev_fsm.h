#ifndef __DEV_FSM_H
#define __DEV_FSM_H

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
    UINT32 isLastPressed : 1; // May be invalid
    UINT32 isPreesed : 1;     // valid Preesed
    UINT32 isReleased : 1;
    BMC_GPIO_enum pin;
    UINT32 preesedStartTick;
} Key_ScanST;

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
    FSM_State curState;  // 当前状态
    FSM_EventID eventId; // 事件ID
    FSM_State nextState; // 下个状态
    FSM_Action action;   // 具体表现
} FSM_StateTransform;

typedef struct {
    FSM_State curState;
    UINT8 transNum;
    const FSM_StateTransform *transform;
    FSM_PrintState printState;
    UINT32 lastHandlerTimeStamp;
} FSM_StateMachine;

UINT32 KeyPressedDurationMs(Key_ScanST *key);
void fsm_Handler(FSM_StateMachine *pSM, const FSM_EventID evt);

#ifdef __cplusplus
}
#endif

#endif /* __API_SUB_DEVICES_H */

