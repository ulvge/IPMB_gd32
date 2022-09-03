/**
 * @file shell_port.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2019-02-22
 * 
 * @copyright (c) 2019 Letter
 * 
 */

#include "shell.h"
#include "bsp_usart0.h"
#include "FreeRTOS.h"
#include "queue.h"

Shell shell;
char shellBuffer[512];


/**
 * @brief 用户shell写
 * 
 * @param data 数据
 */
void userShellWrite(char data)
{
	uart0_send_byte(data);
}


extern QueueHandle_t uart_queue;
/**
 * @brief 用户shell读
 * 
 * @param data 数据
 * @return char 状态
 */
signed char userShellRead(char *data)
{
		BaseType_t err = pdFALSE;
	
		err = xQueueReceive(uart_queue, data, portMAX_DELAY);
		if(err == pdFALSE)
		{
			return -1;
		}
		else
		{
			return 0;
		}
}


/**
 * @brief 用户shell初始化
 * 
 */
void userShellInit(void)
{
    shell.write = userShellWrite;
    shell.read = userShellRead;
    shellInit(&shell, shellBuffer, 512);
}

