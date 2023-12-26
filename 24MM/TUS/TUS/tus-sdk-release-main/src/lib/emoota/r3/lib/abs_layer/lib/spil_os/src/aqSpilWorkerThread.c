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
// Created by DQuimby on 3/19/17.
//

#include <string.h>
#include <spil_os/aqSpilWorkerThread.h>
#include <spil_os/aqSpilImpl.h>
#include <ontrac/ontrac/status_codes.h>
#include <ontrac/ontrac/mem_usage.h>
#include <ontrac/text/utilities.h>

/** @private
 * A copy of the worker thread objects which were passed to the SPIL during system
 * initialization.  This this is the list of all worker threads objects that have
 * are to be initialized and managed by this module.
 */
static struct workerThread *theWorkers = NULL;

/** @private
 * The number of worker thread objects that are in theWorkers.
 */
static uint32_t numWorkerThreads = 0U;

/** @private
 *  @brief Data type used to pass parameters of allocate thread method into atomic kernel service.
 */
typedef struct {
    const char *threadName;    ///< The name that the thread's TCB name should be set to (truncated to fit allocated sz
    cvar_t workerData;  ///< This is pointer to an object that the worker maintains.
    bool_t workerUsesSpilIo;   ///< Indication of whether or not the worker thread uses SPIL IO calls.
    workerThreadCb_t onStart;  ///< Callback registered to be called after worker thread has started.
    workerThreadCb_t doWork;   ///< Callback registered to be called continiously while worker thread is in WT_RUN.
    workerThreadCb_t onStop;   ///< Callback registered to be called after worker thread has been commanded to stop.
} wtAllocateWorker_t;

/** @private
 *  @brief Operation codes that are used to manage the worker thread's event flags via the method
 *  atomicWorkerThreadUpdateEvents().
 */
typedef enum {
    WT_SET_EVENTS,     ///< Set bits of the worker thread event flag using the given event flags.
    WT_GET_EVENTS,     ///< Get the current value of the worker thread event flags.
    WT_CLEAR_EVENTS    ///< Clear bits of the worker thread event flag using the given event flags.
} wtModifyEventsOp_t;

/** @private
 *  @brief Object used to pass parameters from the applicaiton API functions workerThreadSetEvents(),
 *         workerThreadClearEvents(), and workerThreadGetEvents() to the function
 *         atomicWorkerThreadUpdateEvents( void *arg ) which is run via the SPIL OS
 *         aqSpilAopAtomic() method.
 */
typedef struct {
    struct workerThread *worker;      ///< The worker thread who's event flags will be referenced/modified.
    wtModifyEventsOp_t op;       ///< The operation to be performed (set, clear, get) on the worker thread event flags.
    workerThreadEvent_t events;  ///< The event flags operand used to modify or reference the worker thread event flags.
} wtModifyEvents_t;

typedef enum {
    WT_MQ_OP_BIND = 1,
    WT_MQ_OP_RELEASE
} wtBindQueueOp_t;

typedef struct {
    wtBindQueueOp_t op;
    struct workerThread *worker;
    aqSpilMq_t *queue;
} wtBindQueue_t;

#ifdef DEBUG_WT_STATES
/** @private
 *  @brief Human readable symbolic name of the worker thread states.  This is used for logging as needed.
 */
static const char* WT_DESC[] ={
    "WT_INIT",
    "WT_IDLE",       ///< Worker thread is in the idle state.
    "WT_ALLOCATED",  ///< Worker thread has been allocated, but hasn't not been started yet.
    "WT_START",      ///< Worker thread is starting
    "WT_RUN",        ///< Worker thread is currently running
    "WT_STOP"        ///< Worker thread has been commanded to stop, however call to onStop() has not completed yet.
};
#endif

