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
 *  Copyright (c) 2017 Airbiquity Inc.  All rights reserved.
 *
 * ***************************************************************************
 */
/* Outside interface definitions */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <assert.h>
#include <spil_os/aqSpilImpl.h>
#include <spil_file/spil_file.h>
#include <ontrac/ontrac/status_codes.h>
#include <ontrac/util/thread_utils.h>

/* ****************************************************************************
 *
 *  SPIL public functions
 *
 * ***************************************************************************
 * */

// comment maintained in aqSpilImple.h
err_t aqSpilBindOS(aqSpilResources_t *resources) {
    err_t retval = EXIT_SUCCESS;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    return retval;
}

/*****************************************************************************
 *
 *  Function Name:      aqSpilEntryPoint
 *
 *  Description:        Entry point for OS isolation thread
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    None
 *
 *  Output Arguments:   None
 *
 *  Return Value:       None
 *
 *  Global Variables:   isoSystemPipe
 *                      semTable
 *                      semListTable
 *                      semTableMutex
 *                      thrTable
 *                      FIX_ME <list other global variables>
 *
 *  Comments:           None
 *
 *****************************************************************************/
static void aqSpilEntryPoint(cvar_t junk) { printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__); }

/*****************************************************************************
 *
 *  Function Name:      aqSpilThrAckReady
 *
 *  Description:        Let client thread know that the thread is ready to run
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    None
 *
 *  Output Arguments:   None
 *
 *  Return Value:       aqSpilThrStatus_t
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   thrTable
 *                      FIX_ME <list other global variables>
 *
 *  Comments:           None
 *
 *****************************************************************************/
// comment maintained in aqSpil.h
aqSpilThrStatus_t aqSpilThrSleep(uint32_t periodInUs)
{
    aqSpilThrStatus_t retval = aqSpilThrStatusOK;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
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
void aqSpilSetName(char *dst, const char *src) { printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__); }

void aqSpilThrSetName(aqTcb_t *thr, const char *name) { printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__); }

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
    aqSpilSem_t *returnValue = NULL;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
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
    aqSpilSemList_t *returnValue = NULL;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
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
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
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
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
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
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
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
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
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
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
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
    aqSpilMtx_t *mutex = NULL;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
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
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
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
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    return mtx_status;
}

err_t aqSpilGetRuntimeInMillis(uint64_t *timeInMillis) {
    err_t status = EUNSPECIFIED;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
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
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    return tmr;
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
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
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
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
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
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
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
    aqSpilMq_t *mq = NULL;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
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
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
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
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
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
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
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
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
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
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    return result;
}

int32_t aqSpilDiaGetClientPriority(void) {
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    // TODO return (int32_t) clientThread.priority;
    return -1;
}

#ifdef AQ_ONGS_STATISTICS
void aqSpilRsStartExecTiming(aqSpilRsThrID_t mask, bool_t discard) { printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__); }

void aqSpilRsStopExecTiming(aqSpilRsThrID_t mask) { printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__); }
#endif // ifdef AQ_ONGS_STATISTICS

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
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    return thisEntry;
}

err_t aqSpilGetIfAddr(const char *if_name, char *addr, uint8_t addr_sz) {
    err_t retval = EINVAL;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
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
