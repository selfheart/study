/* ***************************************************************************
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
 * ***************************************************************************
 */
/* Outside interface definitions */
#include <spil_os/aqSpilImpl.h>
#include <spil_file/spil_file.h>
#include <ontrac/ontrac/status_codes.h>
#include <ontrac/util/thread_utils.h>

// TODO-FREERTOS: Make sure INCLUDE_vTaskSuspend is defined in FreeRTOSConfig.h
#define BLOCK_INDEFINITELY portMAX_DELAY

static aqSpilResources_t *theResources = NULL;
static aqSpilClkContinuousTime_t spilStartTime = { 0 };
static aqSpilSem_t *semTable = NULL;

static aqTcb_t *thrTable = NULL;
static aqSpilMq_t *mqTable = NULL;
static aqSpilSemList_t *semListTable = NULL;
static aqSpilMtx_t *mtxTable = NULL;

static aqSpilTmr_t        *tmrPool=NULL;
static aqSpilMtx_t        tmrPoolMutex = {0};

static aqSpilMtx_t atomicMutex = { 0 };
static aqSpilMtx_t semTableMutex = { 0 };

static aqSpilThrStatus_t allocateThreads(void);
static err_t initSpil(void);

static TaskHandle_t main_task_handle = NULL;
/* ****************************************************************************
 *
 *  SPIL public functions
 *
 * ***************************************************************************
 * */

// comment maintained in aqSpilImple.h
err_t aqSpilBindOS(aqSpilResources_t *resources) {
    int32_t i = 0;
    err_t retval = EXIT_SUCCESS;

    static bool_t hasBeenCalled = false;
    byte_t sb_buf[256] = "";
    ABQ_ENCODER(encoder, &ascii_codec, sb_buf, sizeof(sb_buf)); /* parasoft-suppress MISRAC2012-RULE_2_2-b-2 "m0048. encoder is used by reference later in the function" */

    if (NULL == resources) {
        SLOG_E("[aqSpilBindOS] invalid argument.");
        retval = EUNSPECIFIED;
    }

    if ((EXIT_SUCCESS == retval) && (true == hasBeenCalled)) {
        SLOG_E("[aqSpilBindOS] illegal operation.");
        retval = EUNSPECIFIED;
    }

    if ((EXIT_SUCCESS == retval) && (0 != spilStartTime.tv_sec)) {
        SLOG_E("[aqSpilBindOS] already bound.");
        retval = EUNSPECIFIED;
    }

    if (EXIT_SUCCESS == retval) {
        main_task_handle = xTaskGetCurrentTaskHandle();

        theResources = resources;
        thrTable = resources->tcbs;
        semTable = resources->semas;
        mtxTable = resources->mutexes;
        semListTable = resources->semaLists;
        mqTable = resources->queues;
        tmrPool = resources->timerz;

        retval = initSpil();
    }

    if (EXIT_SUCCESS == retval) {
        err_t ret = aqSpilClkGetContinuousTime(&spilStartTime);
        if (EXIT_SUCCESS != ret) {
            SLOG_E("[aqSpilBindOS] failed to get time.");
            retval = EUNSPECIFIED;
        }
    }

    hasBeenCalled = true;

    if (EXIT_SUCCESS == retval) {
        initWorkerThreads(resources);
    }

    if (EXIT_SUCCESS == retval) {
        /* create the SPIL "kernel" thread  and initialize resources */
        if (aqSpilThrStatusOK != allocateThreads()) {
            SLOG_E("[aqSpilBindOS] internal error.");
            retval = EUNSPECIFIED;
        }
    }
    // TODO-FREERTOS: wait for sync with threads before continue (as the POSIX implementation does)?
    if (EXIT_SUCCESS == retval) {
        if (NULL != resources->allocFunc) {
            if (EXIT_SUCCESS != resources->allocFunc(resources->allocFuncArg)) {
                retval = EUNSPECIFIED;
            }
        }
    }

    return retval;
}

/** @private
 * @brief Check that sem handle is valid
 *
 * ## Globals affected
 * * semTable
 *
 * @param sem Pointer to semaphore table entry
 * @return <pre>true</pre> if the provided semaphore pointer is a valid handle.
 *         <pre>false</pre> otherwise.
 */
static bool_t isValidSemHandle(const aqSpilSem_t *sem) {
    bool_t retval = true;
    // FIX_ME add more rigorous tests
    if ((sem < &semTable[0]) || (sem >= &semTable[theResources->numSemas])) {
        retval = false;
    }

    return retval;
}

/*****************************************************************************
 *
 *  Function Name:      allocateThread
 *
 *  Description:        Allocate resources to create or register a thread
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:   None
 *
 *  Output Arguments:   None
 *
 *  Return Value:       aqSpilThrStatus_t
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   thrTable
 *                      FIX_ME <list other global variables>
 *
 *  Comments:           This function is called from client thread's context.
 *
 *****************************************************************************/
