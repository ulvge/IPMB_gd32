#include "net_print/net_print.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "lwip/opt.h"
#include "lwip/tcp.h"
#include "lwip/sys.h"
#include "gd32f20x.h"
#include "lwip/memp.h"
#include "main.h"
#include "lwip/api.h"
#include "lwip/sockets.h"
#include "OSPort.h"

#ifdef NET_LOG_ENABLE

volatile int g_socketfd = -1;

void netPrintTask(void *arg)
{
    int recv_len = -1;
    char buff[50];
    int sock;
    struct sockaddr_in server_addr, client_addr;
    int listen_port = 8899;
    struct timeval timeout;
    fd_set readset, readset_c;
    socklen_t sin_size = sizeof(struct sockaddr_in);

    while (1)
    {
        if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
        {
            sleep(1);
            continue;
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(listen_port); 
        server_addr.sin_addr.s_addr = INADDR_ANY;
        memset(&(server_addr.sin_zero), 0x0, sizeof(server_addr.sin_zero));

        /* 绑定socket到服务端地址 */
        if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
        {
            printf("Unable to bind\r\n");
            continue;
        }

        /* 在socket上进行监听 */
        if (listen(sock, 10) == -1)
        {
            printf("Listen error\r\n");
            goto __exit;
        }

        printf("TCPServer Waiting for client on port %d...\r\n", listen_port);
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

        while (1)
        {
            FD_ZERO(&readset);
            FD_SET(sock, &readset);

            /* Wait for read or write */
            if (select(sock + 1, &readset, NULL, NULL, &timeout) == 0)
                continue;

            /* 接受一个客户端连接socket的请求，这个函数调用是阻塞式的 */
            g_socketfd = accept(sock, (struct sockaddr *)&client_addr, &sin_size);
            /* 返回的是连接成功的socket */
            if (g_socketfd < 0)
            {
                printf("accept connection failed!\r\n");
                continue;
            }

            /* 接受返回的client_addr指向了客户端的地址信息 */
            printf("update socket got a connection from (%s , %d)\r\n",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            /* 客户端连接的处理 */
            while (1)
            {
                FD_ZERO(&readset_c);
                FD_SET(g_socketfd, &readset_c);

                /* Wait for read or write */
                if (select(g_socketfd + 1, &readset_c, NULL, NULL, &timeout) == 0)
                {
                    continue;
                }

                recv_len = recv(g_socketfd, buff, 100, 0);
                if (recv_len < 0)
                {
                    printf("Received error, close the connect.\r\n");
                    closesocket(g_socketfd);
                    g_socketfd = -1;
                    break;
                }
                else if (recv_len == 0)
                {
                    /* 打印recv函数返回值为0的警告信息 */
                    printf("Received warning, recv function return 0.\r\n");
                    continue;
                }
                else
                {
                    /* 有接收到数据，把末端清零 */
                    buff[recv_len] = '\0';
                    if (strcmp(buff, "q") == 0 || strcmp(buff, "Q") == 0)
                    {
                        /* 如果是首字母是q或Q，关闭这个连接 */
                        printf("Got a 'q' or 'Q', close the connect.\r\n");
                        closesocket(g_socketfd);
                        g_socketfd = -1;
                        break;
                    }
                    else if (strcmp(buff, "exit") == 0)
                    {
                        /* 如果接收的是exit，则关闭整个服务端 */
                        closesocket(g_socketfd);
                        g_socketfd = -1;
                        goto __exit;
                    }
                    else
                    {  // data process
                        // printf("Received data = %s", recv_data);
                        if (strcmp(buff, "connect") == 0)
                        {
                            net_raw_print("Log socket connected!\r\n");
                        }
                    }
                }
                
            }
        }
    }

__exit:
    if (g_socketfd >= 0)
    {
        closesocket(g_socketfd);
        g_socketfd = -1;
    }
    if (sock >= 0)
    {
        closesocket(sock);
        sock = -1;
    }
    vTaskDelete(NULL);
    return;
}

/**
 * @fn 
 * @brief 
 * @param 
**/
int net_raw_print (const char* format, ...)
{
    int len;
    char buf[100];
    va_list arglist;


    if (-1 != g_socketfd){ 
        va_start(arglist, format);
        vsprintf(buf, format, arglist);
        len = send(g_socketfd, buf, strlen(buf), 0);
        if (len < 0)
        {
            printf("socket send error.\r\n");
            g_socketfd = -1;
            closesocket(g_socketfd);
        }
        else if (len == 0)
        {
            printf("Send warning, send function return 0.\r\n");
        }
        va_end(arglist);
    }

    return len;
}

/**
 * @fn 
 * @brief 
 * @param 
**/
int net_hdr_end_print (const char* hdr_str, const char* end_str, const char* format, ...)
{
    int len;
    char buf[100];
    va_list arglist;

    if (-1 != g_socketfd){ 
        strcpy(buf, hdr_str);
        va_start(arglist, format);
        vsprintf(&buf[strlen(hdr_str)], format, arglist);
        strcat((char*)buf, end_str);
        len = send(g_socketfd, buf, strlen(buf), 0);
        if (len < 0)
        {
            printf("socket send error.\r\n");
            g_socketfd = -1;
            closesocket(g_socketfd);
        }
        else if (len == 0)
        {
            printf("Send warning, send function return 0.\r\n");
        }
        va_end(arglist);
    }
    
    return len;
}


#endif
