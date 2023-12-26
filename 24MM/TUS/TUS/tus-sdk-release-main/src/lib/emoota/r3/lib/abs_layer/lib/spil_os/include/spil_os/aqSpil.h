#ifndef AQSPIL_H
#define AQSPIL_H

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

#include <spil_os/aqTime.h>

/* ****************************************************************************
 *
 *  Thread Services
 *
 * ****************************************************************************
 */

typedef struct thrTableEntry aqTcb_t;

 /** @private
  * @brief Enumeration utilized for the return value to allocateThread.
  *
  * the allocateTread() method is only used internally by the SPIL implementation during
  * the call to aqSpilBindOS().  These values are never returned outside the SPIL API.
  *
  * @see allocateThread(), aqSpilBindOS()
  *
  * @todo move into implementation instead of the public header file
  *
  */
typedef enum aqSpilThrStatus_t {
   aqSpilThrStatusOK       = 0,           ///< @brief explicitly zero.  Thread created successfully.
   aqSpilThrStatusError                   ///< @brief An error occurred while attempting to create the thread.
} aqSpilThrStatus_t;

/** @public
 * @brief Suspend current thread for fixed time period.
 *
 * #### FIXME: This method is currently a stub which always returns success without
 * actually sleeping.
 *
 * @param periodInUs The number of micro-seconds the current thread should be
 *                   suspended.  It is possible that the period will be longer
 *                   that specified depending upon other running tasks and their
 *                   relative priorities.  The thread will be suspended at least
 *                   this many micro-seconds unless interrupted.
 *
 * @return always returns <pre>aqSpilThrStatusOK</pre>
 */
extern aqSpilThrStatus_t aqSpilThrSleep(uint32_t periodInUs); // in ticks

/**
 * @public
 * @brief Compute number of milliseconds since aqSpilBindOS was invoked
 *
 * @param timeInMillis: elapsed number of milliseconds since aqSpilBindOS was invoked
 * @return aqOk if successful, else aqError
 */
extern err_t aqSpilGetRuntimeInMillis(uint64_t *timeInMillis);

/* ****************************************************************************
 *
 *  Semaphore Services
 *
 * ****************************************************************************
 */

/** @public
 *  @brief SPIL semaphore object.
 *
 * @class aqSpilSem_t
 *
 *  @startuml {SpilSemaphoreModelSequence.png}
 *  title SPIL Semaphore Model
 *  hide footbox
 *
 *  participant "Application\nThread\nOne" as t1
 *  participant "Application\nThread\nTwo" as t2
 *  participant "SPIL\nAPI" as api
 *  participant "SPIL\nKernel\nThread" as kt
 *  note over t2
 *    Assume that semaphore was allocated
 *    as described in aqSpilBindOS.
 *  end note
 *
 *  activate kt
 *  activate t2
 *  t2 -> api : aqSpilSemWait( semOne )
 *  deactivate t2
 *
 *  api -> kt : aqSpilSemWait( semOne )
 *  kt -> kt : t2 blocked\non semOne
 *
 *  ==Something happened causing thread one to post to semOne==
 *
 *  t1 -> api : aqSpilSemPost( semOne )
 *  activate api
 *  kt <-/ api : aqSpilSemPost( semOne )
 *
 *  kt -> t2 : unblock t2
 *  activate t2
 *
 *  api --> t1 : return
 *  deactivate api
 *  @enduml
 *
 */
typedef struct aqSpilSem         aqSpilSem_t;

