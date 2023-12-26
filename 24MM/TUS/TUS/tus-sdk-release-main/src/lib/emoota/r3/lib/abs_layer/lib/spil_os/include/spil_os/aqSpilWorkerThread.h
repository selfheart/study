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

//
// Created by DQuimby on 3/16/17.
//

#ifndef SPIL_OS_AQSPILWORKERTHREAD_H
#define SPIL_OS_AQSPILWORKERTHREAD_H

#include <spil_os/aqSpil.h>

/** @file
 *
 * @startuml {spilWorkerThreadState.png}
 * title Worker thread State Diagram
 *
 * state "Idle" as idle
 * state "Running" as run
 * [*] -> idle
 * idle --> run : start /\nonStart()
 * run --> run : null /\ndoWork()
 * run --> idle : stop /\nonStop()
 * @enduml
 *
 */


/** @public
 *  @brief Valid states for worker threads held by the SPIL worker thread pool.
 */
typedef enum {
    WT_INIT=0,     ///< Worker thread is initializing.
    WT_IDLE,       ///< Worker thread is in the idle state.
    WT_ALLOCATED,  ///< Worker thread has been allocated, but hasn't not been started yet.
    WT_START,      ///< Worker thread is starting
    WT_RUN,        ///< Worker thread is currently running
    WT_STOP        ///< Worker thread has been commanded to stop, however call to onStop() has not completed yet.
} workerThreadState_t;

/** @public
 *  @brief Event flags that are utilized by the worker thread and/or bound IO objects.
 */
typedef uint32_t workerThreadEvent_t;
///< No events are currently being flagged
#define WT_EVENT_NONE           (0U)
///< When this bit is set, there is input ready for the bound input device
#define WT_EVENT_INPUT_READY    (0x00000001U)
///< When this bit is set, there is output ready for the bound output device
#define WT_EVENT_OUTPUT_READY   (0x00000002U)
///< When this bit is set, all worker threads should gracefully exit.
#define WT_EVENT_SHUTDOWN       (0x40000000U)
///< When this bit is set, there is output ready for the bound output device
#define WT_EVENT_MASK (WT_EVENT_INPUT_READY | WT_EVENT_OUTPUT_READY | WT_EVENT_SHUTDOWN)

/** @public
 *  @class workerThread_t
 *  @brief Public handle used by applicaiton to manage worker threads in conjuction with the
 *         worker thread "member functions".
 */
typedef const struct workerThread workerThread_t;

/** @public
 *  @memberof workerThread_t
 *  @brief The prototype used for the "member functions" onStart(), onStop() and doWork().
 */
typedef void (*workerThreadCb_t)(workerThread_t *workerThread, cvar_t workerData);

/** @private
 *
 *  @brief The underlying data structure for the worker thread object.
 */
struct workerThread {
    workerThreadCb_t onStart;  ///< Callback registered to be called after worker thread has started.
    workerThreadCb_t doWork;   ///< Callback registered to be called continiously while worker thread is in WT_RUN.
    workerThreadCb_t onStop;   ///< Callback registered to be called after worker thread has been commanded to stop.
    workerThreadState_t state; ///< Current state of the worker thread.
    aqSpilSem_t *sem;          ///< Semaphore used to wake up the worker thread on external events.
    aqTcb_t *tcb;              ///< The Worker thread's Task Control Block (TCB)
    workerThreadEvent_t events;           ///< Events that have been signaled to the worker thread.
    bool_t workerUsesSpilIo;   ///< Indication of whether or not the worker thread uses SPIL IO calls.
    cvar_t workerData;  ///< This is pointer to an object that the worker maintains.
    aqSpilMq_t *queue;         ///< Message queue that has been bound to this worker thread
    uint32_t workerThreadNum;   ///< Debug info.  Easier to follow than an address;
};



/** @public
 *  @memberof workerThread_t
 *  @brief Allocates an idle worker thread from the worker thread pool.
 *
 *  @param threadName The name of the thread.  NOTE: The name will be truncated to fit within
 *                    the space allocated for name in the task control block.
 *  @param workerData The data that is consumed/produced by the worker thread.  This will be
 *                 applicaiton dependent and is not used by either SPIL or the worker thread infastructure.
 *                 The information is simply held and passed to all the worker thread member functions.
 *  @param isWorkerUseSpilIo Instucts indicates if this worker is bound to a SPIL io stream.  If not, the
 *                 worker thread executes every 10ms until stopped, if bound to a SPIL io stream, the
 *                 thread is notified when IO is available via the bound io stream interface.
 *                 TODO: does this need to be included as part of allocate?  Allocate should probably
 *                 set the underlying member to false, and then set to true if an io stream gets bound later (e.g.
 *                 between allocate and start).
 *  @param onStart The callback function which will be called within the worker threads context
 *                 immediately after startWorkerThread is called for the allocated worker thread.
 *  @param doWork The callback function called by the worker thread body after the onStart callback
 *                has been called.  doWork() will be called continiously until the worker thread
 *                instance is commanded to stop via the stopWorkerThread method.  This thread should
 *                either block on a aqSpil_os semaphore or sleep operation to prevent the worker thread
 *                from over-consuming CPU.
 * @param onStop The callback function that will be called after the workerThread_t instance was commanded
 *               to stop via the stopWorkerThread method.  If the provided callback is NULL, then no
 *               callback will be called when the worker thread transitions from the WT_RUN state back
 *               to the WT_IDLE state.  Once the worker thread is stopped and in the WT_IDLE state, it
 *               will be returned to the worker thread pool.
 *
 * @retval NULL indicates that there are not any worker threads available in the thread pool.
 * @retval non-NULL value is a pointer to a worker thread instance which has been allocated and
 *         can be started and stopped via the provided methods.
 */