static struct workerThread *workerThread_resolve(workerThread_t *worker) {
    struct workerThread *retval = NULL;
    if((NULL != worker) && (NULL != theWorkers) && (0U != numWorkerThreads)
            && ((workerThread_t*)&theWorkers[0] <= worker)
            && (worker <= (workerThread_t*)&theWorkers[numWorkerThreads])) {
        ptrdiff_t offset = (ptrdiff_t) (worker - &theWorkers[0]);
        ABQ_VITAL((0 <= offset) && (offset <= (ptrdiff_t)numWorkerThreads));
        retval = &theWorkers[offset];
    }
    return retval;
}

/** @private
 *  @brief internal implementation of method used to update the worker thread's event flags.
 *
 *  This method is is intended to run via the SPIL kernel's aqSpilAopAtomic() method which
 *  will run in context of the SPIL kernel thread.  This insures that the operations within
 *  this method operate within a single thread context.
 *
 * @param arg an instance of wtModifyEvents_t which has been cast to a (void *).  This object
 *            holds the op code and data which will be applied to/from the worker thread's
 *            event member.
 * @retval EXIT_SUCCESS The modification to the worker thread's event member was successful.
 * @retval EUNSPECIFIED No modification took place due to error.  The error is either an
 *         incorect opcode provided in the input parameter, or the input parameter was NULL.
 */
static int32_t atomicWorkerThreadUpdateEvents(cvar_t cbdata)
{
    int32_t retval = (int32_t) EXIT_SUCCESS;
    wtModifyEvents_t *e;
    (void) bytes_copy(&e, &cbdata, sizeof(cvar_t));
    if( (NULL == e) || (NULL == e->worker) ) {
        retval = (int32_t) EUNSPECIFIED;
    } else {
        if( WT_SET_EVENTS == e->op ) {
            e->worker->events |= e->events;
            e->worker->events &= WT_EVENT_MASK;
        } else if( WT_CLEAR_EVENTS == e->op ) {
            e->worker->events &= e->events;
            e->worker->events &= WT_EVENT_MASK;
        } else if( WT_GET_EVENTS == e->op ) {
            e->worker->events &= WT_EVENT_MASK;
            e->events = e->worker->events;
        } else {
            retval = (int32_t) EUNSPECIFIED;
        }
    }
    return retval;
}

// see aqSpilWorkerThread.h
void workerThreadSetEvents( workerThread_t *worker, workerThreadEvent_t setEvents )
{
    wtModifyEvents_t e;
    struct workerThread *wt = workerThread_resolve(worker);

    e.op = WT_SET_EVENTS;
    e.worker = wt;
    e.events = setEvents;
    if( (int32_t) EUNSPECIFIED == aqSpilAopAtomic(atomicWorkerThreadUpdateEvents, &e)) {
        SLOG_E("Error setting worker thread events.");
    } else {
        if( NULL != worker->sem ) {
            (void) aqSpilSemPost(worker->sem);
        }
    }
}

// see aqSpilWorkerThread.h
void workerThreadClearEvents( workerThread_t *worker, workerThreadEvent_t clearEvents )
{
    wtModifyEvents_t e;

    struct workerThread *wt = workerThread_resolve(worker);

    e.op = WT_CLEAR_EVENTS;
    e.worker = wt;
    e.events = clearEvents;
    if( (int32_t) EUNSPECIFIED == aqSpilAopAtomic(atomicWorkerThreadUpdateEvents, &e)) {
        SLOG_E("Error clearing worker thread events.");
    }
}

// see aqSpilWorkerThread.h
/* parasoft-begin-suppress MISRAC2012-RULE_8_7-a-4 "m0053. Not yet used externally" */
workerThreadEvent_t workerThreadGetEvents( workerThread_t *worker )
/* parasoft-end-suppress MISRAC2012-RULE_8_7-a-4 */
{
    wtModifyEvents_t e;
    workerThreadEvent_t retval = WT_EVENT_NONE;

    // MISRA, need to initialize 'e' before assigning it to another variable
    (void) memset(&e, 0, sizeof(e));
    e.op = WT_GET_EVENTS;
    e.worker = workerThread_resolve(worker);

    if( (int32_t) EUNSPECIFIED == aqSpilAopAtomic(atomicWorkerThreadUpdateEvents, &e)) {
        SLOG_E("Error getting worker thread events.");
    } else {
        retval = e.events;
    }
    return retval;
}