/** @public
 *  @brief SPIL semaphore list object.
 *  @class aqSpilSemList_t
 *  @startuml {SpilSemaphoreListModelSeq.png}
 *  title SPIL Semaphore List Model
 *  hide footbox
 *
 *  participant "Application\nThread\nOne" as t1
 *  participant "Application\nThread\nTwo" as t2
 *  participant "Application\nThread\nThree" as t3
 *  participant "SPIL\nAPI" as api
 *  participant "SPIL\nKernel\nThread" as kt
 *  note over t2
 *    Assume that semaphore list one was registered by
 *    the allocate callback function that is
 *    invoked by aqSpilBindOS.  Also assume that
 *    semaphore list one (semListOne) was registered for
 *    semaphore five (semFive) and semaphore six
 *    (semSix).
 *  end note
 *
 *  activate kt
 *  activate t2
 *  t2 -> api : aqSpilSemListWait( semListOne )
 *  deactivate t2
 *
 *  api -> kt : aqSpilSemListWait( semListOne )
 *  kt -> kt : t2 blocked\non semListOne
 *
 *  ==Something happened causing thread one to post to semFive==
 *
 *  t1 -> api : aqSpilSemPost( semFive )
 *  activate api
 *  kt <-/ api : aqSpilSemPost( semFive )
 *
 *  kt -> t2 : unblock t2
 *  activate t2
 *
 *  api --> t1 : return
 *  deactivate api
 *
 *  t2 -> t2 : do some work
 *  t2 -> api : aqSpilSemListWait( semListOne )
 *  deactivate t2
 *  api -> kt : aqSpilSemListWait( semListOne )
 *  kt -> kt : block thread 2\non semListOne
 *
 *  ==Something happened causing thread three to post to semSix==
 *
 *  t3 -> api : aqSpilSemPost( semSix )
 *  activate api
 *  kt <-/ api : aqSpilSemPost( semSix )
 *
 *  kt -> t2 : unblock t2
 *  activate t2
 *
 *  api --> t3 : return
 *  deactivate api
 *
 *  t2 -> t2 : do some work
 *  t2 -> api : aqSpilSemListWait( semListOne )
 *  deactivate t2
 *  api -> kt : aqSpilSemListWait( semListOne )
 *  kt -> kt : block thread 2\non semListOne
 *
 *  @enduml
 */
typedef struct aqSpilSemList     aqSpilSemList_t;

/** @public
 * @memberof aqSpilSem_t
 * @brief Values that are returned by SPIL semaphore services aqSpilSemPost(), aqSpilSemWait() and
 * aqSemTimedWait().
 *
 * @see aqSpilSemPost(), aqSpilSemWait() and aqSemTimedWait().
 */
typedef enum aqSpilSemStatus {
   aqSpilSemStatusOK       = 0,           ///< explicitly zero.  Operation was successful
   aqSpilSemStatusTimeout,                ///< A timeout occurred while waiting on the semaphore
   aqSpilSemStatusError                   ///< A system error occurred while attempting to perform the operation.
} aqSpilSemStatus_t;

/** @public
 * @memberof aqSpilSem_t
 * @brief It increments the value of the semaphore and wakes up a blocked process waiting on the semaphore, if any.
 *
 *
 * @param sem The semaphore to increment.
 * @return <pre>aqSpilSemStatusOK</pre> == the given semaphore was successfully incremented.
 *         <pre>aqSpilSemStatusError</pre> == an error occurred while attempting to perform this operation.
 */
extern aqSpilSemStatus_t aqSpilSemPost(aqSpilSem_t *sem);

/** @public
 * @memberof aqSpilSem_t
 * @brief Allocate a semaphore from the array of semaphores that was passed in by the application
 *        at aqSpilBindOS().
 *
 * @see aqSpilBindOS()
 * @return On success a pointer to the allocated semaphore is returned.  If there are no semaphores available,
 *         or an internal error occurred, NULL will be returned instead.
 */
extern aqSpilSem_t      *aqSpilSemAllocate(void);

/** @public
 * @memberof aqSpilSemList_t
 * @brief Register a list semaphores.
 *
 * This list will be used by a thread to wait on multiple semaphores at the same time.  The semaphores
 * that are in the list being registered can't be registered by any other semaphore list.
 *
 * @param semCollection An NULL terminated array of semaphores that are to be registered to this semaphore list.
 * @return A pointer to the registered semaphore list.  If no semaphore list objects were available, or
 * an error occurred when attempting to register the list NULL will be returned instead.
 */
extern aqSpilSemList_t  *aqSpilSemRegisterList(aqSpilSem_t **semCollection);

/** @public
 * @memberof aqSpilSem_t
 * @brief Signals the given semaphore and wakes up a blocked process waiting on the semaphore, if any.
 *
 * @param sem The semaphore to signal
 * @return <pre>aqSpilSemStatusOK</pre> on success.  <pre>aqSpilSemStatusOK</pre> if an error occurred
 * attempting to signal the given semaphore.
 */
extern aqSpilSemStatus_t aqSpilSemPost(aqSpilSem_t *sem);

/** @public
 * @memberof aqSpilSem_t
 * @brief Wait on a semaphore
 *
 * The semaphore can only be waited on by a single thread at at time.  If another
 * thread is currently waiting on the given semaphore <pre>aqSpilSemStatusError</pre> will
 * be returned by this function.  If the semaphore being waited on has already been signaled without
 * a subsequent wait, the current thread will continue.  If the semaphore being waited on is not in
 * a signaled state, the current thread will be blocked until the semaphore is signaled.
 *
 * @param sem The semaphore to be waited on.
 * @return <pre>aqSpilSemStatusOk</pre> The semaphore was waited on successfully.  If the process
 * is blocked as a result of calling this method, the return code won't be available until after the
 * thread wakes back up.  <pre>aqSpilSemStatusOK</pre> An error occurred when attempting to wait on the
 * semaphore (either an internal error or perhaps the semaphore is already being waited on by another thread?).
 */