extern workerThread_t * allocWorkerThread( const char *threadName,
                                           cvar_t workerData,
                                           bool_t isWorkerUseSpilIo,
                                           workerThreadCb_t onStart,
                                           workerThreadCb_t doWork,
                                           workerThreadCb_t onStop);

/** @public
 *  @memberof workerThread_t
 *  @brief Indicates whether the given worker thread instance is running.
 *
 * @retval aqTRUE indicates the worker thread is running.
 * @retval aqFALSE indicates the worker thread is not running.
 */
extern bool_t isWorkerThreadRunning( const workerThread_t *worker );

/** @public
 *  @memberof workerThread_t
 *  @brief Start a worker thread
 *
 * @param worker The worker thread instance to start.
 *
 */
extern err_t startWorkerThread( workerThread_t *worker );

/** @public
 *  @memberof workerThread_t
 *  @brief Stop a worker thread
 *
 *  Should not be called from inside the worker thread run context.
 *
 * @param worker The worker thread instance to stop.
 */
extern err_t stopWorkerThread( workerThread_t *worker );

/** @protected
 * @brief The entry point for a worker thread.  This is the entry point which will be passed to the
 *        underlying system OS thread.
 * @param arg The argument that is to be passed to the the worker thread's entry point.  This is used
 *            to handle system start-up synchronization between the SPIL API and SPIL kernel.  Otherwise
 *            unused during normal operation.
 */
extern void workerThreadEntry( cvar_t args );

/** @protected
 *  @brief Initialize the worker thread objects.  This needs to be called after the SPIL kernel thread has started
 *         and before any worker threads are allocated via allocateWorkerThread().  This is done from within
 *         the call to aqSpilBindOs() and should not be called anywhere else.  This method is externed simply
 *         because aqSpilOs needs to see it ... is there a better way to hide it's existence?
 *  @param theResources The list of static system resouces that were passed into aqSpilBindOS().  This method
 *         initializes each of the workerThread_t objects that are contained within that structure and allocates
 *         the worker thread stacks that were also passed in via that structure.
 */
extern void initWorkerThreads( const aqSpilResources_t *theResources );


/** @public
 *  @memberof workerThread_t
 *  @brief Set bits of the worker thread's event flag using the given event flags.
 *
 * @param worker The worker thread who's event flags will be modified.
 * @param setEvents The event flags which will be applied to the worker thread's
 *                  event flags.  NOTE: each bit may represent an underlying event flag.
 */
extern void workerThreadSetEvents( workerThread_t *worker, workerThreadEvent_t setEvents );

/** @public
 *  @memberof workerThread_t
 *  @brief Clear bits of the worker thread's event flag using the given event flags.
 *
 * @param worker The worker thread who's event flags will be modified.
 * @param setEvents The event flags to be cleared of the worker thread's event flags.
 *                  NOTE: each bit may represent an underlying event flag.
 */
extern void workerThreadClearEvents( workerThread_t *worker, workerThreadEvent_t clearEvents );

/** @public
 *  @memberof workerThread_t
 *  @brief Get the current value of the worker thread's event flags.
 *
 *  @param worker The worker thread who's event flags will be fetched.
 *  @param setEvents The event flags of the worker thread's event flags which will be cleared.
 *                  NOTE: each bit may represent an underlying event flag.
 *
 *  @return The current value the worker thread's event flags.
 */
extern workerThreadEvent_t workerThreadGetEvents( workerThread_t *worker );

/** @public
 *  @memberof workerThread_t
 *  @brief Bind a SPIL OS Queue to a worker thread.
 *  @param worker
 *  @param queue
 *  @return
 */
extern err_t workerThreadBindQueue( workerThread_t *worker, aqSpilMq_t *queue );

/** @public
 *  @memberof workerThread_t
 *  @brief Release the queue that is bound to the worker thread.
 *  @param worker
 *  @return
 */
extern err_t workerThreadReleaseQueue( workerThread_t *worker );


/** @public
 *  @memberof workerThread_t
 *  @brief Enqueue a message to the message queue that is bound to the given worker thread.
 *
 *  When the message is enqueued, the worker worker thread who's bound to this queue will
 *  become runnable.
 *
 *  NOTE: When the worker thread is stopped, the association between the message queue and the
 *  worker thread will be broken.  The message queue will then be available to bind to another
 *  worker thread.
 *
 * @param queue
 * @return
 */
extern err_t workerThreadEnqueue( workerThread_t *worker, const void *element );

/** @public
 *  @memberof @workerThread_t
 *
 *  If there are no data elements available in the queue, the worker thread calling this
 *  method will be blocked.
 *
 *  NOTE: This method should be called from within the contest of the doWork() method
 *  callback for of a worker thread object.
 *
 *  TODO: Timeout?
 */
extern err_t workerThreadDequeue( workerThread_t *worker, void *element );

#endif //SPIL_OS_TEST_AQSPILWORKERTHREAD_H