static aqSpilThrStatus_t allocateThreads(void) {
    aqSpilThrStatus_t retval = aqSpilThrStatusOK;
    aqTcb_t *thisEntry = &thrTable[0];

    if (aqSpilThrStatusOK == retval) {
        if (thrTable == thisEntry) { // first iteration only
            aqTcb_t *t = thrTable;
            int32_t clientSchedPolicy = 0;
            aqSpilThreadAttr_t *a = theResources->threadAttrs;

            if (aqSpilThrStatusOK == retval) {
                uint16_t n = 0;
                for (n = 0; n < (theResources->numAppThreads); n++) {
                    t->priority = a->priority;
                    t->stackAddr = a->stackAddr;
                    t->stackSize = a->stackSize;
                    t->entryPoint = a->entryPoint;
                    aqSpilThrSetName(t, a->name);
                    t++;
                    a++;
                }

                for (n = 0; n < theResources->numWorkerThreads; n++) {
                    t->priority = 10; // FIXME: need a worker thread default priority.
                    t->stackAddr = (byte_t *)&theResources->workerThreadStacks
                                           [n * ((theResources->workerThreadStackSz + 3U) / 4U)];
                    t->stackSize = theResources->workerThreadStackSz;
                    t->entryPoint = workerThreadEntry;
                    t++;
                }
            }
        }
    }

    if (aqSpilThrStatusOK == retval) {
        size_t total_num = IMPL_RESERVED_THREAD_NUM + theResources->numAppThreads +
                           theResources->numWorkerThreads;
        for (size_t i = 0U; i < total_num; i++) {
            thisEntry = &thrTable[i];
            ABQ_TRACE_MSG_Z("thread_create: ", thisEntry->name);
            thisEntry->xHandle = xTaskCreateStatic((TaskFunction_t)thisEntry->entryPoint,
                    thisEntry->name, thisEntry->stackSize / sizeof(StackType_t), thisEntry,
                    tskIDLE_PRIORITY, // TODO-FREERTOS
                    (StackType_t *)thisEntry->stackAddr, &thisEntry->xTaskBuffer);
            if (NULL == thisEntry->xHandle) {
                ABQ_ERROR_MSG("Unable to create thread");
                retval = aqSpilThrStatusError;
                break;
            }
        }
    }
    return retval;
}

// comment maintained in aqSpil.h
aqSpilThrStatus_t aqSpilThrSleep(uint32_t periodInUs) {
    aqSpilThrStatus_t retval = aqSpilThrStatusOK;
    vTaskDelay(pdMS_TO_TICKS(periodInUs / 1000U));
    return retval;
}

/** @public
 * @brief Set the name of a SPIL Object.
 * This method is used to update the name proporty of the
 * target object.  This method is aware of maximum length
 * allowed for said name and will truncate the provided
 * name as needed.
 *
 * #### NOTE:
 * * Only the thread object currently has a name attribute.
 * * TODO: Update other SPIL objects to have name attributes
 * * This makes debugging easier when steping through the SPIL kernel code.
 *
 * @param dst
 * @param src
 */
void aqSpilSetName(char *dst, const char *src) {
    if ((NULL != dst) && (NULL != src)) {
        size_t sz = strlen(src);
        sz = (sz >= (size_t)SPIL_OBJECT_NAME_SZ) ? ((size_t)SPIL_OBJECT_NAME_SZ - 1u) : sz;
        (void)memset(dst, 0, SPIL_OBJECT_NAME_SZ);
        (void)memcpy(dst, src, sz);
    }
}

void aqSpilThrSetName(aqTcb_t *thr, const char *name) {
    if ((NULL != thr) && (NULL != name)) {
        aqSpilSetName(thr->name, name);
    }
}

/*****************************************************************************
 *
 *  Function Name:      aqSpilSemAllocate
 *
 *  Description:        Allocate a semaphore
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    None
 *
 *  Output Arguments:   None
 *
 *  Return Value:       aqSpilSem_t *
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   semTable
 *                      semTableMutex
 *                      FIX_ME <list other global variables>
 *
 *  Comments:           None
 *
 *****************************************************************************/
aqSpilSem_t *aqSpilSemAllocate(void) {
    static uint32_t semAllocatedCount = 0;
    aqSpilSem_t *returnValue = NULL;

    if (NULL != theResources) {
        if (aqSpilMtxStatusOK == aqSpilMtxLock(&semTableMutex)) {

            // return handle of next available semaphore
            if (semAllocatedCount >= theResources->numSemas) {
                semAllocatedCount = theResources->numSemas;
            } else {
                /* The following 2 lines are retarded because of MISRA2012-RULE-13_3 */
                returnValue = &semTable[semAllocatedCount];
                semAllocatedCount++;
            }

            if (aqSpilMtxStatusOK != aqSpilMtxUnlock(&semTableMutex)) {
                returnValue = NULL;
            }
        }
    }

    return returnValue;
}

/*****************************************************************************
 *
 *  Function Name:      aqSpilSemRegisterList
 *
 *  Description:        Register a list s semaphores
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    semCollection
 *                         FIX_ME <detailed value information>
 *
 *  Output Arguments:   None
 *
 *  Return Value:       aqSpilListSem_t *
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   semTable
 *                      semListTable
 *                      semTableMutex
 *                      FIX_ME <list other global variables>
 *
 *  Comments:           None
 *
 *****************************************************************************/
