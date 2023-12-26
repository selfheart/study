#ifndef AQSPILMPL_H
#define AQSPILMPL_H

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
 *  Copyright (c) 2017 Airbiquity Inc.  All rights reserved.
 *
 *****************************************************************************/
#include <spil_os/aqSpil.h>
#include <spil_os/aqSpilWorkerThread.h>

/* Outside interface definitions */

/** @brief the total space including NULL termination character that is allocated for
 * SPIL object names.
 *
 * TODO: Add names to other SPIL objects.  Currently only the TCB has a name property.
 */
#define SPIL_OBJECT_NAME_SZ ( 10u )

/** @private
 *
 */
typedef void (*thrEntry)(cvar_t);
typedef err_t (*initFunc_t) (cvar_t);

/*****************************************************************************
 *
 *  Timestamp, Timer and Delay Services
 *
 *****************************************************************************/

typedef int64_t aqSpilTvSec_t;
typedef int64_t aqSpilTvNsec_t;

/** @private
 *
 */
struct aqSpilClkContinuousTime {
    // normalize timespec for both OSX/Linux.
    // TODO-FREERTOS: There is really no need to have tv_sec and tv_nsec.
    // in freeRTOS, it would be a lot easier to just have tv_msec
    // However other code is depending on this definition here.
    aqSpilTvSec_t tv_sec;    // OSX was unsigned int, Linux was long int
    aqSpilTvNsec_t tv_nsec;  // OSX was int, Linux was long int
};

/** @private
 *
 */
struct aqSpilClkRtc {
    aqSpilClkContinuousTime_t rtc;
};

// Macros which provide total system resources needed to accommodate for both the client requirements
// and the needs of the SPIL itself
#ifndef IMPL_RESERVED_THREAD_NUM
#warning "Missing platform target build configuration"
#define IMPL_RESERVED_THREAD_NUM (0U)
#endif

/** @public
 * @brief Calculate the total number of threads that need to be allocated
 *        at compile time by the application.
 *
 *  The calculation takes into account the number of threads that SPIL needs
 *  internally plus the number of threads that are required by the application.
 *
 *  @param numClientThreads The number of threads that are needed by the
 *  application.
 */
#define SPIL_NUM_THREADS( numClientThreads , numWorkerThreads ) ( ( numClientThreads ) + ( numWorkerThreads ) + IMPL_RESERVED_THREAD_NUM)

/** @public
 * @brief Calculate the total number of semaphores that need to be allocated
 *        at compile time by the application.
 *
 *  The calculation takes into account the number of semaphores that SPIL needs
 *  internally plus the number of semaphores that are required by the application.
 *
 *  @param numClientSemas The number of semaphores that are needed by the
 *  application.
 */
#define SPIL_NUM_SEMAS( numClientSemas , numWorkerThreads ) ( ( numClientSemas ) + ( numWorkerThreads ) + 2U)

/** @public
 * @brief Calculate the total number of semaphore lists that need to be allocated
 *        at compile time by the application.
 *
 *  The calculation takes into account the number of semaphores lists that SPIL needs
 *  internally plus the number of semaphore lists that are required by the application.
 *
 *  @param numClientSemaSemLists The number of semaphore lists that are needed by the
 *  application.
 */
#define SPIL_NUM_SEMA_LISTS( numClientSemLists ) ( ( numClientSemLists ) + 2U)

/** @public
 * @brief Calculate the total number of message queues that need to be allocated
 *        at compile time by the application.
 *
 *  The calculation takes into account the number of message queues that SPIL needs
 *  internally plus the number of message queues that are required by the application.
 *
 *  @param numClientQueues The number of message queues that are needed by the
 *  application.
 */
#define SPIL_NUM_QUEUES( numClientQueues , numWorkerThreads ) ( ( numClientQueues ) + ( numWorkerThreads ) + 2U)

/** @public
 * @brief Calculate the total number of timers that need to be allocated
 *        at compile time by the application.
 *
 *  The calculation takes into account the number of timers that SPIL needs
 *  internally plus the number of timers that are required by the application.
 *
 *  @param numClientSemas The number of timers that are needed by the
 *  application.
 */
#define SPIL_NUM_TIMERZ( numClientTimerz ) ( ( numClientTimerz ) + 2U)

/** @public Calculate the total number of worker threads  that need to be allocated at
 *          compile time by the applicaiton.
 *
 *  The calculation takes into account the number of mutexes that SPIL needs
 *  internally plus the number of mutexes that are required by the application.
 *
 *  @param numMutexes The number of timers that are needed by the
 *         application
 */
#define SPIL_NUM_WORKERS( numWorkers ) ( ( numWorkers ) > 0U ? ( numWorkers ) : 1U )

