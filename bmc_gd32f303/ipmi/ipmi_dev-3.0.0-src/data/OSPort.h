/*****************************************************************
 *****************************************************************
 ***                                                            **
 ***    (C)Copyright 2005-2006, American Megatrends Inc.        **
 ***                                                            **
 ***            All Rights Reserved.                            **
 ***                                                            **
 ***        6145-F, Northbelt Parkway, Norcross,                **
 ***                                                            **
 ***        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 ***                                                            **
 *****************************************************************
 *****************************************************************
 ******************************************************************
 *
 * OSPort.h
 * OS Specific porting functions.
 *
 *  Author: Govind Kothandapani <govindk@ami.com>
 ******************************************************************/
#ifndef OSPORT_H
#define OSPORT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Types.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "event_groups.h"

#include "debug_print.h"

/*------------------------------------------------------------------
 * Timeout Macros
 *------------------------------------------------------------------*/
#define WAIT_NONE		0
#define WAIT_INFINITE	(-1)

/*---------------------------------------------------------------------
 * Error Macros
 *--------------------------------------------------------------------*/
#define OS_NO_ERR       0

/*---------------------------------------------------------------------
 * Memory related macros
 *--------------------------------------------------------------------*/
#define _FAR_

/*---------------------------------------------------------------------*
 * Task related macros.
 *---------------------------------------------------------------------*/
// #define OS_TASK_MAIN(TaskMain)  int main (int argc, char *argv [])
// #define OS_TASK_RETURN          return 0

/*---- Handle to a Task ID -----*/
// typedef pid_t   HTaskID_T;

/*---- Get the currentTask ID -----*/
//#define GET_TASK_ID() getpid()
//#define GET_THREAD_ID() gettid()

#if 0
/*---- Create a new task ---------------------------------------------*/
#define OS_CREATE_TASK(task, pArg, err ,pid)                            \
do                                                                      \
{                                                                       \
                                                                 \
} while (0)

/*---- Stop a  task ---------------------------------------------*/
#define OS_STOP_TASK(pid) 										\
do                                                              \
{                                                               \
                                                       \
} while (0)


/*---- Suspend a Task ---------------------------------------------*/
#define OS_SUSPEND_TASK(Prio, pErr)                             \
do                                                              \
{                                                               \
} while (0)
#endif

/*---- Create a Thread --------------------------------------------*/
#define OS_CREATE_THREAD(Thread, Arg, pErr)                     \
// do                                                              \
// {                                                               \
//     pthread_t   thread_id;                                      \
//     pthread_attr_t  threadAttr;                                 \
//     pthread_attr_init( &threadAttr );                           \
//     pthread_attr_setdetachstate( &threadAttr, PTHREAD_CREATE_DETACHED );\
//     if (0 != pthread_create (&thread_id, &threadAttr, Thread, Arg))    \
//     {                                                           \
//         IPMI_ERROR ("Error creating thread\n");                     \
//     }                                                           \
// } while (0)

/*---- Delete a Thread --------------------------------------------*/
#define OS_DELETE_THREAD()                     \
// do                                                              \
// {                                                               \
//     pthread_t   thread_id;                                      \
//     thread_id = pthread_self();									\
//     pthread_detach(thread_id);									\
//     pthread_exit(NULL);											\
// } while (0)							


/*------------ OS Stack Type -------------------------------------*/
#define OS_STACK	OS_STK


/*-----------------------------------------------------------------
 * Queue specific macros, functions and typedefs
 *-----------------------------------------------------------------*/
/*---- Handle to a Queue -----*/
typedef long    HQueue_T;

/*------ Allocate memory to hold the queue -------------------------*/
#define OS_ALLOC_Q(QueueName, Size)

#define OS_CREATE_Q(Key, Size, Instance)                      \
    // if(Instance != COMMON_QUEUE)                                   \
    // {                                                                                   \
    //     sprintf(keyInstance, "%s%d", Key, Instance);            \
    // }                                                                                  \
    //  else                                                                           \
    // {                                                                                 \
    //     sprintf(keyInstance,"%s",Key);                                 \
    // }                                                                                 \
    // AddQueue(keyInstance);                                                                                    \
    // if (-1 == mkfifo (keyInstance, 0777) && (errno != EEXIST))      \
    // {                                                       \
    //     IPMI_ERROR ("Error creating named pipe %s\n", keyInstance);     \
    // }