extern aqSpilSemStatus_t aqSpilSemWait(aqSpilSem_t *sem);

/** @public
 * @memberof aqSpilSemList_t
 * @brief Wait on a list of semaphores
 *
 * If any of the semaphores in the semaphore list is already in the signaled state, this call will immediately
 * return with a pointer to **a** signaled semaphore.  If none of the semaphores in the semaphore list is
 * in the signaled state, the calling thread will be blocked until one of the semaphores in the semaphore
 * list is signaled.
 *
 * @param semList The SPIL semaphore list object to wait on.
 *
 * @return a pointer to a semaphore which has been signaled.  If an error occurs, NULL will be returned instead.
 */
extern aqSpilSem_t      *aqSpilSemListWait(aqSpilSemList_t *semList);

/** @public
 * @memberof aqSpilSem_t
 * @brief Wait on a semaphore with a time limit
 *
 *  This function is similar to aqSpilSemWait().  However, this call will not block indefinitely if
 *  the semaphore does not get signaled.  This call will block up to the interval specified by the
 *  interval parameter.
 *
 *  @todo This function is currently broken and purposely asserts false.  This needs to be updated
 *  from the QNX timeout handling for general timeout handling.  The interval should be updated
 *  to be microseconds rather than ticks so that the timing is not implementation specific.
 *
 *
 * @param sem The semaphore to wait on.
 * @param interval The maximum period to wait before returning on timeout.
 *
 * @return <pre>aqSpilSemStatusOK</pre> indicates that the semaphore has been signaled.
 *         <pre>aqSpilSemStatusTimeout</pre> indicates that the specified timeout interval
 *         elapsed.  <pre>aqSpilSemStatusError</pre> indicates an error occurred (maybe an
 *         invalid semaphore handle was passed in...or there was an internal error).
 */
extern aqSpilSemStatus_t aqSpilSemTimedWait(aqSpilSem_t *sem, uint32_t interval);

/** @public
 * @memberof aqSpilSem_t
 * @brief Get current value of a semaphore
 *
 * @todo Fix for OSX.  OSX has deprecated posix unnamed semaphores.  To work around this
 * the dispatch semaphore was utilized.  Unfortunately, it doesn't seem to support the
 * equivalent of sem_getvalue().  Need to do more research to see if getting the value
 * is possible.  This method was needed by aqLink when doing a "software reset" function.   This
 * was doing pushing a semaphore to each thread which caused them to exit their main processing loop
 * and performing a timed wait on each of the semaphores allocated until the value became zero.  It looks
 * like such functionality would still be possible under Linux if needed, but would not be doable under OSX.
 * This may be acceptable if OSX is only used to aid development and not for production software.
 *
 * @param sem The semaphore to get the value of.
 * @param semValue A pointer to the value which is populated by this function.
 * @return <pre>aqSpilSemStatusOK</pre> Call was successful and the value of the specified semaphore
 *         [at the time it was queried] is stored @ semValue. <pre>aqSpilSemStatusError</pre> This call
 *         was not successful.  It could be that an invalid semaphore was passed in or that an internal
 *         error occurred.
 */
extern aqSpilSemStatus_t aqSpilSemGetValue(aqSpilSem_t *sem, int32_t *semValue);

/* ****************************************************************************
 *
 *  Mutex Services
 *
 * ****************************************************************************
 */