/** @public Calculate the total number of muteses that need to be allocated at
 *          compile time by the applicaiton.
 *
 *  The calculation takes into account the number of mutexes that SPIL needs
 *  internally plus the number of mutexes that are required by the application.
 *
 *  @param numMutexes The number of timers that are needed by the
 *         application
 */
#define SPIL_NUM_MUTEXES( numMutexes ) ( ( numMutexes ) + 1U )


/** @public Calculate the total number words needed for worker thread stack space.
 *
 *  If zero worker threads are needed, this will still cause 1 word to be allocated to
 *  prevent attempting to zero init a 0 byte array (which fails under gcc).
 *
 *  @param numWorkers The number of worker threads that are needed by the applicaiton.
 *  @param workerStackSz The number of bytes needed for each worker thread stack.
 */
#define SPIL_WORKER_THREAD_STACK_SZ( numWorkers, workerStackSz ) \
   ( ( numWorkers ) < 1U ) ? 4U :  ( numWorkers ) *  ( ( ( workerStackSz ) + 3U ) / 4U )

/** @public
 * @brief Thread attributes for application threads that will be created by the
 *        SPIL when the SPIL is bound to the applicaiton via aqSpilBindOs.
 */
typedef struct {
    /** @brief The stack address that the SPIL should assign to the application thread */
    byte_t *stackAddr;

    /** @brief The number of bytes that were allocated at compile time to the thread's stack. */
    size_t stackSize;

    /** @brief The priority that the SPIL is to assign to the application thread.
     *
     * @todo The priority should be portable rather than OS specific.
     *
     * #### NOTE: Ubuntu works better if real time threads aren't used.  Thus,
     * Linux standard threding is used  unles the precompiler definition
     * RT_THREADS is not defined.
     */
    int32_t priority;

    /** @brief The thread entry function */
    thrEntry entryPoint;

    /** @brief The thread name.
     *
     * This doesn't need to be populated, however, it makes stepping through the SPIL
     * kernel thread table simpler to understand.
     */
    char name[SPIL_OBJECT_NAME_SZ];
} aqSpilThreadAttr_t;


/** @public
 *  @brief This structure is used to provide SPIL resources that are
 *         allocated at compile time by the application to the SPIL
 *         kernel.
 */
struct spilResources {
    uint8_t numThreads;  ///< The number of threads TCBs being passed to the SPIL (includs internal, app and worker)
    aqTcb_t *tcbs;       ///< An array of thread TCBs being passed to the kernel (includes internal, app and worker)
    uint16_t numWorkerThreads; ///< The number of worker threads requested.
    uint16_t numAppThreads; ///< The number of application threads (that aren't worker threads).
    uint32_t workerThreadStackSz; ///< The size used for all worker thread stacks.
    int32_t *workerThreadStacks;
    struct workerThread *workerThreads;

    /** @brief The thread attributes that are being passed to the kernel.
     *
     *  This list is should only include the attributes for the application TCBs
     *  that are being passed into the kernel.
     */
    aqSpilThreadAttr_t *threadAttrs;

    /** @brief The number of semaphores being passed to teh SPIL by the application. */
    uint16_t numSemas;

    /** @brief An array of SPIL semaphores that are created by the application at compile time.
     *
     * The semaphores will be initialized and managed by the SPIL kernel.  The array must
     * contain at least 'numSemas' semaphores.
     */
    aqSpilSem_t *semas;

    /** @brief The number of semaphore lists being passed to teh SPIL by the application. */
    uint16_t numSemLists;

    /** @brief An array of SPIL semaphore lists that are created by the application at compile time.
     *
     * The semaphore lists will be initialized and managed by the SPIL kernel.  The array must
     * contain at least 'numSemLists' semaphore lists (passing in extra wastes memory.  passing
     * in too few is catastrophic).
     */
    aqSpilSemList_t *semaLists;


    /** @brief The number of SPIL semaphore lists being passed to teh SPIL by the application. */
    uint16_t numQueues;

    /** @brief An array of SPIL message queues being passed to the SPIL by the application.
     *
     * The array must contain at least 'numQueues' message queues.
     */
    aqSpilMq_t *queues;

    /** @brief The number of SPIL timer objects being passed into the SPIL by the application */
    uint16_t numTimerz;

    /** @brief An array of SPIL timer objects being passed into the SPIL by the application.
     *
     * The array of timers must contain at least 'numTimerz' timers.
     */
    aqSpilTmr_t *timerz;

    uint16_t numMutexes;
    aqSpilMtx_t *mutexes;

    /** @brief Function that allocates the application's  SPIL resources after the SPIL is initialized. */
    initFunc_t allocFunc;