aqSpilSemList_t *aqSpilSemRegisterList(aqSpilSem_t **semCollection) {
    aqSpilSem_t *semListElement = NULL;
    aqSpilSemList_t *semList = NULL;
    uint16_t i = 0;
    aqSpilSemList_t *returnValue = NULL;

    if ((NULL != semCollection) && (NULL != *semCollection)) {

        if (aqSpilMtxStatusOK == aqSpilMtxLock(&semTableMutex)) {

            // verify that a list is available in semListTable
            for (i = 0; i < theResources->numSemLists; i++) {
                if (false == semListTable[i].in_use) {
                    semList = &semListTable[i];
                    break;
                }
            }

            // traverse and validate proposed semaphore list
            i = 0;
            semListElement = semCollection[i];
            while (semListElement != NULL) {
                // validate sem handle and verify sem is not already part of a list
                if ((false == isValidSemHandle(semListElement))) { // TODO-FREERTOS
                    break;
                }
                i++;
                semListElement = semCollection[i];
            }

            /* NOTE: if previous while loop terminated properly, then semListElement will == NULL
             * here */
            if ((NULL == semListElement) && (NULL != semList)) {
                // proposed list was valid and have a place to store it, so register list
                i = 0;
                semList->in_use = true;
                semListElement = semCollection[0];

                /* Populate the semList */
                while (NULL != semListElement) {
                    // Add to QueueSet
                    xQueueAddToSet(semListElement->sem, semList->list);

                    /* Move to next list element */
                    i++;
                    semListElement = semCollection[i];
                }
                returnValue = semList;
            }

            if (aqSpilMtxStatusOK != aqSpilMtxUnlock(&semTableMutex)) {
                returnValue = NULL;
            }
        }
    }
    return returnValue;
}

/*****************************************************************************
 *
 *  Function Name:      aqSpilSemPost
 *
 *  Description:        Signal a semaphore
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    sem
 *                         FIX_ME <detailed value information>
 *
 *  Output Arguments:   None
 *
 *  Return Value:       aqSpilSemStatus_t
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   isoSystemPipe
 *                      FIX_ME <list other global variables>
 *
 *  Comments:           None
 *
 *****************************************************************************/
aqSpilSemStatus_t aqSpilSemPost(aqSpilSem_t *sem) {
    aqSpilSemStatus_t spilStatus = aqSpilSemStatusOK;
    if ((NULL == sem) || (NULL == sem->sem)) {
        spilStatus = aqSpilSemStatusError;
    } else {
        if (pdTRUE != xSemaphoreGive(sem->sem)) {
            spilStatus = aqSpilSemStatusError;
        }
    }
    return spilStatus;
}

/*****************************************************************************
 *
 *  Function Name:      aqSpilSemWait
 *
 *  Description:        Wait on a semaphore
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    sem
 *                         FIX_ME <detailed value information>
 *
 *  Output Arguments:   None
 *
 *  Return Value:       aqSpilSemStatus_t
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   semTable
 *                      semListTable
 *                      semTableMutex
 *                      thrTable
 *                      FIX_ME <list other global variables>
 *
 *  Comments:           None
 *
 *****************************************************************************/
aqSpilSemStatus_t aqSpilSemWait(aqSpilSem_t *sem) {
    aqSpilSemStatus_t sem_status = aqSpilSemStatusOK;
    if ((NULL == sem) || (NULL == sem->sem)) {
        sem_status = aqSpilSemStatusError;
    } else {
        if (pdTRUE != xSemaphoreTake(sem->sem, BLOCK_INDEFINITELY)) {
            sem_status = aqSpilSemStatusError;
        }
    }
    return sem_status;
}

/*****************************************************************************
 *
 *  Function Name:      aqSpilSemListWait
 *
 *  Description:        Wait on a list of semaphores
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    semList
 *                         FIX_ME <detailed value information>
 *
 *  Output Arguments:   None
 *
 *  Return Value:       aqSpilSem_t *
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   semTable
 *                      semListTable
 *                      semTableMutex
 *                      thrTable
 *                      FIX_ME <list other global variables>
 *
 *  Comments:           None
 *
 *****************************************************************************/
aqSpilSem_t *aqSpilSemListWait(aqSpilSemList_t *semList) {
    aqSpilSem_t *sem = NULL;

    QueueSetMemberHandle_t xActivatedMember = NULL;

    if ((NULL != semList) && (NULL != semList->list)) {
        xActivatedMember = xQueueSelectFromSet(semList->list, BLOCK_INDEFINITELY);
    }

    if (NULL != xActivatedMember) {
        if (aqSpilMtxStatusOK == aqSpilMtxLock(&semTableMutex)) {
            for (size_t i = 0U; i < theResources->numSemas; i++) {
                if (semTable[i].sem == xActivatedMember) {
                    sem = &semTable[i];
                    break;
                }
            }

            if (NULL == sem) {
                ABQ_ERROR_MSG("Invalid semaphore");
            }

            /* Take the semaphore to make sure it can be "given" again. */
            xSemaphoreTake(xActivatedMember, 0);

            ABQ_EXPECT(aqSpilMtxStatusOK == aqSpilMtxUnlock(&semTableMutex));
        }
    }
    return sem;
}