/** @public
 * @class aqSpilMtx_t
 *
 *
 *  @startuml {SpilSemaphoreListModelSeq.png}
 *
 * !define aqgreen 97ca3d
 * !define aqblue 1c9ad6
 * !define aqorange F05123
 * !define aqgrey 555759
 * !define aqdk_blue 091f40
 *
 *  title SPIL Mutex Semaphore Model
 *  hide footbox
 *
 *  participant "Application\nThread\nOne" as t1
 *  participant "Application\nThread\nTwo" as t2
 *  participant "SPIL\nAPI" as api
 *  participant "SPIL\nKernel\nThread\nOr OS\nKernel" as kt
 *
 *  note over kt
 *     Depending on the underlying OS, the
 *     mutex may be mapped directly to the
 *     Operating system's Mutex by the rather
 *     than being modeled in the SPIL kernel
 *  end note
 *
 *  activate t1
 *  t1 -> api : aqSpilMtxLock( mutexOne )
 *  activate api
 *  api->kt : sysMtxLock( mutexOne->sysMtx)
 *  activate kt
 *  kt --> api : return
 *  deactivate kt
 *  api --> t1 : return
 *  deactivate api
 *
 *
 *  t1 -> t1 : do some work on data\nprotected by mutexOne
 *  note over t1
 *     thread 1 is preempted
 *     and thread 2 becomes the
 *     running thread.
 *  end note
 *  deactivate t1
 *  kt --> t2 : make runnable
 *
 *  activate t2
 *  t2 -> api : aqSpilMtxLock( mutexOne )
 *  activate api #aqgreen
 *  api -> kt : sysMtxLock( mutexOne->sysMtx )
 *  activate kt #aqgreen
 *
 *
 *  note over t2
 *     mutex one is already locked by t1
 *     thus t2 is now blocked
 *  end note
 *
 *  kt ---> t2 : blocked
 *  kt ---> t1 : make runnable
 *  'deactivate kt
 *  deactivate t2
 *  activate t1
 *  t1 -> t1 : continue work on data\nprotected by mutexOne
 *  t1 -> api : aqSpilMtxUnlock( mutexOne )
 *  activate api #aqblue
 *  api -> kt : sysMtxUnlock( mutexOne->sysMtx )
 *  activate kt #aqblue
 *  kt --> api : return
 *  deactivate kt
 *  api --> t1 : return
 *  deactivate api
 *  kt --> t1 : block
 *  deactivate t1
 *  kt --> api : return
 *  deactivate kt
 *  api --> t2 : return (and make runnable)
 *
 *  deactivate api
 *
 *  activate t2
 *  t2 -> t2 : do some work on data\nprotected by mutexOne
 *  t2 -> api : aqSpilMtxUnlock( mutexOne )
 *  activate api
 *  api -> kt : sysMtxLock( mutexOne->sysMtx )
 *  activate kt
 *  kt ---> api : return
 *  deactivate kt
 *  api --> t2 : return
 *  deactivate api
 *  deactivate t2
 *
 *  @enduml
 *
 * @brief SPIL mutex object.
 */
typedef struct aqSpilMtx        aqSpilMtx_t;

/** @public
 * @brief Data type for used to to report status of calls to aqSpilMutexLock() and aqSpilMutexUnlock().
 */
typedef enum aqSpilMtxStatus_t {
   aqSpilMtxStatusOK       = 0,           ///< explicitly zero.  The result of the lock/unlock operation was successful.
   aqSpilMtxStatusError                   ///< The result of the lock/unlock operation was **NOT** successful.
} aqSpilMtxStatus_t;

/** @public
 * @memberof aqSpilMtx_t
 * @brief Allocate an unlocked mutex
 *
 * Allocate a mutex from the array of mutex semaphores that were passed in by aqSpilBindOS.
 *
 * @return A pointer to a mutex semaphore.  NULL if no mutexes are available.
 */
extern aqSpilMtx_t      *aqSpilMtxAllocate(void);

/** @public
 * @memberof aqSpilMtx_t
 * @brief Lock a mutex
 *
 * @param mutex The mutex to lock.
 * @return <pre>aqSpilMtxStatusOK</pre> the mutex was successfully locked.  <pre>aqSpilMtxStatusError</pre>
 * an error occurred while trying to lock the mutex object (could be a bad mutex handle or an internal error).
 */
extern aqSpilMtxStatus_t aqSpilMtxLock(aqSpilMtx_t *mutex);


/** @public
 * @memberof aqSpilMtx_t
 * @brief Unlock a mutex
 *
 * @param mutex The mutex to unlock.
 * @return <pre>aqSpilMtxStatusOK</pre> the mutex was successfully unlocked.  <pre>aqSpilMtxStatusError</pre>
 * an error occurred while trying to unlock the mutex object (could be a bad mutex handle or an internal error).
 */
extern aqSpilMtxStatus_t aqSpilMtxUnlock(aqSpilMtx_t *mutex);

/* ****************************************************************************
 *
 *  Timestamp, Timer and Delay Services
 *
 * ****************************************************************************
 */

// Timer services
/** @public
 * @brief The minimum value of a timeout in micro-seconds that can be specified by via
 * aqSpilTmrStart().
 */
#define SPIL_MIN_TIMER_PERIOD_IN_USEC (200)         // 200 micro-seconds

/** @public
 * @brief The maximum value of a timeout in micro-seconds that can be specified by via
 * aqSpilTmrStart().
 */