#define OS_GET_Q(Key, RDWR,Instance)                         \
    // if(Instance != COMMON_QUEUE)                                  \
    // {                                                                                  \
    //     sprintf(keyInstance, "%s%d", Key, Instance);            \
    // }                                                                                  \
    // else                                                                             \
    // {                                                                                  \
    //     sprintf(keyInstance,"%s",Key);                                   \
    // }                                                                                  \
    // if (GetQueue(keyInstance,RDWR) == -1)                     \
    // {                                                                                 \
    //     IPMI_ERROR ("Attempt to open before creating pipe %s\n", keyInstance);    \
    // }                                                                                \

#define OS_CLOSE_Q(hQueue)                                  \
    // close (hQueue);

#define POST_TO_Q(pBuf,Size,key,pErr,BMCInst)     \
// {                                                                   \
//      AddToQueue(pBuf,key,Size,BMCInst);    \
// }

/*------- Add this message to the queue ----------------------------*/
#define OS_ADD_TO_Q(pBuf, Size, hQueue, pErr)               \
 //   *pErr = write (hQueue, pBuf, Size)                      \

/*------ Get the message from the queue -----------------------------*/
#define OS_GET_FROM_Q(pBuf, Size, hQueue, timeout, pErr) 	\
// {                                                           \
//     int ReadLen = 0, Left, Len;                             \
//     Left = Size;                                            \
//     while( Left > 0 )                                       \
//     {                                                       \
//         Len = read (hQueue, (INT8U*)pBuf + ReadLen, Left ); \
//         if( Len < 0 )                                       \
//         {                                                   \
//             if( errno == EINTR || errno == EAGAIN )         \
//             {                                               \
//                 continue;                                   \
//             }                                               \
//             else                                            \
//             {                                               \
//                 *pErr = -1;                                 \
//                 break;                                      \
//             }                                               \
//         }                                                   \
//         ReadLen += Len;                                     \
//         Left -= Len;                                        \
//     }                                                       \
//     *pErr = ReadLen;                                        \
// }

/*-------- Get the number of messages in the Queue --------------------*/
#define OS_GET_Q_DEPTH(hQueue, Depth, Size)                 \
//  do                                                         \
//  {                                                          \
//     struct  stat Stat;                                      \
//     if(0 == fstat (hQueue, &Stat))                          \
//     {                                                       \
//         Depth = Stat.st_size/Size;                          \
//     }                                                       \
//     else { Depth = 0; }                                     \
//  } while (0)

/*----------------Locking mechanism for Queue-----------------------*/
#define LOCK_QUEUE(hQueue)                              \
// if (-1 == file_lock_write (hQueue))                     \
// {                                                       \
//     IPMI_WARNING ("Error locking Queue\n");       \
// }

#define UNLOCK_QUEUE(hQueue)                            \
// if (-1 == file_unlock (hQueue))                         \
// {                                                       \
//     IPMI_WARNING ("Error unlocking Queue\n");     \
// }                                                       \

/*---------------- Semaphores -------------------------------------*/
typedef int     Semaphore_T;

// #define CREATE_SEMAPHORE(Key)               semget (Key, 1, 0777 | IPC_CREAT)
// #define GET_SEMAPHORE(Key)                  semget (Key, 1, 0777)
#define RELEASE_SEMAPHORE(hSem)             \
// do {                                        \
//     struct sembuf  Sembuf;                  \
//     Sembuf.sem_num = 0;                     \
//     Sembuf.sem_op  = -1;                    \
//     Sembuf.sem_flg = 0;                     \
//     semop (hSem, &Sembuf, 1);               \
// }while (0)

#define LOCK_SEMAPHORE(hSem, Timeout, pErr) \
// do {                                        \
//     struct sembuf  Sembuf [2];              \
//     Sembuf [0].sem_num = 0;                 \
//     Sembuf [0].sem_op  = 0;                 \
//     Sembuf [0].sem_flg = 0;                 \
//     Sembuf [1].sem_num = 0;                 \
//     Sembuf [1].sem_op  = 1;                 \
//     Sembuf [1].sem_flg = 0;                 \
//     if (Timeout == WAIT_NONE)               \
//     {                                       \
//         Sembuf [0].sem_flg |= IPC_NOWAIT;   \
//         semop (hSem, Sembuf, 1);            \
//     }                                       \
//     else if (Timeout == WAIT_INFINITE)      \
//     {                                       \
//         semop (hSem, Sembuf, 2);            \
//     }                                       \
//     else                                    \
//     {                                       \
//         IPMI_ERROR ("OSPort.h : ERROR\n");      \
//     /*OSSemPend (EVENT_ID, TIMEOUT, ERR); */\
//     }                                       \
// } while (0)


/*------------- Mutexes -------------------------------------------*/
#define OS_CREATE_MUTEX(hMutex,MutexName)		    \
    // if ((hMutex = mutex_create_recursive (MutexName)) == NULL)            \
    // {                                                       \
    //     IPMI_ERROR ("Error creating Mutex %s\n",MutexName); \
    // }                                                       \

