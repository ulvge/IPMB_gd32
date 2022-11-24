
#include <stdio.h>
#include <stdbool.h>
#include "Types.h"
#include "OSPort.h"
#include "boot_update.h"
#include "project_select.h"   
#include "jump.h"
#include "Message.h"     
#include "flash.h" 
#include "bsp_uartcomm.h"


#define APP_UPGRADE_PAKGE   128
#define DRV_UART1_PutChar(dat) UART_sendByte(USART0, dat)

typedef void (*pFunction)(void);

static void param_init(void);
static unsigned char checksum(const char *buff, int len); 
static UINT32 ProcessUpdateReq(const BootPkt_T *pReq);  
static unsigned char hexchecksum(const char *buff, int len);    
static bool convert_hex_to_msg(const char *hex_dat, uint32_t len, HexRomMsg *hex_msg); 
static int ProcessHexDat(const char *hex_dat, uint32_t len);


xQueueHandle updateDatMsg_Queue = NULL;

void JumpToAPP(void)
{
    /* 关闭全局中断 */
    CPU_IntDisable();

    /* 关闭滴答定时器，复位到默认值 */
    NVIC_DisableIRQ(SysTick_IRQn);

    /* 设置所有时钟到默认状态， 使用 HSI 时钟 */
    rcu_deinit();

    fwdgt_config(2 * 500, FWDGT_PSC_DIV256);
    /* after 1.6 seconds to generate a reset */
    fwdgt_write_disable();

    rcu_periph_clock_disable(RCU_TIMER0);
    rcu_periph_clock_disable(RCU_TIMER1);
    rcu_periph_clock_disable(RCU_TIMER2);
    rcu_periph_clock_disable(RCU_TIMER3);
    rcu_periph_clock_disable(RCU_TIMER4);
    //rcu_periph_clock_disable(RCU_USART0);
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


__IO uint32_t g_app_addr_base = ADDRESS_START_BOOTLOADER;
__IO uint32_t g_app_addr;

void updateTask(void *arg)
{
    BootPkt_T reqMsg;

	UINT32 res;
				  
    updateDatMsg_Queue = xQueueCreate(1, sizeof(BootPkt_T));
    if (updateDatMsg_Queue ==  NULL) {
        LOG_E("updateDatMsg_Queue create ERR!");
    }

	while (1)
	{
        xQueueReceive(updateDatMsg_Queue, &reqMsg, portMAX_DELAY);
		
		printf("xQueueReceive  count = %d \r\n", reqMsg.Size);
		res = ProcessUpdateReq(&reqMsg); 
		
        //vTaskDelay(10);

//		if (res == UPDATE_EVENT_FINISHED)
//		{
//			printf("jump to app \r\n");
//			sleep(1);
//			JumpToAPP();
//        }
	}
}


static UINT16 XMODEM_Crc16(const UINT8 *buf, UINT8 len)
{
    UINT8 i = 0;
    UINT16 crc = 0;

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
    return crc;
}
static bool boot_eraseAllPage()
{
    UINT32 startPageNum = (ADDRESS_START_APP - FLASH_BASE + 1) / FMC_PAGE_SIZE;
    UINT32 erasePageNum = (ADDRESS_END_APP - ADDRESS_START_APP + 1) / FMC_PAGE_SIZE;

    erase_page(startPageNum, erasePageNum);
	return true;
}
static bool g_userUpdating = false;
static bool g_userPrepareUpdate = false;
static bool g_userFormatYet = false;

static UINT32 ProcessUpdateReq(const BootPkt_T *pReq)
{
    xmodemMsg *msg = (xmodemMsg *)(&pReq->Data);
    UINT32 startAddr;
	UINT16 crc;
    switch (msg->head)//128*256 = 
    {
        case XMODEM_SOH:
            crc = XMODEM_Crc16(msg->data, sizeof(msg->data));
            //tcrc = (buf[sz]<<8)+buf[sz+1];
            if (crc != msg->crc) {
                DRV_UART1_PutChar(XMODEM_NAK);
                return UPDATE_EVENT_ERROR_TRYAGAIN;
            }
            if (g_userPrepareUpdate && !g_userFormatYet) {
                g_userFormatYet = true;
				boot_eraseAllPage();
            }
            if ((msg->pn * APP_UPGRADE_PAKGE) > ADDRESS_END_APP) {
                return UPDATE_EVENT_ERROR_TRYAGAIN;
            }
            startAddr = msg->pn * APP_UPGRADE_PAKGE;
            FLASH_Program(startAddr, (uint32_t*)(&msg->data), sizeof(msg->data));
            return UPDATE_EVENT_CONTINUE;
        case XMODEM_EOT:
            DRV_UART1_PutChar(XMODEM_ACK);
            return UPDATE_EVENT_FINISHED;
        case XMODEM_CANCEL:
            if (g_userPrepareUpdate == false) {
                g_userPrepareUpdate = true;
            }
            return UPDATE_EVENT_START;
        case 'q':
        case 'Q':
            DRV_UART1_PutChar(XMODEM_ACK);
            return UPDATE_EVENT_CANCEL;
        default:
            return UPDATE_EVENT_CONTINUE;
    }
}



