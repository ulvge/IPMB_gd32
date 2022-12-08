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
#include "Types.h"
#include "bsp_uartcomm.h"
#include "main.h"

Shell shell;
char shellBuffer[200];


/**
 * @brief 用户shell写
 * 
 * @param data 数据
 */
void userShellWrite(char data)
{
	UART_sendByte(DEBUG_UART_PERIPH, data);
}


/**
 * @brief 用户shell读
 * 
 * @param data 数据
 * @return char 状态
 */
BOOLEAN userShellRead(char *data)
{
    if (UART_getByte(DEBUG_UART_PERIPH, (INT8U *)data) == false) {  
        return false;
    } else {
        return true;
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
    shellInit(&shell, shellBuffer, sizeof(shellBuffer));
}