#define OS_ACQUIRE_MUTEX(hMutex, Timeout)                                      \
// if (Timeout == WAIT_INFINITE)                                                  \
// {                                                                              \
//     if (-1 == mutex_lock_recursive (hMutex, 15 ))                                       \
//     {                                                                          \
//         TCRIT ("Error acquiring %s mutex in Task ID %d  \n", hMutex->MutexName, getpid());      \
//     }                                                                          \
// }                                                                              \
// else                                                                           \
// {                                                                              \
//     if (-1 == mutex_lock_recursive (hMutex, Timeout ))                                  \
//     {                                                                          \
//         TCRIT ("Error acquiring %s mutex in Task ID %d  \n", hMutex->MutexName, getpid());      \
//     }                                                                          \
// }
                                                                                                
#define OS_RELEASE_MUTEX(hMutex)                                              \
// if (-1 == mutex_unlock_recursive (hMutex))                                             \
// {                                                                             \
//     IPMI_ERROR ("Error releasing mutex\n");                                   \
// }                                                       

#define OS_DESTROY_MUTEX(hMutex)						\
// if (-1 == mutex_destroy_recursive (hMutex))                                   \
// {                                                                             \
//     IPMI_ERROR ("Error releasing mutex\n");                                   \
// }                                                       

#define OS_CREATE_TASK_THREAD(Thread,Arg,pErr,ThreadID) \
// do                                                                                         \
// {                                                                                          \
//     if(0 != pthread_create(&ThreadID,NULL,Thread,Arg)) \
//     {                                                                                     \
//         IPMI_ERROR("Error creating thread \n");                 \
//     }                                                                                    \
// } while (0)


    /*------------- ThreadMutexes -------------------------------------------*/
#define OS_THREAD_MUTEX_DECLARE(MutexName) pthread_mutex_t MutexName
    
#define OS_THREAD_MUTEX_INIT(MutexName, Type)                                  \
    // do {                                                                           \
    //     pthread_mutexattr_t    attr;                                               \
    //     pthread_mutexattr_init(&attr);                                             \
    //     pthread_mutexattr_settype(&attr, Type);                                    \
    //     pthread_mutex_init(&MutexName, &attr);                                     \
    //     pthread_mutexattr_destroy(&attr);                                          \
    // } while  (0);

//#define OS_THREAD_MUTEX_DEFINE(MutexName)   pthread_mutex_t MutexName = PTHREAD_MUTEX_INITIALIZER;
    
#define OS_THREAD_MUTEX_ACQUIRE(MutexName, Timeout)                            \
// do{                                                                                                        \
//     if (Timeout == WAIT_INFINITE)                                                  \
//     {                                                                              \
//         if (0 != pthread_mutex_lock(MutexName))                                    \
//         {                                                                          \
//             IPMI_ERROR ("Error acquiring mutex in Thread ID %d\n", getpid());        \
//         }                                                                          \
//     }                                                                              \
//     else                                                                           \
//     {                                                                              \
//         struct timespec ts;                                                        \
//         ts.tv_sec = time(NULL) + Timeout;                                          \
//         ts.tv_nsec = 0L;                                                           \
//         if (ETIMEDOUT == pthread_mutex_timedlock (MutexName, &ts))                 \
//         {                                                                          \
//             IPMI_ERROR ("Error acquiring in Thread ID %d  \n", getpid());              \
//         }                                                                          \
//     }                                                                              \
// }while(0);
    
#define OS_THREAD_MUTEX_RELEASE(MutexName)	                 \
// do {                                                                                                  \
//       if( 0 != pthread_mutex_unlock(MutexName))                           \
//       {                                                                                                 \
//           IPMI_ERROR("Error in releasing mutex in Thread ID %d\n",getpid());  \
//       }                                                                                                 \
// } while (0);

#define OS_THREAD_MUTEX_DESTROY(MutexName)	          \
// do{                                                                                                    \
//      if( 0 != pthread_mutex_destroy(MutexName))                          \
//      {                                                                                                   \
//          IPMI_ERROR("Error in destroying mutex of Thread ID %d\n",getpid()); \
//      }                                                                                                  \
// }while(0);

/*---------------Thread local storage (TLS)--------------------------*/
#define OS_THREAD_TLS_CREATE(key)           \
// do{                                                                     \
//     pthread_key_create(key,NULL);                        \
// }while(0);