/*****************************************************************************
 *
 *  Function Name:      aqSpilSemTimedWait
 *
 *  Description:        Wait on a semaphore with a time limit
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    sem
 *                         FIX_ME <detailed value information>
 *                      interval
 *                         FIX_ME <detailed value information>
 *
 *  Output Arguments:   None
 *
 *  Return Value:       aqSpilSemStatus_t
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   semTable
 *                      semListTable
 *                      semTableMutex
 *                      thrTable
 *                      FIX_ME <list other global variables>
 *
 *  Comments:           None
 *
 *****************************************************************************/
#ifdef SPIL_FEATURE_SEM_TIMEDWAIT
aqSpilSemStatus_t aqSpilSemTimedWait(aqSpilSem_t *sem, uint32_t interval) {
    aqSpilSemStatus_t sem_status = aqSpilSemStatusOK;
    if ((NULL == sem) || (NULL == sem->sem)) {
        sem_status = aqSpilSemStatusError;
    } else {
        if (pdFALSE == xSemaphoreTake(sem->sem, pdMS_TO_TICKS(interval / 1000U))) {  // TODO: interval in usec?
            sem_status = aqSpilSemStatusTimeout;
        }
    }
    return sem_status;
}
#endif /* SPIL_FEATURE_SEM_TIMEDWAIT */

/*****************************************************************************
 *
 *  Function Name:      aqSpilSemGetValue
 *
 *  Description:        Get current value of a semaphore
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    sem
 *                         FIX_ME <detailed value information>
 *
 *  Output Arguments:   semValue
 *                         FIX_ME <detailed value information>
 *
 *  Return Value:       aqSpilSemStatus_t
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   semTable
 *                      semTableMutex
 *                      FIX_ME <list other global variables>
 *
 *  Comments:           None
 *
 *****************************************************************************/
aqSpilSemStatus_t aqSpilSemGetValue(aqSpilSem_t *sem, int32_t *semValue) {
    aqSpilSemStatus_t sem_status = aqSpilSemStatusOK;

    if ((NULL == sem) || (NULL == sem->sem) || (NULL == semValue)) {
        sem_status = aqSpilSemStatusError;
    } else {
        *semValue = uxSemaphoreGetCount(sem->sem);
    }
    return sem_status;
}

/*****************************************************************************
 *
 *  Function Name:      aqSpilMtxAllocate
 *
 *  Description:        Allocate an unlocked mutex
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    None
 *
 *  Output Arguments:   None
 *
 *  Return Value:       aqSpilMtx_t *
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   mtxTable
 *                      FIX_ME <list other global variables>
 *
 *  Comments:           None
 *
 *****************************************************************************/
aqSpilMtx_t *aqSpilMtxAllocate(void) {
    static uint32_t mtxAllocatedCount = 0;
    aqSpilMtx_t *mutex = NULL;
    /* return handle of next available mutex */
    if (mtxAllocatedCount < theResources->numMutexes) {
        mutex = &mtxTable[mtxAllocatedCount];
        mtxAllocatedCount++;
    }
    return mutex;
}

/*****************************************************************************
 *
 *  Function Name:      aqSpilMtxLock
 *
 *  Description:        Lock a mutex
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    mutex
 *                         FIX_ME <detailed value information>
 *
 *  Output Arguments:   None
 *
 *  Return Value:       aqSpilMtxStatus_t
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   FIX_ME <list other global variables>
 *
 *  Comments:           None
 *
 *****************************************************************************/
aqSpilMtxStatus_t aqSpilMtxLock(aqSpilMtx_t *mutex) {
    aqSpilMtxStatus_t mtx_status = aqSpilMtxStatusOK;
    if ((NULL == mutex) || (NULL == mutex->mtx)) {
        mtx_status = aqSpilMtxStatusError;
    } else {
        if (pdTRUE != xSemaphoreTakeRecursive(mutex->mtx, BLOCK_INDEFINITELY)) {
            mtx_status = aqSpilMtxStatusError;
        }
    }
    return mtx_status;
}

/*****************************************************************************
 *
 *  Function Name:      aqSpilMtxUnlock
 *
 *  Description:        Unlock a mutex
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    mutex
 *                         FIX_ME <detailed value information>
 *
 *  Output Arguments:   None
 *
 *  Return Value:       aqSpilMtxStatus_t
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   FIX_ME <list other global variables>
 *
 *  Comments:           None
 *
 *****************************************************************************/