#define SPIL_MAX_TIMER_PERIOD_IN_USEC (3600000000)  // 1 hour

/** @public
 * @class aqSpilTmr_t
 * @brief SPIL timer object.
 *
 * @startuml {spilTimerSequence.png}
 *
 *  hide footbox
 *  participant "Allocate\nResource\nCallback" as cb
 *  participant "Application\nThread\nOne" as t1
 *  participant "Application\nThread\nTwo" as t2
 *  'participant "Application\nThread\nThree" as t3
 *  participant "SPIL\nAPI" as api
 *  participant "SPIL\nTimer\nThread" as tt
 *  participant "SPIL\nKernel\nThread" as kt
 *
 *  activate kt
 *  ==From within context of allocate resource callback registered in aqSpilBindOS()==
 *  api -> cb : allocateResources()
 *  activate cb
 *  api <-/ cb  : tmrOneSem = aqSpilSemAllocate()
 *  api <-/ cb : tmrOne = aqSpilTmrAllocate( tmrOneSem )
 *  note right of cb
 *    ... allocate other stuff too!
 *  end note
 *  cb --> api : return
 *  deactivate cb
 *
 *  ==After app threads running==
 *  activate tt
 *  activate t1
 *  activate t2
 *  t2 -> api : aqSpilSemWait( tmrOneSem )
 *  deactivate t2
 *  api -> kt : aqSpilSemWait( tmrOneSem )
 *  kt -> kt : thread 2 blocked\non tmrOneSem
 *  api <-/ t1 : aqSpilTmrStart( tmrOne, 100000uS, true )
 *  t1 -> api : wait on something ...
 *  deactivate t1
 *  api -> kt : Thread 1 wait on something
 *  kt -> kt : thread 1\nblocked
 *  loop forever
 *     tt -> tt : search for expired timer
 *     alt if tmrOne expired
 *        tt -> kt : aqSpilSemPost( tmrOneSem )
 *        kt -> t2 : unblock
 *        tt -> tt : reload tmrOne\n(if periodic)
 *        activate t2
 *        t2 -> t2 : do some work
 *        t2 -> api : aqSpilSemWait( tmrOneSem )
 *        deactivate t2
 *        api -> kt : aqSpilSemWait( tmrOneSem )
 *        kt -> kt : thread 2 blocked\non tmrOneSem
 *     end
 *  end loop
 *
 * @enduml
 *
 */
typedef struct aqSpilTmr        aqSpilTmr_t;

/** @public
 * @brief Return status for calls to aqSpilTmrStart(), aqSpilTmrStop, and aqSpilTmrFree()
 *
 * @see aqSpilTmrStart(), aqSpilTmrStop, and aqSpilTmrFree()
 */
typedef enum aqSpilTmrStatus_t {
   aqSpilTmrStatusOK       = 0,     ///< explicitly zero.  Call was successful
   aqSpilTmrStatusInactive,         ///< Attempting to stop a timer that was inactive.
   aqSpilTmrStatusError             ///< An error occurred while attempting to perform the request.
} aqSpilTmrStatus_t;

/** @public
 * @memberof aqSpilTmr_t
 * @brief Allocate a timer resource
 *
 * This function utilizes the array of timer objects that was passed in via
 * aqSpilBindOS() and returns an available timer resources to the caller.
 *
 * @param sem The semaphore that is to be signaled when the the timer represented
 * by this object expires (the timer must first be started).
 *
 * @return A pointer to the timer resource.  If no timer resource is available NULL will
 * be returned instead.
 */
extern aqSpilTmr_t      *aqSpilTmrAllocate(aqSpilSem_t *sem);

/** @public
 * @memberof aqSpilTmr_t
 * @brief Start a timer.
 *
 * This starts a timer of a specified duration.  After the specified duration elapses
 * the timer thread will signal associated with the specified timer object.
 *
 * @todo * Add range checking for timer period.
 *       * Check to see if timer is already running and report an error?  Or is it desired to change timeout of a already running timer?
 *       * Add method to validate timer handle (currently only checked against NULL)
 *
 * @param timer The timer object that is to be started.
 * @param periodInUsecs The number of micro-seconds that will elapse prior to the
 *                      timer expiring.
 * @param isPeriodic If set to true, the timeout interval will be re-loaded and the
 *                   timeout will be signaled on a periodInUsecs interval (+/- a
 *                   few micro-seconds).
 *
 * @return <pre>aqSpilTmrStatusOK</pre>  Timer was started successfully.
 *         <pre>aqSpilTmrStatusError</pre> An error occurred while attempting to start the timer (
 *         either an invalid timer handle, timer doesn't have an associated semaphore,
 *         specified period is out of range, or some internal error occurred).
 */