    /** @brief Argument that the SPIL will pass to the allocFunc callback */
    cvar_t allocFuncArg;

};

#define makeResources( theResources, appThreadCount, workerThreadCount, stackSz, appSemaCount, appSemListCount, appQueueCount, appTimerzCount ) \
static aqTcb_t theResources ##_theTcbs[SPIL_NUM_THREADS( ( appThreadCount ), ( workerThreadCount ) ) ] = {0}; \
static int32_t theResources ##_theWorkerStacks[ ( workerThreadCount ) *  ( ( ( stackSz ) + 3 ) / 4 )] = {0}; \
static aqSpilSem_t theResources ##_theSems[ SPIL_NUM_SEMAS( ( appSemaCount ), ( workerThreadCount ) ) ] = {0}; \
static aqSpilSemList_t theResources ##_theSemaLists[ SPIL_NUM_SEMA_LISTS( ( appSemListCount ) ) ] = {0}; \
static aqSpilMq_t theResources ##_theQueues[ SPIL_NUM_QUEUES( ( appQueueCount ), ( workerThreadCount ) ) ] = {0}; \
static aqSpilTmr_t theResources ##_theTimerz[ SPIL_NUM_TIMERZ( appTimerzCount )] = {0}; \
static workerThread_t theResources ##_theWorkerThreads[ ( workerThreadCount ) ] = {0}; \
static aqSpilResources_t theResources = { \
    .numThreads = SPIL_NUM_THREADS( ( appThreadCount ), ( workerThreadCount ) ), \
    .tcbs = theResources ##_theTcbs, \
    .numWorkerThreads = workerThreadCount, \
    .numAppThreads = appThreadCount, \
    .workerThreadStackSz = stackSz, \
    .workerThreadStacks = theResources ##_theWorkerStacks, \
    .workerThreads = theResources ##_theWorkerThreads, \
    .threadAttrs = NULL, /* TODO: Needs to be filled by caller */ \
    .numSemas = SPIL_NUM_SEMAS( ( appSemaCount ), ( workerThreadCount ) ), \
    .semas = theResources ##_theSems, \
    .numSemLists = SPIL_NUM_SEMA_LISTS( ( appSemListCount ) ), \
    .semaLists = theResources ##_theSemaLists, \
    .numQueues = SPIL_NUM_QUEUES( ( appQueueCount ), ( workerThreadCount  ) ), \
    .queues = theResources ##_theQueues, \
    .numTimerz = SPIL_NUM_TIMERZ( appTimerzCount ), \
    .timerz = theResources ##_theTimerz, \
    .numMutexes = 0, \
    .mutexes = NULL, \
    .allocFunc = NULL, /* TODO: Needs to be filled by caller */ \
    .allocFuncArg = NULL /* TODO: Needs to be filled by caller */ \
}


#define makeResourcesPlusMutexes( theResources, appThreadCount, workerThreadCount, stackSz, appSemaCount, \
                                  appSemListCount, appQueueCount, appTimerzCount, mutexCount ) \
static aqTcb_t theResources ##_theTcbs[SPIL_NUM_THREADS( ( appThreadCount ), ( workerThreadCount ) ) ] = {0}; \
static int32_t theResources ##_theWorkerStacks[ SPIL_WORKER_THREAD_STACK_SZ( workerThreadCount, stackSz ) ] = {0}; \
static aqSpilSem_t theResources ##_theSems[ SPIL_NUM_SEMAS( ( appSemaCount ), ( workerThreadCount ) ) ] = {0}; \
static aqSpilSemList_t theResources ##_theSemaLists[ SPIL_NUM_SEMA_LISTS( ( appSemListCount ) ) ] = {0}; \
static aqSpilMq_t theResources ##_theQueues[ SPIL_NUM_QUEUES( ( appQueueCount ), ( workerThreadCount ) ) ] = {0}; \
static aqSpilTmr_t theResources ##_theTimerz[ SPIL_NUM_TIMERZ( appTimerzCount )] = {0}; \
static struct workerThread theResources ##_theWorkerThreads[ ( SPIL_NUM_WORKERS( workerThreadCount ) ) ] = {0}; \
static aqSpilMtx_t theResources ##_theMutexes[ ( SPIL_NUM_MUTEXES( mutexCount ) ) ] = {0}; \
static aqSpilResources_t theResources = { \
    .numThreads = SPIL_NUM_THREADS( ( appThreadCount ), ( workerThreadCount ) ),\
    .tcbs = theResources ##_theTcbs, \
    .numWorkerThreads = workerThreadCount, \
    .numAppThreads = appThreadCount, \
    .workerThreadStackSz = stackSz, \
    .workerThreadStacks = theResources ##_theWorkerStacks, \
    .workerThreads = theResources ##_theWorkerThreads, \
    .threadAttrs = NULL, /* TODO: Needs to be filled by caller */ \
    .numSemas = SPIL_NUM_SEMAS( ( appSemaCount ), ( workerThreadCount ) ), \
    .semas = theResources ##_theSems, \
    .numSemLists = SPIL_NUM_SEMA_LISTS( ( appSemListCount ) ), \
    .semaLists = theResources ##_theSemaLists, \
    .numQueues = SPIL_NUM_QUEUES( ( appQueueCount ), ( workerThreadCount  ) ), \
    .queues = theResources ##_theQueues, \
    .numTimerz = SPIL_NUM_TIMERZ( appTimerzCount ), \
    .timerz = theResources ##_theTimerz, \
    .numMutexes = mutexCount, \
    .mutexes = theResources ##_theMutexes, \
    .allocFunc = NULL, /* TODO: Needs to be filled by caller */ \
    .allocFuncArg = NULL /* TODO: Needs to be filled by caller */ \
}