aqSpilMtxStatus_t aqSpilMtxUnlock(aqSpilMtx_t *mutex) {
    aqSpilMtxStatus_t mtx_status = aqSpilMtxStatusOK;
    if ((NULL == mutex) || (NULL == mutex->mtx)) {
        mtx_status = aqSpilMtxStatusError;
    } else {
        if (pdTRUE != xSemaphoreGiveRecursive(mutex->mtx)) {
            mtx_status = aqSpilMtxStatusError;
        }
    }
    return mtx_status;
}

err_t aqSpilGetRuntimeInMillis(uint64_t *timeInMillis) {
    err_t status = EUNSPECIFIED;
    static aqSpilClkContinuousTime_t now;
    // get current timestamp
    if ((NULL != timeInMillis) && (0 != spilStartTime.tv_sec)) {
        if (EXIT_SUCCESS == aqSpilClkGetContinuousTime(&now)) {
            *timeInMillis = 1000u * ((uint64_t)now.tv_sec - (uint64_t)spilStartTime.tv_sec);

            number_t tmp_time_in_ms = ((number_t)now.tv_nsec - (number_t)spilStartTime.tv_nsec);
            tmp_time_in_ms = tmp_time_in_ms / (number_t)MS_PER_NANO_SEC;
            *timeInMillis += (uint64_t)tmp_time_in_ms;
            status = EXIT_SUCCESS;
        }
    }
    return status;
}

/*****************************************************************************
 *
 *  Function Name:      aqSpilTmrAllocate
 *
 *  Description:        Allocate a timer resource
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    sem
 *                         FIX_ME <detailed value information>
 *
 *  Output Arguments:   None
 *
 *  Return Value:       aqSpilTmr_t *
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   tmrPool
 *                      tmrPoolMutex
 *
 *  Comments:           None
 *
 *****************************************************************************/
aqSpilTmr_t *aqSpilTmrAllocate(aqSpilSem_t *sem) {
    aqSpilTmr_t *tmr = NULL;

    if (NULL != sem) {

        VITAL_IS_OK(aqSpilMtxLock(&tmrPoolMutex));

        for (uint16_t i = 0; i < theResources->numTimerz; i++ ) {
            tmr = &tmrPool[i];
            if (NULL == tmr->sem) {
                break; // found a free timer
            }
        }

        if (tmr >= &tmrPool[theResources->numTimerz] ) {
            VITAL_IS_OK(aqSpilMtxUnlock(&tmrPoolMutex));
            /* handle outside bounds of timer pool. Nullify it */
            tmr = NULL;
        } else {
            if( NULL != tmr ) {
                tmr->sem = sem;
            }

            VITAL_IS_OK(aqSpilMtxUnlock(&tmrPoolMutex));
        }
    }
    return tmr;
}

static void vTimerCallback( TimerHandle_t xTimer ) {
    VITAL_NOT_NULL(xTimer);
    aqSpilTmr_t *timer = (aqSpilTmr_t *)pvTimerGetTimerID(xTimer);
    VITAL_NOT_NULL(timer);
    VITAL_NOT_NULL(timer->sem);
    aqSpilSemPost(timer->sem);
}
/*****************************************************************************
 *
 *  Function Name:      aqSpilTmrStart
 *
 *  Description:        Start a timer
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    timer
 *                         FIX_ME <detailed value information>
 *                      period
 *                         FIX_ME <detailed value information>
 *                      isPeriodic
 *                         FIX_ME <detailed value information>
 *
 *  Output Arguments:   None
 *
 *  Return Value:       aqSpilTmrStatus_t
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   tmrPool
 *                      tmrPoolMutex
 *
 *  Comments:           None
 *
 *****************************************************************************/
aqSpilTmrStatus_t aqSpilTmrStart(aqSpilTmr_t *timer,
        uint32_t periodInUsecs, // in ticks
        bool_t isPeriodic) {
    aqSpilTmrStatus_t status = aqSpilTmrStatusOK;
    if (NULL == timer) {
        status = aqSpilTmrStatusError;
    } else {
        if (NULL != timer->timer) {
            // delete previous timer
            if (pdPASS != xTimerDelete(timer->timer, BLOCK_INDEFINITELY)) {
                status = aqSpilTmrStatusError;
            }
        }
        if (aqSpilTmrStatusOK == status) {
            timer->timer = xTimerCreateStatic(NULL, pdMS_TO_TICKS(periodInUsecs/1000U), isPeriodic,
                    timer, vTimerCallback, &timer->xTimerBuffer);
            if (NULL == timer->timer) {
                status = aqSpilTmrStatusError;
            } else {
                if (pdPASS != xTimerStart(timer->timer, 0)) {
                    status = aqSpilTmrStatusError;
                }
            }
        }
    }
    return status;
}

/*****************************************************************************
 *
 *  Function Name:      aqSpilTmrStop
 *
 *  Description:        Stop a timer
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    timer
 *                         FIX_ME <detailed value information>
 *
 *  Output Arguments:   None
 *
 *  Return Value:       aqSpilTmrStatus_t
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   tmrPool
 *                      tmrPoolMutex
 *
 *  Comments:           None
 *
 *****************************************************************************/
