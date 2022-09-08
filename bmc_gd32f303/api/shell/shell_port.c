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

Shell shell;
char shellBuffer[512];


/**
 * @brief 用户shell写
 * 
 * @param data 数据
 */
void userShellWrite(char data)
{
	uart1_send_byte(data);
}


/**
 * @brief 用户shell读
 * 
 * @param data 数据
 * @return char 状态
 */
signed char userShellRead(char *data)
{
    uint32_t len=0;
    if (uart1_get_data((INT8U *)data, sizeof(shellBuffer), &len) == true){
		//uart1_send_dat(data, len);
		return 0;
    } else {
        return -1;
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

