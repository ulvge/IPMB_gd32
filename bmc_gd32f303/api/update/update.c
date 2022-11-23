
#include "update/update.h"
#include "project_select.h"
#include <stdio.h>
#include <string.h>

typedef void (*pFunction)(void);

void JumpToBootloader(void)
{
    /* 关闭全局中断 */
    CPU_IntDisable();

    /* 关闭滴答定时器，复位到默认值 */
    NVIC_DisableIRQ(SysTick_IRQn);

    /* 设置所有时钟到默认状态， 使用 HSI 时钟 */
    rcu_deinit();

    /* after 1.6 seconds to generate a reset */
    fwdgt_write_disable();

    rcu_periph_clock_disable(RCU_TIMER0);
    rcu_periph_clock_disable(RCU_TIMER1);
    rcu_periph_clock_disable(RCU_TIMER2);
    rcu_periph_clock_disable(RCU_TIMER3);
    rcu_periph_clock_disable(RCU_TIMER4);
    rcu_periph_clock_disable(RCU_USART0);
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
	
	uint32_t JumpAddress = *(volatile uint32_t *)(ADDRESS_START_BOOTLOADER + 4);
    pFunction SysMemBootJump = (pFunction)JumpAddress;
								
    //nvic_vector_table_set(ADDRESS_START_BOOTLOADER, 0);
    /* 设置主堆栈指针 */
    __set_PSP(*(volatile uint32_t*) ADDRESS_START_BOOTLOADER);
    //__set_PSP(0x20004000);  // 0x200003FC
    __set_CONTROL(0);
    __set_MSP(*(volatile uint32_t *)ADDRESS_START_BOOTLOADER);
    //__set_MSP(0x20004000);

	CPU_IntEnable();
    /* 跳转到系统 BootLoader */
    SysMemBootJump();

    /* 跳转成功的话，不会执行到这里，用户可以在这里添加代码 */
    while (1) {
        ;
    }
}