extern aqSpilTmrStatus_t aqSpilTmrStart(aqSpilTmr_t *timer,
                                        uint32_t     periodInUsecs, // in ticks
                                        bool_t      isPeriodic);

/** @public
 * @memberof aqSpilTmr_t
 * @brief Stop a timer.
 *
 * Stop a timer that is already in the running state.  Once stopped, the associated semaphore
 * will not be signaled.  However, there is a possible race where the timer's semaphore will
 * be signaled prior to it being stopped, and thus is a corner case that will need to be
 * handled by the application.
 *
 * @todo Add method to validate timer handle (currently only checked against NULL)
 *
 * @param timer The timer that will be stopped.
 *
 * @return <pre>aqSpilTmrStatusOK</pre> The timer was stopped successfully.
 *         <pre>aqSpilTmrStatusInactive</pre> The specified timer isn't running.
 *         <pre>aqSpilTmrStatusError</pre> An error occurred while processing the request (
 *         could be an invalid timer handle or an internal error).
 */
extern aqSpilTmrStatus_t aqSpilTmrStop(aqSpilTmr_t *timer);

/** @public
 * @memberof aqSpilTmr_t
 * @brief Free a timer resource
 *
 * @todo Add method to validate timer handle (currently only checked against NULL)
 *
 * @param timer A pointer to a timer resource to be freed.
 * @return <pre>aqSpilTmrStatusOK</pre> The request was processed successfully.
 *         <pre>aqSpilTmrStatusError</pre> An error occurred while processing
 *         the request (could be an invalid timer handle, trying to free a timer that
 *         is not currently allocated, or an internal error).
 */
extern aqSpilTmrStatus_t aqSpilTmrFree(aqSpilTmr_t *timer);

/** @public
 * @class aqSpilClkRtc_t
 * @brief Data type used for Real Time Time services.. e.g. @c YYYY-MM-DD @c HH:MM:SS
 */

/* ****************************************************************************
 *
 *  Message Queue Services
 *
 * ****************************************************************************
 */

/** @public
 * @class aqSpilMq_t
 * @brief SPIL message queue object.
 *
 * @startuml {spilQueueSeq.png}
 *  title SPIL Message Queue Model
 *  hide footbox
 *  participant "Allocate\nResource\nCallback" as cb
 *  participant "Application\nThread\nOne" as t1
 *  participant "Application\nThread\nTwo" as t2
 *  'participant "Application\nThread\nThree" as t3
 *  participant "SPIL\nAPI" as api
 *  'participant "SPIL\nTimer\nThread" as tt
 *  participant "SPIL\nKernel\nThread" as kt
 *
 *  activate kt
 *  ==From within context of allocate resource callback registered in aqSpilBindOS()==
 *  api -> cb : allocateResources()
 *  activate cb
 *  api <-/ cb  : qOneSem = aqSpilSemAllocate()
 *  api <-/ cb : qOne = aqSpilMqAllocate( width, depth, store, qOneSem )
 *  note right of cb
 *    ... allocate other stuff too!
 *  end note
 *  cb --> api : return
 *  deactivate cb
 *
 *  ==After app threads running==
 *  activate t1
 *  activate t2
 *  t2 -> api : aqSpilSemWait( qOneSem )
 *  deactivate t2
 *  api -> kt : aqSpilSemWait( qOneSem )
 *  kt -> kt : thread 2 blocked\non qOneSem
 *  api <-/ t1 : aqSpilMqEnqueue( qOne, element )
 *  t1 -> api : wait on something ...
 *  deactivate t1
 *  api -> kt : Thread 1 wait on something
 *  kt -> kt : thread 1\nblocked
 *
 *  api -> kt: aqSpilMqEnqueue( qOne, element )
 *  kt -> kt : copy element into\nmessage queue\nassuming space\navailable
 *  alt if message queue transitioned from empty to non-empty
 *     kt -> kt : aqSpilSemPost( qOneSem )
 *     kt -> t2 : unblock
 *     activate t2
 *  end
 *  api <-/ t2 : aqSpilMqDequeue( qOne, element )
 *  t2 -> t2 : do some work
 *
 *  t2 -> api : aqSpilSemWait( qOneSem )
 *  deactivate t2
 *  api -> kt : aqSpilSemWait( qOneSem )
 *  kt -> kt : thread 2 blocked\non qOneSem
 *
 * @enduml
 *
 */