static void shutdown( int32_t workerNum )
{
    byte_t buffer[128] = "";
    ABQ_ENCODER(encoder, &ascii_codec, buffer, sizeof(buffer));
    (void) abq_encode_ascii(&encoder, "workerNum ", -1);
    (void) abq_encode_int(&encoder, (int64_t) workerNum, DECIMAL_RADIX);
    (void) abq_encode_ascii(&encoder, " got WT_EVENT_SHUTDOWN...exiting", -1);
    SLOG_I(buffer);
}

// see aqSpilWorkerThread.h
void workerThreadEntry( cvar_t arg )
{
    workerThreadEvent_t ev = WT_EVENT_NONE;
    bool_t isShutdown = false;
    char buf[256] = "";
    ABQ_ENCODER(encoder, &ascii_codec, buf, sizeof(buf));

    if( NULL != arg ) {
        aqTcb_t *thread = thisThread();
        ABQ_VITAL(arg == (cvar_t)thread);
        // find the worker thread which has this task control block
        struct workerThread *wt = NULL;
        uint32_t workerNum = 0;
        for(workerNum = 0; workerNum < numWorkerThreads; workerNum++) {
            if( theWorkers[workerNum].tcb == thread ) {
                wt = &theWorkers[workerNum];

                encoder.pos = 0U; // Reset encoder
                (void) abq_encode_char(&encoder, '[');
                (void) abq_encode_int(&encoder, (int64_t) workerNum, DECIMAL_RADIX );
                (void) abq_encode_ascii(&encoder,  "] == thread: ", -1);
                (void) abq_encode_ptraddr(&encoder, thread );
                (void) abq_encode_ascii(&encoder,  ", thread->stack: ", -1);
                (void) abq_encode_ptraddr(&encoder, thread->stackAddr );
                (void) abq_encode_ascii(&encoder,  ", wt->sem: ", -1);
                (void) abq_encode_ptraddr(&encoder, wt->sem );
                ABQ_INFO_MSG_Y("theWorkers", buf, encoder.pos);
                break;
            }
        }

        VITAL_NOT_NULL(wt);

        while (false == isShutdown) {
            do {
                (void) aqSpilSemWait(wt->sem);
                ev = workerThreadGetEvents(wt);
                if( 0U != (ev & WT_EVENT_SHUTDOWN) ) {
                    isShutdown=true;
                    shutdown((int32_t) workerNum );
                }
            } while ((WT_ALLOCATED != wt->state) && (WT_START != wt->state) );

            do {
                ABQ_INFO_MSG_X("waiting for WT_START on workerNum:", workerNum);
                (void) aqSpilSemWait(wt->sem);
                ev = workerThreadGetEvents(wt);
                if( 0U != (ev & WT_EVENT_SHUTDOWN) ) {
                    isShutdown=true;
                    shutdown((int32_t) workerNum);
                }
            } while (WT_START != wt->state);

            if (NULL != wt->onStart) {
                wt->onStart(wt, wt->workerData);
            }

            wt->state = WT_RUN;

            while (true == isWorkerThreadRunning(wt)) {
                if (NULL != wt->doWork) {
                    wt->doWork(wt, wt->workerData);
                }


                if(false == wt->workerUsesSpilIo) {
                    // Worker thread doesn't use SPIL model (e.g. block on semaphore), so sleep a little
                    (void) aqSpilThrSleep(10000);
                }

                ev = workerThreadGetEvents(wt);
                if( 0U != (ev & WT_EVENT_SHUTDOWN) ) {
                    isShutdown=true;
                    shutdown((int32_t) workerNum);
                }
            }

            if (NULL != wt->onStop) {
                wt->onStop(wt, wt->workerData);
            }

            // clean up.
            wt->onStart = NULL;
            wt->onStop = NULL;
            wt->doWork = NULL;
            wt->workerData = NULL;
            wt->workerUsesSpilIo = false;
            if( NULL != wt->queue ) {
                (void) workerThreadReleaseQueue(wt);
            }

            // Once in idle, the worker thread can be re-allocated
            wt->state = WT_IDLE;
        }
    }
}