aqSpilTmrStatus_t aqSpilTmrStop(aqSpilTmr_t *timer) {
    aqSpilTmrStatus_t status = aqSpilTmrStatusOK;
    if ((NULL == timer) || (NULL == timer->timer)) {
        status = aqSpilTmrStatusError;
    } else {
        if (pdPASS != xTimerStop(timer->timer, BLOCK_INDEFINITELY)) {
            status = aqSpilTmrStatusError;
        }
    }
    return status;
}

/*****************************************************************************
 *
 *  Function Name:      aqSpilTmrFree
 *
 *  Description:        Free a timer resource
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    timer
 *                         FIX_ME <detailed value information>
 *
 *  Output Arguments:   None
 *
 *  Return Value:       aqSpilTmrStatus_t
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   tmrPool
 *                      tmrPoolMutex
 *
 *  Comments:           None
 *
 *****************************************************************************/
aqSpilTmrStatus_t aqSpilTmrFree(aqSpilTmr_t *timer) {
    aqSpilTmrStatus_t status = aqSpilTmrStatusOK;

    VITAL_IS_OK(aqSpilMtxLock(&tmrPoolMutex));
    if ( (NULL == timer)         // FIX_ME add check for isValidTmrHandle()
         || (NULL == timer->sem)  ) {
        status = aqSpilTmrStatusError;
    } else {
        timer->sem = NULL;
    }
    VITAL_IS_OK(aqSpilMtxUnlock(&tmrPoolMutex));

    return status;
}

/*****************************************************************************
 *
 *  Function Name:      aqSpilMqAllocate
 *
 *  Description:        Allocate a message queue
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    width
 *                         FIX_ME <detailed value information>
 *                      depth
 *                         FIX_ME <detailed value information>
 *                      sem
 *                         FIX_ME <detailed value information>
 *
 *  Output Arguments:   None
 *
 *  Return Value:       aqSpilMq_t *
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   mqTable
 *
 *  Comments:           None
 *
 *****************************************************************************/
aqSpilMq_t *aqSpilMqAllocate(uint32_t width, uint32_t depth, uint8_t *storage, aqSpilSem_t *sem) {
    // ASSUME: Q allocation is done at init time.  If done after threading has started, a mutex will need to be
    // added to the message queue table.
    static uint32_t mqAllocatedCount=0;

    uint32_t                    size = width * depth;
    aqSpilMq_t                 *mq = NULL;

    if ((0U != width) && (0U != depth)) {
        if (mqAllocatedCount < theResources->numQueues) {
            mq = &mqTable[mqAllocatedCount];
            mqAllocatedCount++;
            mq->mutex = xSemaphoreCreateRecursiveMutex();
            mq->queue = xQueueCreateStatic(depth, width, storage, &mq->xQueueBuffer);
            if ((NULL == mq->queue) || (NULL == mq->mutex)) {
                SLOG_E("MQ mutex_init failure");
                mq = NULL;
            } else {
                mq->sem = sem;
            }
        }
    }
    return mq;
}

/*****************************************************************************
 *
 *  Function Name:      aqSpilMqEnqueue
 *
 *  Description:        Enqueue an element to a message queue
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    msgQ
 *                         FIX_ME <detailed value information>
 *                      elementAddr
 *                         FIX_ME <detailed value information>
 *
 *  Output Arguments:   None
 *
 *  Return Value:       aqSpilMqStatus_t
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   mqTable
 *
 *  Comments:           None
 *
 *****************************************************************************/
aqSpilMqStatus_t aqSpilMqEnqueue(aqSpilMq_t *msgQ, cvar_t elementAddr) {
    aqSpilMqStatus_t rvalue = aqSpilMqStatusOK;
    if ((NULL == msgQ) || (NULL == elementAddr)) {
        rvalue = aqSpilMqStatusError;
    } else {
        if (pdTRUE == xSemaphoreTakeRecursive(msgQ->mutex, BLOCK_INDEFINITELY)) {
            UBaseType_t count = uxQueueMessagesWaiting(msgQ->queue);
            if ((0 == uxQueueSpacesAvailable(msgQ->queue)) ||
                    (pdTRUE != xQueueSendToBack(msgQ->queue, elementAddr, 0))) { // Should not block
                rvalue = aqSpilMqStatusError;
            } else {
                if ((0U == count) && (NULL != msgQ->sem)) {
                    // The queue was empty before. Signal sem to wake waiting task
                    if (aqSpilSemStatusOK != aqSpilSemPost(msgQ->sem)) {
                        rvalue = aqSpilMqStatusError;
                    }
                }
            }
            ABQ_EXPECT(pdTRUE == xSemaphoreGiveRecursive(msgQ->mutex));
        }
    }
    return rvalue;
}

/*****************************************************************************
 *
 *  Function Name:      aqSpilMqDequeue
 *
 *  Description:        Dequeue an element from a message queue
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    msgQ
 *                         FIX_ME <detailed value information>
 *
 *  Output Arguments:   elementAddr
 *                         FIX_ME <detailed value information>
 *
 *  Return Value:       aqSpilMqStatus_t
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   mqTable
 *
 *  Comments:           None
 *
 *****************************************************************************/