typedef struct aqSpilMq         aqSpilMq_t;

/** @public
 * @brief Status codes returned from SPIL message queue functions aqSpilMqEnqueue(),
 * aqSpilMqDequeue(), aqSpilMqGetEntries(), and aqSpilMqFlush().
 *
 * @see aqSpilMqEnqueue(), aqSpilMqDequeue(), aqSpilMqGetEntries(), and aqSpilMqFlush().
 */
typedef enum {
   aqSpilMqStatusOK        = 0,           ///< explicitly zero.  Request successful.
   aqSpilMqStatusError                    ///< An error occurred processing the request.
} aqSpilMqStatus_t;

/** @public
 * @memberof aqSpilMq_t
 * @brief Allocate a message queue from the resources passed in by the application
 * by aqSpilBindOS()
 *
 * If no message queues are available a NULL pointer will be returned instead.
 *
 * @param width The number of bytes each element of the message queue contains.
 * @param depth The number of elements that can be stored in the message queue before
 *              the message queue becomes full.
 * @param storage The buffer that is used to store the message queue elements (The
 *                application should not access this storage directly.  Instead it
 *                should use the API calls provided to manage the message queue.
 * @param sem The semaphore to be associated with the message queue.  <pre>NULL</pre>
 *            is an acceptable semaphore.  However, no semaphore will be signaled
 *            in conjunction with this queue and it will be up to the application
 *            to pull the queue to see if there are any elements available.
 * @return A pointer to a message queue.  If there are no more message queue's
 *         available, <pre>NULL</pre> will be returned instead.
 */
extern aqSpilMq_t       *aqSpilMqAllocate(uint32_t width,
                                        uint32_t depth,
                                        uint8_t *storage,
                                        aqSpilSem_t *sem);

/** @public
 * @memberof aqSpilMq_t
 * @brief Enqueue an element to a SPIL message queue object.
 *
 * If the message is successfully queued, and the message queue has a semaphore, and
 * enqueuing of the message caused the message queue to go from an empty to a non-empty
 * state, the associated semaphore will be signalled.
 *
 * @todo Add a unique status code for a queue that is full.
 *
 * @param msgQ The SPIL message queue object to enqueue an entry to.
 * @param elementAddr The element that will be enqueued into the SPIL message queue.
 *        The element is transferred by copying queue.width bytes from \@elementAdder
 *        into the message queue's storage.
 * @return <pre>aqSpilMqStatusOK</pre> on success. <pre>aqSpilMqStatusError</pre> indicates
 *         an error occurred while processing the request (could be an invalid
 *         message queue pointer, an invalid element pointer, an invalid queue
 *         width, a full message queue, or an internal error).
 */
extern aqSpilMqStatus_t  aqSpilMqEnqueue(aqSpilMq_t *msgQ, cvar_t elementAddr);


/** @public
 * @memberof aqSpilMq_t
 * @brief Dequeue an element to a SPIL message queue object.
 *
 * @todo add a unique status code to indicate an empty queue.
 *
 * @param msgQ The SPIL message queue object to dequeue an entry from.
 * @param elementAddr The element that will be used to transfer the dequeued data to.
 *        The element is transferred by copying queue.width bytes from the queue
 *        storage into \@elementAddr.
 * @return <pre>aqSpilMqStatusOK</pre> on success. <pre>aqSpilMqStatusError</pre> indicates
 *         an error occurred while processing the request (could be an invalid
 *         message queue pointer, an invalid element pointer, an invalid queue
 *         width, an empty message queue, or an internal error).
 */
extern aqSpilMqStatus_t  aqSpilMqDequeue(aqSpilMq_t *msgQ, void* elementAddr);

/** @public
 * @memberof aqSpilMq_t
 * @brief Get the number of entries enqueued in a message queue
 *
 * This is the indication of how many entries are currently stored in the
 * message queue, not the number that have been stored over time.  e.g. it
 * will be the total number of entries added over time minus the total number
 * of entries dequeued.
 *
 * @param msgQ The SPIL message queue object.
 * @param numOfEntries A pointer to an integer where the number of entries should be
 *        stored.
 * @return <pre>aqSpilMqStatusOK</pre> on success -- The number of entries will be
 *         stored \@numberOfEntries. <pre>aqSpilMqStatusError</pre> indicates
 *         an error occurred while processing the request (could be an invalid
 *         message queue pointer, an invalid numberOfEntries pointer, an invalid queue
 *         width, or an internal error).
 */