/** @public
 * @brief Initialize SPIL resource handles.
 *
 * Prior to task startup during the power-up sequence,
 * the applicaiton's main thread will call this function to map
 * SPIL resources (constants known at applicaiton compile-time),
 * to SPIL resource handles accessed by the remainder of
 * the application library.
 *
 * The expected number of resources for the applicaiton library
 * for each type of SPIL objects are alos given in the enum
 * resources. For each SPIL object type, an array
 * of that type with the values of the SPIL resources
 * needs to be passed as an argument. The lists do not
 * need to be null terminated. If no SPIL resources are
 * allocated for a particular type, then a NULL pointer
 * needs to be passed.
 *
 * In addition to the SPIL objects, the resource list passed
 * in contains the attributes for each of the applicaiton threads.
 * The attributes include stack size, stack address, and entry point.
 *
 * The final item that is passed in via the resouces list is the
 * entry point for the funciton that will allocate SPIL resources
 * after the SPIL kernel has initialized said resources.
 *
 * @startuml {spilInitSeq.png}
 * title SPIL Sample Application Initialization
 * hide footbox
 *
 * participant main as main
 * participant "Alloc\nCallback" as ac
 * participant "App\nThread\nOne" as t1
 * participant "App\nThread\nTwo" as t2
 * participant "App\nThread\nThree" as t3
 * participant "App\nThread\nFour" as t4
 * participant "SPIL API" as api
 * participant "SPIL\nTimer\nThread" as tt
 * participant "SPIL\nKernel\nThread" as kt
 *
 * main->main : create SPIL\nresources
 * main->api : aqSpilBindOs()
 * activate api
 * note left of kt
 *    The resources passed to aqSpilBindOs includes all the SPIL objects
 *    that will be managed by the kernel thread.  The objects include
 *    threads, semaphores, semaphore lists, message queues, and timers.
 *    Also a list of applicaiton thread attributes is passed in which
 *    indicates each of the thread entry functions and other per thread
 *    info.  Finally, another funciton pointer is passed in to allocate the
 *    applicaiton SPIL resources.  These resources are initialized and
 *    maintained by the OS and can't be allocated until the SPIL kernel
 *    thread is setup.
 * end note
 *
 * api-> kt : create
 * activate kt
 * kt --> api : return
 * api -> ac : allocateResources()
 * activate ac
 * note left
 *    Allocate semaphores,
 *    timers, message queues,
 *    and semaphore lists.
 * end note
 * ac --> api : return
 * deactivate ac
 *
 * api -> kt : create timer thread
 * kt -> tt : create
 * activate tt
 *
 * api-> kt : create App T1
 * kt -> t1 : create
 * activate t1
 *
 * api-> kt : create App T2
 * kt -> t2 : create
 * activate t2
 *
 * api-> kt : create App T3
 * kt -> t3 : create
 * activate t3
 *
 * api-> kt : create App T4
 * kt -> t4 : create
 * activate t4
 *
 * @enduml
 *
 * @param resources The list of SPIL resources that will be managed
 *                  by the SPIL kernel as well as thread attributes,
 *                  and the application's resource allocation entry point.
 * @return
 */
extern err_t  aqSpilBindOS(aqSpilResources_t *resources);

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
extern void aqSpilSetName(char *dst, const char *src);

/** @public
 * @brief Set the name of a SPIL thread.
 * @param thr The task control block of the thread to set the thread name.
 * @param name The name to set for the provided thread TCB.  This will be
 *             truncated if the provicded name is too big.
 */
extern void aqSpilThrSetName(aqTcb_t *thr, const char * name);
/**
  */
extern aqTcb_t     *thisThread(void);

#include "spil_os_plat.h"

#endif // _AQSPILMPL_H