aqSpilMqStatus_t aqSpilMqDequeue(aqSpilMq_t *msgQ, void *elementAddr) {
    aqSpilMqStatus_t rvalue = aqSpilMqStatusOK;
    if ((NULL == msgQ) || (NULL == elementAddr)) {
        rvalue = aqSpilMqStatusError;
    } else {
        if (pdTRUE == xSemaphoreTakeRecursive(msgQ->mutex, BLOCK_INDEFINITELY)) {
            if ((0 == uxQueueMessagesWaiting(msgQ->queue)) ||
                    (pdTRUE != xQueueReceive(msgQ->queue, elementAddr, 0))) { // Should not block
                rvalue = aqSpilMqStatusError;
            }
            ABQ_EXPECT(pdTRUE == xSemaphoreGiveRecursive(msgQ->mutex));
        }
    }
    return rvalue;
}

/*****************************************************************************
 *
 *  Function Name:      aqSpilMqGetEntries
 *
 *  Description:        Get the number of entries enqueued in a message queue
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    msgQ
 *                         FIX_ME <detailed value information>
 *
 *  Output Arguments:   numOfEntries
 *                         FIX_ME <detailed value information>
 *
 *  Return Value:       aqSpilMqStatus_t
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   mqTable
 *
 *  Comments:           None
 *
 *****************************************************************************/
aqSpilMqStatus_t aqSpilMqGetEntries(aqSpilMq_t *msgQ, uint32_t *numOfEntries) {
    aqSpilMqStatus_t rvalue = aqSpilMqStatusOK;
    if ((NULL == msgQ) || (NULL == numOfEntries)) {
        rvalue = aqSpilMqStatusError;
    } else {
        if (pdTRUE == xSemaphoreTakeRecursive(msgQ->mutex, BLOCK_INDEFINITELY)) {
            *numOfEntries = uxQueueMessagesWaiting(msgQ->queue);
            ABQ_EXPECT(pdTRUE == xSemaphoreGiveRecursive(msgQ->mutex));
        }
    }
    return rvalue;
}

/*****************************************************************************
 *
 *  Function Name:      aqSpilMqFlush
 *
 *  Description:        Flush all entries from a message queue
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    msgQ
 *                         FIX_ME <detailed value information>
 *
 *  Output Arguments:   None
 *
 *  Return Value:       aqSpilMqStatus_t
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   mqTable
 *
 *  Comments:           None
 *
 *****************************************************************************/
aqSpilMqStatus_t aqSpilMqFlush(aqSpilMq_t *msgQ) {
    aqSpilMqStatus_t rvalue = aqSpilMqStatusOK;
    if (NULL == msgQ) {
        rvalue = aqSpilMqStatusError;
    } else {
        if (pdTRUE == xSemaphoreTakeRecursive(msgQ->mutex, BLOCK_INDEFINITELY)) {
            ABQ_EXPECT(pdPASS == xQueueReset(msgQ->queue));
            ABQ_EXPECT(pdTRUE == xSemaphoreGiveRecursive(msgQ->mutex));
        }
    }
    return rvalue;
}

/*****************************************************************************
 *
 *  Function Name:      aqSpilAopAtomic
 *
 *  Description:        Execute a function atomically
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    func
 *                         FIX_ME <detailed value information>
 *                      cbdata
 *                         FIX_ME <detailed value information>
 *                      cblen
 *                         FIX_ME <detailed value information>
 *
 *  Output Arguments:   None
 *
 *  Return Value:       int32_t
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   thrTable
 *                      isoSystemPipe
 *
 *  Comments:           This function may still be interrupted by signals
 *
 *****************************************************************************/
int32_t aqSpilAopAtomic(aqSpilAopAtomicFunc func, cvar_t cbdata) {
    int32_t result = EUNSPECIFIED;
    if (NULL != func) {
        if (aqSpilMtxStatusOK == aqSpilMtxLock(&atomicMutex)) {
            result = func(cbdata);
            ABQ_EXPECT(aqSpilMtxStatusOK == aqSpilMtxUnlock(&atomicMutex));
        }
    }
    return result;
}

int32_t aqSpilDiaGetClientPriority(void) {
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    // TODO return (int32_t) clientThread.priority;
    return -1;
}

#ifdef AQ_ONGS_STATISTICS
void aqSpilRsStartExecTiming(aqSpilRsThrID_t mask, bool_t discard) {
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
}

void aqSpilRsStopExecTiming(aqSpilRsThrID_t mask) { printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__); }
#endif // ifdef AQ_ONGS_STATISTICS

/*****************************************************************************
 *
 *  Function Name:      initSpil
 *
 *  Description:        Initialize the OS isolation layer
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    unused
 *                         FIX_ME
 *
 *  Output Arguments:   None
 *
 *  Return Value:       cvar_t
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   semTable
 *                      semListTable
 *                      tmrPool
 *                      mqTable
 *                      isoSystemPipe
 *                      FIX_ME <list other global variables>
 *
 *  Comments:           Called from temporary helper thread to minimize impact
 *                      of excessive stack space usage due to pipe creation
 *
 *****************************************************************************/