extern aqSpilMqStatus_t  aqSpilMqGetEntries(aqSpilMq_t *msgQ,
                                          uint32_t *numOfEntries);

/** @public
 * @memberof aqSpilMq_t
 * @brief aqSpilMqFlush
 *
 * @param msgQ The SPIL message queue object.
 * @return <pre>aqSpilMqStatusOK</pre> on success -- zero elements remain
 *         in the message queue (head/tail and entry count are re-initialized).
 *         <pre>aqSpilMqStatusError</pre> indicates
 *         an error occurred while processing the request (could be an invalid
 *         message queue pointer, an invalid queue width, or an internal error).
 */
extern aqSpilMqStatus_t  aqSpilMqFlush(aqSpilMq_t *msgQ);

/* ****************************************************************************
 *
 *  Atomic Operation Services
 *
 * ****************************************************************************
 */

typedef int32_t (*aqSpilAopAtomicFunc)(cvar_t cbdata);

extern int32_t aqSpilAopAtomic(aqSpilAopAtomicFunc func, cvar_t cbdata);

/* ****************************************************************************
 *
 *  Diagnostic Services
 *
 * ****************************************************************************
 */

extern int32_t aqSpilDiaGetClientPriority(void);

/* ****************************************************************************
 *
 *  Resource Timing
 *
 * ****************************************************************************
 */

#ifdef AQ_ONGS_STATISTICS
   #define aqSpilRsUpdateAndStartTiming(threadMask) \
      aqSpilRsStartExecTiming((threadMask), aqFALSE)
   #define aqSpilRsDiscardAndStartTiming(threadMask) \
      aqSpilRsStartExecTiming((threadMask), aqTRUE)
   #define aqSpilRsStopTiming(threadMask) \
      aqSpilRsStopExecTiming(threadMask)
#else
   #define aqSpilRsUpdateAndStartTiming(threadMask)  ((void) 0)
   #define aqSpilRsDiscardAndStartTiming(threadMask) ((void) 0)
   #define aqSpilRsStopTiming(threadMask)            ((void) 0)
#endif // ifdef AQ_ONGS_STATISTICS

enum {
   aqSpilRsDataSamples    = 30 * 100  // 30 seconds of data sampled at 100 Hz
};

#ifdef AQ_ONGS_STATISTICS
extern int32_t                       aqSpilRsStatsIndex;
extern uint32_t                       aqSpilRsCyclesUsed;
extern uint32_t                       aqSpilRsCyclesUsedStats[];
extern aqSpilClkTimeStamp_t           aqSpilRsTimeStampStats[];

extern void             aqSpilRsStartExecTiming(aqSpilRsThrID_t mask, bool_t discard);
extern void             aqSpilRsStopExecTiming(aqSpilRsThrID_t mask);
#endif // ifdef AQ_ONGS_STATISTICS

// TODO: Find a home for me.... this need to be exposed in aqSpil.h to support worker thread API.
typedef struct spilResources aqSpilResources_t;

/** Return the hardware address of the specified interface.
 *
 *  @param if_name The name of the interface which the desired hardware address
 *                 is being requested (e.g. "eth0", "wlan0" ...).
 *  @param addr A buffer which will be populated with the hardware address
 *  @param addr_sz the number of bytes which have been allocated for addr.
 *
 *  @retval aqInvalidArgument One of the input parameters is not valid.
 *  @retval aqError An error occurred while attempting to fetch the hw address
 *  @retval aqOK The hardware address was successfully obtained and the
 *          contents of addr is a NULL terminated string representing the HW address.
 */
err_t aqSpilGetIfAddr( const char* if_name, char* addr, uint8_t addr_sz );


#define SPIL_DEVICE_ADDR_BUF_SZ (64U)

/** Return a unique device address.  Ideally, this will query a device
 *  serial number.  However, this is platform dependent, so in absence
 *  of an actual device serial number, this function may use network
 *  interface devices or other identifiers (e.g. IMEI) in order to
 *  provide a device specific unique ID.  In an extreme worst case,
 *  the platform could potentially generate and persist a UUID if
 *  needed (UUID creation/persistence is not currently implemented).
 *
 *  @param addr A buffer which will be populated with the hardware address
 *  @param addr_sz the number of bytes which have been allocated for addr.
 *
 *  @retval aqInvalidArgument One of the input parameters is not valid.
 *  @retval aqError An error occurred while attempting to fetch the hw address
 *  @retval aqOK The hardware address was successfully obtained and the
 *          contents of addr is a NULL terminated string representing the HW address.
 */
const char* aqSpilGetDeviceAddr( void );

#endif // AQSPIL_H