// see aqSpilWorkerThread.h
/* parasoft-begin-suppress MISRAC2012-RULE_8_7-a-4 "m0054. Not yet used externally" */
bool_t isWorkerThreadRunning( const workerThread_t *worker ) {
/* parasoft-end-suppress MISRAC2012-RULE_8_7-a-4 */
    bool_t retval = false;

    if( NULL != worker ) {
        retval = (worker->state == WT_RUN) ? true : false;
    }
    return retval;
}

// see aqSpilWorkerThread.h
err_t startWorkerThread( workerThread_t *worker ) {
    err_t status = EINVAL;
    struct workerThread *wt = workerThread_resolve(worker);
    if( (NULL != wt) && (WT_ALLOCATED == worker->state) ) {
        wt->state = WT_START;
        status = EXIT_SUCCESS;
        if( NULL != worker->sem ) {
            (void) aqSpilSemPost(worker->sem);
        }
    }
    return status;
}

// see aqSpilWorkerThread.h
err_t stopWorkerThread( workerThread_t *worker ) {
    err_t status = EINVAL;
    struct workerThread *wt = workerThread_resolve(worker);
    if( (NULL != wt) && (WT_RUN == worker->state) ) {
        wt->state = WT_STOP;
        status = EXIT_SUCCESS;
        if( NULL != worker->sem ) {
            (void) aqSpilSemPost(worker->sem);
        }
    }
    return status;
}


/** @private
 *  @brief Agent which performs the duties of allocateWorkerThread() from within the context of
 *  the SPIL kernel thread.
 *
 * ASSUMES: The spil has been intialized before this is called.
 *
 * @param arg An instance of wtAllocateWorker_t that has been cast to a NULL pointer.  This contains the
 *            same parameters which were passed from the application to the allocateWorkerThread() method.
 * @retval > 0 indicates a successful return and is also the index of the worker thread that has been allocated
 *             and should be returned to the application.
 * @retval < 0 indicates that a worker thread could not be allocated at this time.
 */
static int32_t atomicAllocWorkerThread(cvar_t cbdata)
{
    uint32_t i = 0;
    if( NULL != theWorkers ) {
        wtAllocateWorker_t* aw;
         (void) bytes_copy(&aw, &cbdata, sizeof(cvar_t));
        for(i = 0; i < numWorkerThreads; i++) {
            if( WT_IDLE == theWorkers[i].state ) {
                theWorkers[i].state = WT_ALLOCATED;
                theWorkers[i].onStart = aw->onStart;
                theWorkers[i].doWork = aw->doWork;
                theWorkers[i].onStop = aw->onStop;
                theWorkers[i].workerUsesSpilIo = aw->workerUsesSpilIo;
                aqSpilThrSetName(theWorkers[i].tcb, aw->threadName );
                theWorkers[i].workerData = aw->workerData;
                theWorkers[i].queue = NULL;
                break;
            }
        }
    }
    return (int32_t) i;
}

