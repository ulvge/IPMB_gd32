/*!
    \file    main.c
*/

/*
    Copyright (c) 2018, GigaDevice Semiconductor Inc.

    All rights reserved.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdbool.h>
#include "main.h"

#include "OSPort.h"

#include "systick.h"
#include "bsp_led.h"
#include "bsp_usart0.h"
#include "bsp_usart1.h"
#include "bsp_uart3.h"
#include "bsp_uart7.h"
#include "bsp_i2c.h"

#include "fan/api_fan.h"
#include "adc/api_adc.h"
#include "tmp/api_tmp.h"
#include "cpu/api_cpu.h"
#include "utc/api_utc.h"
#include "flash/api_flash.h"

#include "shell_port.h"

#include "MsgHndlr.h"

#include "gd32f20x_enet_eval.h"
#include "netconf.h"
#include "lwip/tcp.h"
#include "hello_gigadevice.h"
#include "tcp_client.h"
#include "udp_echo.h"
#include "LANIfc.h"
#include "update/update.h" 
#include "net_print/net_print.h"
#include "bsp_timer.h"


#define FAN_TASK_PRIO       22
#define ENET_TASK_PRIO      20
#define TEST_TASK_PRIO      9
#define COM_TASK_PRIO       21
#define LANIFC_TASK_PRIO    23

void start_task(void *pvParameters);
void fan_task(void *pvParameters);
void enet_task(void *pvParameters);
void test_task(void *pvParameters);
void com_task(void *pvParameters);
void led_task(void *pvParameters);

bool net_init(void);
static void watch_dog_init(void);

__IO uint32_t g_localtime = 0; /* for creating a time reference incremented by 10ms */
__IO bool g_net_init_flag = false;
__IO uint64_t g_utc_time_bmc_firmware_build = 0;
__IO uint16_t g_bmc_firmware_version = 0;

const char* shellText = 
	"\r\n"
	"********************************************\r\n"
	"************      BMC INFO      ************\r\n"
	"********************************************\r\n"
	"Build:    "__DATE__"  "__TIME__"\r\n"
	"Version:  "BMC_VERSION" \r\n"
	"Copyright: (c) ZKCC\r\n"
	"********************************************\r\n"
	"********************************************\r\n"
	"\r\n"
;


/*!
    \brief      main function
    \param[in]  none
    \param[out] none
    \retval     none
*/

int main(void)
{
	//nvic_vector_table_set(NVIC_VECTTAB_FLASH, 0x16000);
  systick_config();
  nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
	
#ifdef USE_UART0_ENABLE
	com0_init();
#endif
#ifdef USE_UART1_ENABLE	
	com1_init();
#endif
#ifdef USE_UART3_ENABLE	
	com3_init();
#endif
#ifdef USE_UART7_ENABLE	
	com7_init();
#endif
		
  printf("%s", shellText);
	g_utc_time_bmc_firmware_build = currentSecsSinceEpoch(__DATE__,  __TIME__);
  g_bmc_firmware_version = GetBmcFirmwareVersion(BMC_VERSION);
	
	timer_config_init();
  led_init();
  g_net_init_flag = net_init();
#ifdef NET_LOG_ENABLE	
  xTaskCreate(netPrintTask, "netPrintTask", configMINIMAL_STACK_SIZE*2, NULL, 5, NULL);
#endif
  xTaskCreate(start_task, "start", configMINIMAL_STACK_SIZE*2, NULL, 1, NULL);
  xTaskCreate(led_task, "led", configMINIMAL_STACK_SIZE, NULL, 26, NULL); 
	//watch_dog_init();
	vTaskStartScheduler(); 
  while (1)
  {}
}

void start_task(void *pvParameters)
{
  i2c_int();
  // fan_init();
	sample_init();
	
#ifdef FATFS_ENABLE	
	fatfs_init();
#endif
	
  taskENTER_CRITICAL();

  LOG_I("start create task.\r\n");

  //xTaskCreate(fan_task, "fan", configMINIMAL_STACK_SIZE*2, NULL, FAN_TASK_PRIO, NULL);

  xTaskCreate(enet_task, "enet", configMINIMAL_STACK_SIZE*2, NULL, ENET_TASK_PRIO, NULL);

  //xTaskCreate(test_task, "test", configMINIMAL_STACK_SIZE, NULL, TEST_TASK_PRIO, NULL);

  xTaskCreate(com_task, "com", configMINIMAL_STACK_SIZE*4, NULL, COM_TASK_PRIO, NULL);

  xTaskCreate(LANIfcTask, "LANIfcTask", configMINIMAL_STACK_SIZE*3, NULL, LANIFC_TASK_PRIO, NULL);

  xTaskCreate(updateTask, "updateTask", configMINIMAL_STACK_SIZE*3, NULL, 18, NULL);

  xTaskCreate(tmpSampleTask, "tmpSampleTask", configMINIMAL_STACK_SIZE*2, NULL, 12, NULL);

  xTaskCreate(cpuGetInfoTask, "cpu", configMINIMAL_STACK_SIZE*5, NULL, 11, NULL);
	
	xTaskCreate(shellTask, "shellTask", 512, &shell, 2, NULL);

	vTaskDelete(NULL);
  taskEXIT_CRITICAL();

}

void fan_task(void *pvParameters)
{
  fan_ctrl_loop(); 
}

void enet_task(void *pvParameters)
{
  bool link_status = false;
  // if(!enet_hardware_init()){
  //   printf("enet hardware init failed!\n");
  //   vTaskDelete(NULL);
  // }
  while (1)
  {
    link_status = enet_get_link_status();
    if(link_status == true)
    {
      if(g_net_init_flag != true)  
      {
        printf("init phy chip... \r\n");
        if(enet_software_init()){
          g_net_init_flag = true;
          /* initilaize the LwIP stack */
          lwip_stack_init();
          printf("phy init finished!\r\n");
        }
      }
    } 
    vTaskDelay(2000);
  }
}

void test_task(void *pvParameters)
{
  while (1)
  {
		vTaskDelay(1000);
  }
}
 
void led_task(void *pvParameters)
{
  while (1)
  {
    led1_set(1);
    vTaskDelay(70);
    led1_set(0);
    vTaskDelay(700);
  }
}

void com_task(void *pvParameters)
{
  MsgHndlr(pvParameters);
}


/*!
    \brief      updates the system local time
    \param[in]  none
    \param[out] none
    \retval     none
*/
void time_update(void)
{
    g_localtime += portTICK_PERIOD_MS;
}


bool net_init()
{
    if(!enet_hardware_init()){
      printf("enet hardware init failed!\r\n");
      return false;
    }
    if(!enet_get_link_status())
    {
      printf("phy not linked!\r\n");
      return false;
    }

    printf("init phy chip... \r\n");
    if(!enet_software_init()){
      return false;
    } 
    /* initilaize the LwIP stack */
    lwip_stack_init();
    printf("phy init finished!\r\n");
 
    return true;
}

/*!
    \brief     task idle hook functions
    \param[in]  none     
    \param[out] none
    \retval     none
*/
void vApplicationIdleHook( void )
{
		/* reload FWDGT counter */
		fwdgt_counter_reload();
}

static void watch_dog_init()
{
	    /* check if the system has resumed from FWDGT reset */
    if (RESET != rcu_flag_get(RCU_FLAG_FWDGTRST)){

    }
    /* confiure FWDGT counter clock: 40KHz(IRC40K) / 64 = 0.625 KHz */
    fwdgt_config(2*500,FWDGT_PSC_DIV64);
    /* after 1.6 seconds to generate a reset */
    fwdgt_enable();
}
