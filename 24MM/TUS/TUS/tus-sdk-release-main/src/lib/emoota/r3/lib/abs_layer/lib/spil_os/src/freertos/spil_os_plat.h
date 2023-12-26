/*****************************************************************************
 *
 *         AIRBIQUITY INC. PROPRIETARY AND CONFIDENTIAL INFORMATION
 *
 *  The software and information contained herein is the proprietary and
 *  confidential property of Airbiquity Inc. and shall not be disclosed
 *  in whole or in part. Possession, use, reproduction or transfer of
 *  this software and information is prohibited without the express written
 *  permission of Airbiquity Inc.
 *
 *  Copyright (c) 2021 Airbiquity Inc.  All rights reserved.
 *
 *****************************************************************************/

#ifndef _SPIL_OS_PLATFORM_H_
#define _SPIL_OS_PLATFORM_H_

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <timers.h>

/** @private
 */
struct thrTableEntry {
    StaticTask_t xTaskBuffer;
    TaskHandle_t xHandle;
    byte_t                    *stackAddr;         ///< thread stack addr
    size_t                     stackSize;         ///< thread stack size

    int32_t                    priority;          ///< thread priority
    thrEntry                   entryPoint;        ///< thread entry point

    char                       name[SPIL_OBJECT_NAME_SZ]; //< name of the TCB entry
};

/*****************************************************************************
 *
 *  Semaphore Services
 *
 *****************************************************************************/

/** @private
 *
 */
struct aqSpilSem {
    SemaphoreHandle_t   sem;
};

/** @private
 *
 */
struct aqSpilSemList {
    bool_t                     in_use;       ///< Whether this SemList in use or not
    QueueSetHandle_t           list;        ///< QueueSet for SemList
};

/*****************************************************************************
 *
 *  Mutex Services
 *
 *****************************************************************************/
/** @private
 *
 */
struct aqSpilMtx {
    SemaphoreHandle_t          mtx;               ///< Posix Pthread mutex
};

/** @private
 *
 */
struct aqSpilTmr {
    TimerHandle_t               timer;            ///< handle of the timer
    StaticTimer_t               xTimerBuffer;     ///< static memory for the timer object
    aqSpilSem_t                *sem;              ///< notification
};

/*****************************************************************************
 *
 *  Message Queue Services
 *
 *****************************************************************************/


/** @private
 *  @brief The SPIL internal data structure for managing the SPIL
 *  message queues.
 *
 *  #### the following members must be protected by the .mutex:
 *  * sem
 *  * head
 *  * tail
 *  * msgsInQueue
 */
struct aqSpilMq {
    aqSpilSem_t                *sem;               ///< notify waiters
    SemaphoreHandle_t           mutex;             ///< mutex for queue
    QueueHandle_t               queue;             ///< handle to the queue
    StaticQueue_t               xQueueBuffer;      ///< static memory to hold the queue object
};

#endif /* _SPIL_OS_PLATFORM_H_ */