// see aqSpilWorkerThread.h
workerThread_t * allocWorkerThread( const char *threadName,
                                           cvar_t workerData,
                                           bool_t isWorkerUseSpilIo,
                                           workerThreadCb_t onStart,
                                           workerThreadCb_t doWork,
                                           workerThreadCb_t onStop) {
    workerThread_t* retval = NULL;
    int32_t wtIndex = 0;

    // prepare params to be passed to atomic
    wtAllocateWorker_t aw;
    aw.threadName = threadName;
    aw.workerData = workerData;
    aw.onStart = onStart;
    aw.doWork = doWork;
    aw.onStop = onStop;
    aw.workerUsesSpilIo = isWorkerUseSpilIo;

    // Do the allocation within the context of the "atomic" operator to insure that only
    // one worker can be allocated at a time to prevent the WT table from getting
    // fubar.
    wtIndex = aqSpilAopAtomic(atomicAllocWorkerThread, &aw);

    // Convert the worker thread index returned by atomic operation to pointer to workerThread_t
    if((wtIndex >= 0) && (wtIndex < (int32_t) numWorkerThreads) ) {
        retval = &theWorkers[wtIndex];
        (void) aqSpilSemPost(retval->sem);
    }



    return retval;
}

// see aqSpilWorkerThread.h
void initWorkerThreads(const aqSpilResources_t *theResources) {
    if( (NULL != theResources) && (NULL != theResources->workerThreads) ) {
        numWorkerThreads = theResources->numWorkerThreads;
        theWorkers = theResources->workerThreads;
        uint16_t i = 0;


        for(i=0;i<numWorkerThreads;i++) {
            struct workerThread* wt = &theWorkers[i];
            wt->state = WT_INIT;
            wt->workerData = NULL;
            wt->doWork = NULL;
            wt->onStart = NULL;
            wt->onStop = NULL;
            wt->events = WT_EVENT_NONE;
            wt->workerUsesSpilIo = false;
            wt->queue = NULL;
            wt->workerThreadNum = (uint32_t) i;
            wt->sem = aqSpilSemAllocate();
            VITAL_NOT_NULL(wt->sem);

            // FIXME: This is gross ... maybe it should be moved into bind OS rather than part of worker thread...
            // worker thread needs its TCB in order for any signals to be sent to the proper thread.
            wt->tcb = &theResources->tcbs[IMPL_RESERVED_THREAD_NUM + theResources->numAppThreads + i];

            // Create the thread name based upon worker thread number
            ABQ_ENCODER(encoder, &ascii_codec,
                    wt->tcb->name, sizeof(wt->tcb->name));
            VITAL_IS_OK(abq_encode_ascii(&encoder, "Work: ", -1));
            VITAL_IS_OK(abq_encode_int(&encoder, (int64_t) i, DECIMAL_RADIX));
            ABQ_VITAL(encoder.pos < encoder.max);
            wt->state = WT_IDLE;
        }
    }
}


static int32_t atomicWorkerThreadQueueBinding( cvar_t cbdata)
{
    int32_t retval = -1;
    bool_t isAlreadyBound = false;
    bool_t isWorkerValid = false;
    uint32_t i = 0;
    VITAL_NOT_NULL(cbdata);
    wtBindQueue_t *bq;
     (void) bytes_copy(&bq, &cbdata, sizeof(cvar_t));
    if( (NULL != bq->worker) && (NULL != bq->queue) ) {
        switch( bq->op ) {
            case WT_MQ_OP_BIND:

                // insure the queue isn't already bound to another worker thread
                for(i = 0; i < numWorkerThreads; i++) {
                    if( bq->queue == theWorkers[i].queue ) {
                        isAlreadyBound = true;
                        break;
                    }

                    // the worker thread object is valid too?
                    if(bq->worker == &theWorkers[i] ) {
                        isWorkerValid = true;
                    }
                }
                // good... bind it.
                if( (false == isAlreadyBound) && (true == isWorkerValid) ) {
                    if( aqSpilMqStatusOK == aqSpilMqFlush(bq->queue) ) {
                        bq->worker->queue = bq->queue;
                        retval = 0;
                    }
                }
                break;

            case WT_MQ_OP_RELEASE:
                for(i = 0; i < numWorkerThreads; i++) {

                    // insure the message queue handle is valid
                    if(bq->worker == &theWorkers[i] ) {

                        // The worker is valid.  break the association
                        if( aqSpilMqStatusOK == aqSpilMqFlush(bq->worker->queue) ) {
                            retval = 0;
                            break;
                        }
                        bq->worker->queue = NULL;
                    }
                }
                break;

            default:
                /* Nothing to do */
                break;
        }
    }
    return retval;
}