#define OS_THREAD_TLS_SET(key,val)                       \
// do{                                                                               \
//     if( 0 != pthread_setspecific(key,val))                        \
//     {                                                                              \
//         IPMI_ERROR("Error in setting TLS data %d \n",getpid());    \
//     }                                                                              \
// }while(0);

#define OS_THREAD_TLS_GET(key,val)                              \
// do{                                                                                     \
//     val = (int)pthread_getspecific(key);                                        \
// }while(0);

/*-------------------------Thread Priority--------------------------------*/

/*-------------------------SCHED_FIFO/RR------------------------------*/
#define HIGH            75
#define LOW             1

#define OS_THREAD_SET_PRIORITY(thrd_id,policy,priority)                 \
// do{                                                                     \
//     struct sched_param param;                                           \
//     param.sched_priority = priority;                                    \
//     if(0 != pthread_setschedparam(thrd_id,policy,&param))               \
//     {                                                                   \
//         IPMI_ERROR("Error while setting the thread priority\n");        \
//     }                                                                   \
// }while(0);

#define OS_THREAD_GET_PRIORITY(thrd_id,policy,priority)                 \
// do{                                                                     \
//     struct sched_param param;                                           \
//     if(0 != pthread_getschedparam(thrd_id,&policy,&param))              \
//     {                                                                   \
//         IPMI_ERROR("Error while setting the thread priority\n");        \
//     }                                                                   \
//     *priority = param.sched_priority;                                   \
// }while(0);


/*------------- Events -------------------------------------------*/
typedef int	Event_T;

// #define CREATE_EVENT(KEY)           semget (KEY, 1, 0777 | IPC_CREAT)
// #define GET_EVENT(KEY)              semget (KEY, 1, 0777)
#define DELETE_EVENT(EVENT, ERR)
#define SIGNAL_EVENT(EVENT_ID)              \
// do {                                        \
//     struct sembuf  Sembuf;                  \
//                                             \
//     Sembuf.sem_num = 0;                     \
//     Sembuf.sem_op  = 1;                     \
//     Sembuf.sem_flg = 0;                     \
//                                             \
//     semop (EVENT_ID, &Sembuf, 1);           \
// }while (0)

#define WAIT_FOR_EVENT(EVENT_ID, TIMEOUT, ERR)\
// do {                                        \
//     struct sembuf  Sembuf;                  \
//                                             \
//     Sembuf.sem_num = 0;                     \
//     Sembuf.sem_op  = -1;                    \
//     Sembuf.sem_flg = 0;                     \
//                                             \
//     if (TIMEOUT == WAIT_NONE)               \
//     {                                       \
//         Sembuf.sem_flg |= IPC_NOWAIT;       \
//         semop (EVENT_ID, &Sembuf, 1);       \
//     }                                       \
//     else if (TIMEOUT == WAIT_INFINITE)      \
//     {                                       \
//         semop (EVENT_ID, &Sembuf, 1);       \
//     }                                       \
//     else                                    \
//     {                                       \
//     /*OSSemPend (EVENT_ID, TIMEOUT, ERR); */   \
//     }                                       \
// } while (0)

/*-------------- Time related macros ------------------------------*/
#define OS_TIME_DELAY_HMSM(Hrs, Mins, Secs, Millisecs)  \
// do {                                                    \
// 	usleep ( (Hrs * 3600 * 1000 * 1000) + (Mins * 60 * 1000 * 1000) + (Secs * 1000 * 1000) + (Millisecs * 1000));         \
// } while (0)

/**
 * @def GET_SYSTEM_TIME_STAMP
 * @brief Gets the system time since the Epoch (00:00:00 UTC, January 1, 1970),
 *        measured in seconds.
 **/
#define GET_SYSTEM_TIME_STAMP() //time(NULL)

#define sleep(x)   vTaskDelay(x*1000)
#define msleep(x)  vTaskDelay(x)
#define delay_ms(x)  vTaskDelay(x)

/**
 * @def SET_SYSTEM_TIME_STAMP
 * @brief Sets the system time since the Epoch (00:00:00 UTC, January 1, 1970),
 *        measured in seconds.
 * @param TIME 32 bit value in seconds from January 1, 1970 GMT.
 **/
#define SET_SYSTEM_TIME_STAMP(TIME) //stime((time_t *)TIME)

#define ipmitoh_u32(x)  (x) 
#define ipmitoh_u16(x)  (x) 
#define htoipmi_u16(Val)    (Val)
#define htoipmi_u32(Val)    (Val)

#define _fmemcpy memcpy
#define _fmemcmp memcmp
#define _fmemset memset
#define _fstrcpy strcpy
#define _fstrlen strlen
#define _fstrcat strcat
#define _fstrcmp strcmp

#endif	/* OSPORT_H */
