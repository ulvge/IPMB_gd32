
#include "boot_update.h"
#include "Message.h"
#include "OSPort.h"
#include "Types.h"
#include "bsp_uartcomm.h"
#include "flash.h"
#include "jump.h"
#include "project_select.h"
#include <stdbool.h>
#include <stdio.h>

#define XMODEM_PAKGE_LENGTH 128

typedef void (*pFunction)(void);
static void param_init(void);
static UPDATE_SM boot_ProcessUpdateReq(const BootPkt_T *pReq);

xQueueHandle updateDatMsg_Queue = NULL;
UPDATE_SM g_UpdatingSM = UPDATE_SM_INIT;
volatile UINT32 g_resendCount = 0;
bool g_xmodemIsCheckTpyeCrc = false;

void JumpToAPP(void)
{
    /* 关闭全局中断 */
    CPU_IntDisable();

    /* 关闭滴答定时器，复位到默认值 */
    NVIC_DisableIRQ(SysTick_IRQn);

    /* 设置所有时钟到默认状态， 使用 HSI 时钟 */
    rcu_deinit();
    // fwdgt_write_disable();

    rcu_periph_clock_disable(RCU_TIMER0);
    rcu_periph_clock_disable(RCU_TIMER1);
    rcu_periph_clock_disable(RCU_TIMER2);
    rcu_periph_clock_disable(RCU_TIMER3);
    rcu_periph_clock_disable(RCU_TIMER4);
    // rcu_periph_clock_disable(RCU_USART0);
    rcu_periph_clock_disable(RCU_USART1);
    rcu_periph_clock_disable(RCU_USART2);
    rcu_periph_clock_disable(RCU_I2C0);
    rcu_periph_clock_disable(RCU_I2C1);
    rcu_periph_clock_disable(RCU_GPIOA);
    rcu_periph_clock_disable(RCU_GPIOB);
    rcu_periph_clock_disable(RCU_GPIOC);
    rcu_periph_clock_disable(RCU_GPIOD);
    rcu_periph_clock_disable(RCU_GPIOE);
    rcu_periph_clock_disable(RCU_GPIOF);
    rcu_periph_clock_disable(RCU_GPIOG);
    rcu_periph_clock_disable(RCU_ADC0);
    rcu_periph_clock_disable(RCU_ADC1);
    rcu_periph_clock_disable(RCU_ADC2);
    /* 关闭所有中断，清除所有中断挂起标志 */
    for (uint32_t i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    /* 跳转到系统 BootLoader，首地址是 MSP，地址+4 是复位中断服务程序地址 */
    uint32_t appJumpAddress = *(volatile uint32_t *)(ADDRESS_START_APP + 4);
    pFunction appJump = (pFunction)appJumpAddress;

    nvic_vector_table_set(ADDRESS_START_APP, 0);
    /* 设置主堆栈指针 */
    __set_CONTROL(0);
    __set_MSP(*(volatile uint32_t *)ADDRESS_START_APP);

    CPU_IntEnable();
    /* 跳转到系统 BootLoader */
    appJump();
    /* 跳转成功的话，不会执行到这里，用户可以在这里添加代码 */
    while (1) {
        ;
    }
}

void updateTask(void *arg)
{
    BootPkt_T reqMsg;

    UINT32 res;

    updateDatMsg_Queue = xQueueCreate(1, sizeof(BootPkt_T));
    if (updateDatMsg_Queue == NULL) {
        LOG_E("updateDatMsg_Queue create ERR!");
    }

    while (1) {
        xQueueReceive(updateDatMsg_Queue, &reqMsg, portMAX_DELAY);

        //printf("xQueueReceive  count = %d \r\n", reqMsg.Size);
        g_UpdatingSM = boot_ProcessUpdateReq(&reqMsg);
    }
}
static bool boot_xmodeCheck(bool type, const UINT8 *buf, UINT8 len, UINT16 recCrc)
{
    UINT8 i = 0;

    if (type == XMODEM_CHECK_CRC16) {     
		UINT16 crc = 0;
		UINT16 tcrc;
        while (len--) {
            crc ^= *buf++ << 8;

            for (i = 0; i < 8; ++i) {
                if (crc & 0x8000) {
                    crc = (crc << 1) ^ 0x1021;
                } else {
                    crc = crc << 1;
                }
            }
        }
		tcrc = recCrc<<8 | (recCrc >> 8);
        if (crc != tcrc)
        {
            return false;
        }
         return true;
    } else  if (type == XMODEM_CHECK_SUM) {
        UINT8 cks = 0;
        for (i = 0; i < len; ++i) 
        {
            cks += buf[i];
        }
        if (cks != recCrc)
        {
            return false;
        }
         return true;
    }
    return false;
}
static bool boot_eraseAllPage()
{
    UINT32 startPageNum = (ADDRESS_START_APP - FLASH_BASE + 1) / FMC_PAGE_SIZE;
    UINT32 erasePageNum = (ADDRESS_END_APP - ADDRESS_START_APP + 1) / FMC_PAGE_SIZE;

    erase_page(startPageNum, erasePageNum);
    return true;
}

static UPDATE_SM boot_ProcessUpdateReq(const BootPkt_T *pReq)
{
    xmodemMsg *msg = (xmodemMsg *)(&pReq->Data);
    static UINT32 pnPage = 0;
    static UINT8 lastPn = 0;
    UINT32 startAddr;
    bool isCrcOK;
    UINT16 tmpCrc;

    g_resendCount = 0;
    switch (msg->head) // 128*256 =
    {
        case XMODEM_SOH:
            isCrcOK = boot_xmodeCheck(g_xmodemIsCheckTpyeCrc, msg->data, sizeof(msg->data), msg->crc);
            if (!isCrcOK && (g_UpdatingSM == UPDATE_SM_START)) {
                g_xmodemIsCheckTpyeCrc = !g_xmodemIsCheckTpyeCrc;
                isCrcOK = boot_xmodeCheck(g_xmodemIsCheckTpyeCrc, msg->data, sizeof(msg->data), msg->crc);
            }
            if (!isCrcOK) {
                boot_UartSendByte(XMODEM_NAK);
                return UPDATE_SM_ERROR_TRYAGAIN;
            }
            if (g_UpdatingSM == UPDATE_SM_START) {
                boot_eraseAllPage();
            }
            if (((UINT8)(lastPn + 1)) != msg->pn){
                return UPDATE_SM_ERROR_TRYAGAIN;
            }
            lastPn = msg->pn;
            if (msg->pn == 0) {
                pnPage += 256;
            }
            startAddr = ADDRESS_START_APP + (pnPage + msg->pn - 1) * XMODEM_PAKGE_LENGTH;

            printf("update start addr = %#X , page Num = %d \r\n", startAddr, pnPage + msg->pn);
            if ((startAddr + XMODEM_PAKGE_LENGTH) > ADDRESS_END_APP) {
                boot_UartSendByte(XMODEM_NAK);
                return UPDATE_SM_ERROR_TRYAGAIN;
            }
            FLASH_Program(startAddr, (uint32_t *)(&msg->data), sizeof(msg->data));
            vTaskDelay(2);
            boot_UartSendByte(XMODEM_ACK);
            return UPDATE_SM_PROGRAMING;
        case XMODEM_EOT:
            boot_UartSendByte(XMODEM_ACK);
            pnPage = 0;
            return UPDATE_SM_FINISHED;
        case XMODEM_CANCEL:
        case 'q': // quit
        case 'Q':
            boot_UartSendByte(XMODEM_ACK);
            pnPage = 0;
            return UPDATE_SM_CANCEL;
        case XMODEM_CTRLC: /* abandon startup ,and prepare to upload */
        case 'u':
        case 'U':
            if (g_UpdatingSM == UPDATE_SM_INIT) {
                return UPDATE_SM_START;
            }
            if (g_UpdatingSM == UPDATE_SM_PROGRAMING) {
                return UPDATE_SM_CANCEL;
            }
            break;
        case 'a': // go to app /* give up upload,then startup */
        case 'A':
            if ((g_UpdatingSM == UPDATE_SM_INIT) || (g_UpdatingSM == UPDATE_SM_START)) {
                return UPDATE_SM_FINISHED;
            }
            break;
        default:
            break;
    }
    return g_UpdatingSM;
}