// see aqSpilWorkerThread.h
err_t workerThreadBindQueue( workerThread_t *worker, aqSpilMq_t *queue )
{
    err_t retval = EINVAL;
    struct workerThread *wt = workerThread_resolve(worker);
    if( (NULL != wt) && (NULL != queue) ) {
        wtBindQueue_t bq;
        bq.op = WT_MQ_OP_BIND;
        bq.worker = wt;
        bq.queue = queue;
        retval = (err_t) aqSpilAopAtomic(atomicWorkerThreadQueueBinding, &bq);
    }
    return retval;
}

// see aqSpilWorkerThread.h
/* parasoft-begin-suppress MISRAC2012-RULE_8_7-a-4 "m0055. Not yet used externally" */
err_t workerThreadReleaseQueue( workerThread_t *worker )
/* parasoft-end-suppress MISRAC2012-RULE_8_7-a-4 */
{
    err_t retval = EINVAL;
    struct workerThread *wt = workerThread_resolve(worker);
    if( NULL != wt ) {
        wtBindQueue_t bq;
        bq.op = WT_MQ_OP_RELEASE;
        bq.worker = wt;
        bq.queue = worker->queue;
        retval = (err_t) aqSpilAopAtomic(atomicWorkerThreadQueueBinding, &bq);
    }
    return retval;
}



// see aqSpilWorkerThread.h
err_t workerThreadEnqueue( workerThread_t *worker, const void *element )
{
    err_t retval = EUNSPECIFIED;
    struct workerThread *wt = workerThread_resolve(worker);
    if( (NULL != wt) && (NULL != wt->queue) && (NULL != wt->sem) ) {
        if( aqSpilMqStatusOK == aqSpilMqEnqueue( wt->queue, element)) {
            if( aqSpilSemStatusOK == aqSpilSemPost(wt->sem) ) {
                retval = EXIT_SUCCESS;
            }
        }
    }
    return retval;
}


// see aqSpilWorkerThread.h
err_t workerThreadDequeue( workerThread_t* worker, void *element )
{
    err_t retval = EUNSPECIFIED;
    aqSpilSemStatus_t semStatus;
    bool_t done = false;

    struct workerThread *wt = workerThread_resolve(worker);
    if( (NULL != wt) && (NULL != wt->queue) && (NULL != wt->sem) ) {
        uint32_t numEntries = 0;

        do {
            if( aqSpilMqStatusOK == aqSpilMqGetEntries( wt->queue, &numEntries ) ) {
                if( numEntries > 0U ) {
                    if( aqSpilMqStatusOK == aqSpilMqDequeue( wt->queue, element ) ) {
                        retval = EXIT_SUCCESS;
                        done = true;
                    }
                } else {

                    // see if there is anything interesting pending ... like stop or shutdown
                    workerThreadEvent_t ev = workerThreadGetEvents(wt);
                    if (0U != (ev & WT_EVENT_SHUTDOWN)) {
                        // TODO: need this to reach out to the worker thread run loop.
                        retval = EUNSPECIFIED;
                        done = true;
                    } else {

                        // go to sleep
                        semStatus = aqSpilSemWait(wt->sem);

                        // If semaphore status is not OK or worker thread is no longer in run state give up.
                        if ((semStatus != aqSpilSemStatusOK) || (WT_RUN != wt->state)) {
                            done = true;
                        }
                    }
                }
            }
        } while( false == done );
    }
    return retval;
}