/** @private
 *  @brief Initialize the SPIL's objects.
 *  Initialize all threads, semaphores, semaphore lists, and timers that
 *  were passed into the SPIL at the aqSpilBindOs API call.
 *
 * @param pStatus This is a pointer to a aqSpilInitStatus_t. If not NULL,
                  the initialization result code will be populated here.
 * @return FIXME -- BAD MISRA form.  casting a arithmetic type to a pointer.
 */
static err_t initSpil(void) {
    err_t err = EXIT_SUCCESS;
    aqSpilSem_t *ste = NULL;
    aqSpilMtx_t *mxe = NULL;
    uint16_t i = 0;

    uint16_t mtx_idx = 0;
    uint16_t tmr_idx = 0;

    // initialize semaphore data structures
    ste = semTable;
    for (i = 0U; i < theResources->numSemas; i++) {
        ste->sem = xSemaphoreCreateBinary();
        if (NULL == ste->sem) {
            err = ENOMEM;
            break;
        }
        ste++;
    }

    if (EXIT_SUCCESS == err) {
        for (i = 0U; i < theResources->numSemLists; i++) {
            semListTable[i].list = xQueueCreateSet(100); // TODO-FREERTOS
            if (NULL == semListTable[i].list) {
                err = ENOMEM;
                break;
            }
            semListTable[i].in_use = false;
        }
    }
    if (EXIT_SUCCESS == err) {
        for( tmr_idx = 0U; tmr_idx < theResources->numTimerz; tmr_idx++ ) {
            tmrPool[tmr_idx].sem = NULL;
        }
    }
    // initialize mutex table
    if (EXIT_SUCCESS == err) {
        for (mtx_idx = 0U; mtx_idx < theResources->numMutexes; mtx_idx++) {
            mxe = &mtxTable[mtx_idx];
            mxe->mtx = xSemaphoreCreateRecursiveMutex();
            if (NULL == mxe->mtx) {
                err = ENOMEM;
                break;
            }
        }
    }

    if (EXIT_SUCCESS == err) {
        semTableMutex.mtx = xSemaphoreCreateRecursiveMutex();
        tmrPoolMutex.mtx = xSemaphoreCreateRecursiveMutex();
        atomicMutex.mtx = xSemaphoreCreateRecursiveMutex();
        if ((NULL == semTableMutex.mtx) || (NULL == atomicMutex.mtx) ||
                (NULL == tmrPoolMutex.mtx)) {
            err = ENOMEM;
        }
    }
    return err;
}

/** @private
 *  @brief Find thread table handle for calling thread
 *
 *  ## Global's Affected
 *  * thrTable
 *
 * @return A pointer to thread's thread control block.
 */
aqTcb_t *thisThread(void) {
    aqTcb_t *thisEntry = NULL;

    if (NULL != theResources) { /* during Unit tests, 'theResources' may not be set */
        TaskHandle_t thisThreadHandle = xTaskGetCurrentTaskHandle();
        thisEntry = &thrTable[theResources->numThreads];
        // find correct entry in thread table
        while (thisEntry-- > thrTable) {
            // The threads were created statically, so the handle is the address of the xTaskBuffer
            // Using thisEntry->xHandle has a slight chance of race condition before the TaskCreateStatic
            // returns the handle.
            if (&thisEntry->xTaskBuffer == thisThreadHandle) {
                break;
            }
        }
    }
    return thisEntry;
}

err_t aqSpilGetIfAddr(const char *if_name, char *addr, uint8_t addr_sz) {
    err_t retval = EINVAL;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    retval = EXIT_SUCCESS;
    return retval;
}

const char *aqSpilGetDeviceAddr(void) {
    // would like to make return to aqSpilGetDeviceAddr a const char* ... but
    // items that need to consume this were created earlier with char*
    // expected.  Thus, double declaring so the cached copy remains untouched
    // ASSUMING something isn't trashing memory!
    static char device_addr[SPIL_DEVICE_ADDR_BUF_SZ] = { 0 };
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    // return the address
    return device_addr;
}

number_t cputime(void) {
    number_t rvalue = 0.;
    TaskStatus_t xTaskDetails;
    if (NULL != main_task_handle) {
        vTaskGetInfo(main_task_handle, &xTaskDetails, pdFALSE, eRunning);
        rvalue += (number_t)xTaskDetails.ulRunTimeCounter;
    }

    size_t total_num = IMPL_RESERVED_THREAD_NUM + theResources->numWorkerThreads +
            theResources->numWorkerThreads;
    for (uint16_t i = 0; i < total_num; i++) {
        aqTcb_t *entry = &thrTable[i];
        if (NULL != entry->xHandle) {
            vTaskGetInfo(main_task_handle,
                    &xTaskDetails,
                    pdFALSE,
                    eRunning); // any value other than eInvalid prevents geting the real state, to save time
            rvalue += (number_t)xTaskDetails.ulRunTimeCounter;
        }
    }
    return rvalue;
}
