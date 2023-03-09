#include "api_sensor.h"
#include "bsp_gpio.h"
#include "sensor.h"
#include <stdlib.h>
#include <string.h>
#include "dev_fsm.h"
//#include <debug_print.h>

/// @brief count pluse ms of the pin
/// @param pin 
/// @param lastMs 
/// @return The duration of the key pressed
UINT32 KeyPressedDurationMs(Key_ScanST *key)
{
    #define KEY_JITTER_DELAY  20
    uint32_t durationMs;
    BMC_GPIO_enum pin = key->pin;
    if (GPIO_isPinActive(pin)) {
        key->isLastPressed = true;
        durationMs = GetTickMs() - key->preesedStartTick;
        if (durationMs >= KEY_JITTER_DELAY) {
            key->isPreesed = true;
        }
        return durationMs;
    }else{
        if (key->isLastPressed == true) {
            key->isLastPressed = false; 
            key->isPreesed = false;
            key->isReleased = true;
            durationMs = GetTickMs() - key->preesedStartTick;
        } else{
            key->isReleased = false;
			durationMs = 0;
        }
        key->preesedStartTick = GetTickMs(); //update to newest tick
        return durationMs;
    }
}

static const FSM_StateTransform *fsm_findTrans(FSM_StateMachine *pSM, const FSM_EventID evt)
{
    int i;
    for (i = 0; i < pSM->transNum; i++) {
        if ((pSM->transform[i].curState == pSM->curState) && (pSM->transform[i].eventId == evt)) {
            return &pSM->transform[i];
        }
    }
    return NULL;
}
void fsm_Handler(FSM_StateMachine *pSM, const FSM_EventID evt)
{
    const FSM_StateTransform *pTrans;
    pTrans = fsm_findTrans(pSM, evt);
    if (pTrans == NULL) {
        //LOG_I("CurState= %d Do not process enent: %d\r\n", pSM->curState, evt);
        return;
    }
    FSM_Action act = pTrans->action;
    if (act == NULL) {
        LOG_I("change state to %d. No action\r\n", pSM->curState);
        return;
    }
    if (act(pSM, evt) == true) { // is handler over
        pSM->curState = pTrans->nextState;
        if (pSM->printState != NULL) {
            pSM->printState(pSM->curState);
        }
        pSM->lastHandlerTimeStamp = GetTickMs();
    }
}

