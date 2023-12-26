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
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>
#include <assert.h>
#include <spil_os/aqSpilImpl.h>
#include <spil_file/spil_file.h>
#include <ontrac/ontrac/status_codes.h>
#include <ontrac/util/thread_utils.h>
#ifdef __APPLE__
#include <spil_os/aqSpilWorkerThread.h>
#elif defined(_WIN32) && !defined(__SYMBIAN32__)
#include <io.h>
#include <fcntl.h>
#include <windows.h>
//include <synchapi.h>
#endif


// Intra-aqFramework definitions

// Private type definitions
typedef double float64_t;


#if 0
#define RT_THREADS
#endif

/**
 * brief Status codes returned by POSIX
 */
#define POSIX_OK ( 0 )
#define POSIX_ERROR ( -1 )


/** @file
 *  System Platform Isolation Layer (SPIL)
 */
#ifdef AQ_ONGS_STATISTICS
// Resource Timing

int32_t                       aqSpilRsStatsIndex;
uint32_t                       aqSpilRsCyclesUsed;
uint32_t                       aqSpilRsCyclesUsedStats[aqSpilRsDataSamples];
aqSpilClkTimeStamp_t           aqSpilRsTimeStampStats[aqSpilRsDataSamples];

static aqSpilClkTimeStamp_t    startTime;
static volatile uint32_t       activeThreads; // bit 0=cp, 1=md, 2=td, 3=aud
#endif // ifdef AQ_ONGS_STATISTICS

static aqSpilResources_t *theResources=NULL;

// Semaphores
static aqSpilClkContinuousTime_t spilStartTime={0};

static aqSpilSem_t             *semTable=NULL;
static aqSpilSemList_t         *semListTable=NULL;

static pthread_mutex_t        semTableMutex;

// Mutexes -- Used internally.  Not part of the SPIL model ... YET.  TBD --- needed?  Definitely nice to have.
static aqSpilMtx_t             *mtxTable=NULL;

// Threads

// FIXME_DQ: AQ_NUMBER_OF_THREADS must be the total passed in by bind os + 1 (Spil thread)
static aqTcb_t                *thrTable = NULL;
static aqTcb_t                clientThread;
static uint8_t                stack_aud[STACK_SIZE_AUD];
static uint8_t                *stack_ini = stack_aud; /* parasoft-suppress MISRAC2012-RULE_8_9-a-4 "m0047. By design. Usage explained by comment in aqSpilEntryPoint()" */
// Timers

static aqSpilTmr_t             *tmrPool=NULL;

static pthread_mutex_t        tmrPoolMutex;

// Message Queues

static aqSpilMq_t              *mqTable=NULL;

// The isoSsytemPipe is used internally between SPIL layer API and internal SPIL kernel
static pipeFdSet  isoSystemPipe;

/* Prototypes for internal (non-API) functions */
static aqSpilThrStatus_t   allocateThread( void );
static void* initSpil(void* pStatus);
static void*  posixThreadEntryPoint(void* arg);
static bool_t     isValidSemHandle(const aqSpilSem_t *sem);
static bool_t     isValidSemListHandle(const aqSpilSemList_t *list);
static bool_t     isValidMtxHandle(const aqSpilMtx_t *mtx);
static aqSpilSem_t        *removeSemFromQueue(aqSpilSem_t *s);
static void aqSpilEntryPoint(cvar_t junk);
static aqSpilThrStatus_t aqSpilThrAckReady(void);

/* ****************************************************************************
 *
 *  SPIL public functions
 *
 * ***************************************************************************
 * */

// comment maintained in aqSpilImple.h
err_t  aqSpilBindOS(aqSpilResources_t *resources)
{
    // FIXME - more robust error handling
    int32_t        i = 0;
    err_t retval = EXIT_SUCCESS;

    static bool_t  hasBeenCalled = false;
    byte_t sb_buf[256] = "";
    ABQ_ENCODER(encoder, &ascii_codec, sb_buf, sizeof(sb_buf)); /* parasoft-suppress MISRAC2012-RULE_2_2-b-2 "m0048. encoder is used by reference later in the function" */

    if( NULL == resources) {
        SLOG_E( "[aqSpilBindOS] invalid argument.");
        retval = EUNSPECIFIED;
    }

    if ( ( EXIT_SUCCESS == retval ) && ( true == hasBeenCalled ) ) {
        SLOG_E( "[aqSpilBindOS] illegal operation.");
        retval = EUNSPECIFIED;
    }

    if( ( EXIT_SUCCESS == retval ) && ( 0 != spilStartTime.tv_sec ) ){
        SLOG_E( "[aqSpilBindOS] already bound.");
        retval = EUNSPECIFIED;
    }

    int64_t timediff = 0;
    aqSpilClkContinuousTime_t int32UsecOverflow = {
            .tv_sec = 2148,
            .tv_nsec = 0
    };

    if( EXIT_SUCCESS == retval ) {
        if(aqSpilClkStatusOK != aqSpilDiffContinuousTime(&int32UsecOverflow,
                                                         &spilStartTime, &timediff)){
            SLOG_E( "[aqSpilBindOS] failed to diff time.");
            retval = EUNSPECIFIED;
        } else if(timediff <= 0){
            SLOG_E( "[aqSpilBindOS] diff time overflow error.");
            retval = EUNSPECIFIED;
        } else {
            /* do nothing. MISRA insisted that else follow else if. */
        }
    }

    if( EXIT_SUCCESS == retval ) {
        err_t ret = aqSpilClkGetContinuousTime(&spilStartTime);
        if( EXIT_SUCCESS != ret ) {
            SLOG_E( "[aqSpilBindOS] failed to get time.");
            retval = EUNSPECIFIED;
        }
    }

    hasBeenCalled = true;

    if( EXIT_SUCCESS == retval ) {
        theResources = resources;
        thrTable = resources->tcbs;
        semTable = resources->semas;
        mtxTable = resources->mutexes;
        semListTable = resources->semaLists;
        mqTable = resources->queues;
        tmrPool = resources->timerz;

        /* create the SPIL "kernel" thread  and initialize resources */
        if (aqSpilThrStatusOK != allocateThread( )) {
            SLOG_E( "[aqSpilBindOS] internal error.");
            retval = EUNSPECIFIED;
        }
    }

    if( EXIT_SUCCESS == retval ) {
        SLOG_I("waiting for ISO to sync.");
        aqTcb_t *t = &thrTable[0];
        VITAL_IS_OK(pthread_mutex_lock(&t->condMutex));
        while (NOT_BLOCKED != t->blockage) {
            VITAL_IS_OK(pthread_cond_wait(&t->cond, &t->condMutex));
        }
        VITAL_IS_OK(pthread_mutex_unlock(&t->condMutex));
    }

    if( EXIT_SUCCESS == retval ) {
        SLOG_I("ISO synced.");

#ifdef __APPLE__
        aqSpilThrSleep(1000000);
#endif

        if( NULL != resources->allocFunc) {
            if( EXIT_SUCCESS != resources->allocFunc(resources->allocFuncArg) ) {
                retval = EUNSPECIFIED;
            }
        }
    }

    if( EXIT_SUCCESS == retval ) {
        initWorkerThreads(resources);

        // allocate AUD + all SPIL application threads
        for (i = 1; (uint8_t) i < theResources->numThreads; i++) {
            if (aqSpilThrStatusOK != allocateThread( )) {
                retval = EUNSPECIFIED;
                break;
            }
        }
    }

    if( EXIT_SUCCESS == retval ) {
        // wait for AUD + all SPIL application and worker threads to spin up
        for (i = 1; (uint8_t) i < theResources->numThreads; i++) {
            encoder.pos = 0U; // Reset encoder
            (void) abq_encode_ascii(&encoder, "waiting for thread ", -1);
            (void) abq_encode_int(&encoder, (int64_t) i, DECIMAL_RADIX);
            (void) abq_encode_ascii(&encoder, " -- ", -1);
            (void) abq_encode_ascii(&encoder, theResources->tcbs[i].name, -1);
            (void) abq_encode_ascii(&encoder, " to sync.", -1);
            SLOG_I(sb_buf);

            aqTcb_t *t = &thrTable[i];
            VITAL_IS_OK(pthread_mutex_lock(&t->condMutex));
            while (BLOCKED_ON_INIT == t->blockage) {
                VITAL_IS_OK(pthread_cond_wait(&t->cond, &t->condMutex));
            }
            VITAL_IS_OK(pthread_mutex_unlock(&t->condMutex));

            encoder.pos = 0U; // Reset encoder
            (void) abq_encode_ascii(&encoder, "Thread ", -1);
            (void) abq_encode_int(&encoder, (int64_t) i, DECIMAL_RADIX);
            (void) abq_encode_ascii(&encoder, " synced.", -1);
            SLOG_I(sb_buf);
        }
    }

   return retval;
}

static void aqSpilTimerThreadEntryPoint(cvar_t myTcb) {

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    aqSpilClkContinuousTime_t nextWakeup;
    int64_t wakeUpUsecs=0;

    /* Supress MISRA2012-RULE-2_7
     * Parameter 'myTcb' is not used in function 'aqSpilTimerThreadEntryPoint'
     * The parameter must be declared in order to fit the thread entry point
     * function structure. Even if it isn't used.
     */
    if( NULL != myTcb ) {

    }

    while(1) {
        if( NULL == theResources ) {
			(void) aqSpilThrSleep(1000000);
        } else {
            aqSpilTmr_t *tmr = NULL;
            aqSpilClkContinuousTime_t ts_now;
            #ifdef __APPLE__
               nextWakeup.tv_sec = INT64_MAX;
            #elif defined ( __CYGWIN__ )
               nextWakeup.tv_sec = INT32_MAX;
            #else
               nextWakeup.tv_sec = INT32_MAX;
            #endif
            nextWakeup.tv_nsec = 0;

            // get current time
            (void) aqSpilClkGetContinuousTime(&ts_now);

            // find expired timers and signal semaphore
            int64_t diffTime = 0;
            for ( tmr = tmrPool; tmr < &tmrPool[theResources->numTimerz]; tmr++) {
                VITAL_IS_OK(pthread_mutex_lock(&tmrPoolMutex));
                if ((tmr->periodInUsecs > 0U) && (NULL != tmr->sem) ) {
                    // Timer is currently allocated and running

                    if( ( aqSpilClkStatusError != aqSpilDiffContinuousTime(&ts_now, &tmr->timeout, &diffTime ) )
                        && ( diffTime >= 0 ) ) {

                        // this timer has expired.  Signal the semaphore and reload if needed.
                        (void) aqSpilSemPost(tmr->sem);

                        if (false == tmr->isPeriodic) {
                            // timer is done ...
                            tmr->periodInUsecs = 0;
                        } else {
                            // reload the timer for the next timeout
                            do {

                                // Add timeout period until timeout > now
                                tmr->timeout.tv_nsec += ((int64_t) tmr->periodInUsecs * (int64_t) NS_PER_USEC);
                                // timeout is normalized within call to aqSpilDiffContinuousTime
                                if( aqSpilClkStatusOK != aqSpilDiffContinuousTime(&ts_now, &tmr->timeout, &diffTime ) ) {
                                    break;
                                }
                            } while ( diffTime > 0 );
                        }
                    }

                    // See if wakeup for this timer is sooner than current
                    // earliest computed wake up time
                    if( ( aqSpilClkStatusError != aqSpilDiffContinuousTime(&tmr->timeout, &nextWakeup, &diffTime) )
                        && ( diffTime < 0 ) ) {
                        nextWakeup.tv_sec = tmr->timeout.tv_sec;
                        nextWakeup.tv_nsec = tmr->timeout.tv_nsec;
                    }
                }
                VITAL_IS_OK(pthread_mutex_unlock(&tmrPoolMutex));
            }

            // figure up the next wakeup
            if( ( aqSpilClkStatusError == aqSpilDiffContinuousTime(&nextWakeup, &ts_now, &wakeUpUsecs) )
                || ( wakeUpUsecs > USEC_PER_SEC ) ) {
                // Wake up at-least once per second in order to deal any timers which were started
                // FIXME -- deadlines will be missed started timers which have < 1 second for first timeouts
#if defined(_WIN32)  && !defined(__SYMBIAN32__)
                Sleep(USEC_PER_SEC / 1000);
#else
                (void) usleep(USEC_PER_SEC);
#endif
            } else if ( wakeUpUsecs > 0 ) {
#if defined(_WIN32)  && !defined(__SYMBIAN32__)
                Sleep(wakeUpUsecs / 1000);
#else
                /* (uint32_t) cast is valid since if we got here, we already know wakeUpUsecs is > 0 */
                (void) usleep( (uint32_t) wakeUpUsecs);
#endif
            } else {
                // wakeup already past due ... back to the top of the loop
            }
        }
    }
#pragma clang diagnostic pop
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
static void aqSpilEntryPoint(cvar_t junk)
{

   schedEvent_t               event;
   ssize_t                    bytesRead = 0;
   size_t                     accumBytesRead = 0;
   uint8_t                    *bufPtr = NULL;
   aqTcb_t                    *t = NULL;
   bool_t                     threadWasUnblocked;
   pthread_t                  thread_id = (pthread_t){0};
   pthread_attr_t             pthread_attr;

   if( NULL != junk ) {
    /* Supress MISRA2012-RULE-2_7
     * Parameter 'junk' is not used in function 'aqSpilEntryPoint'
     * The parameter must be declared in order to fit the thread entry point
     * function structure. Even if it isn't used.
     */
   }

   /*  (QNX):
    *  The stack space required to create pipes is expensive so perform
    *  initialization in a temporary helper thread, "borrowing" its stack
    *  space from the other threads' stacks, and reclaiming the stack space
    *  when finished before the other threads are created
    */

   /* MISRA2012-DIR-4_6_b The basic numerical type 'int' should not be used.
    * but ... int is what pthread_atter_init returns. Need to justify this
    * exception. The other alternative is to have very messy compiler directives
    * which use different types on different systems based upon sizeof(int) */
   VITAL_IS_OK(pthread_attr_init(&pthread_attr));
#ifdef __APPLE__
#elif defined(_WIN32) && !defined(__SYMBIAN32__)
#else
   /* stack_ini is an alias for stack_aud. This works because stack_ini is only used for
    * early initialization of the spil_os. However, the initSpil routine can't utilize
    * the spil_iso stack because spilInit is called from the context of the spil "kernel"
    * thread which alsu utilizes the spil_iso stack. the spilInit routine will exit prior
    * to any other threads are created (including stack_aud).
    *
    * re-use of the spil_ini/spil_aud stack saves some RAM overhead.
    */
   static const size_t stack_size_ini = STACK_SIZE_INI; // So MISRA doesn't complain
   VITAL_IS_OK(pthread_attr_setstack(&pthread_attr, stack_ini, stack_size_ini));
#endif
   ABQ_TRACE_MSG("pthread_create: initSpil");
   static void ** nullptrptr = NULL; // So MISRA doesn't complain about macro arguments
   VITAL_IS_OK(pthread_create(&thread_id, &pthread_attr, initSpil, (void*)nullptrptr));
   VITAL_IS_OK(pthread_attr_destroy(&pthread_attr)) ;
   VITAL_IS_OK(pthread_join(thread_id, nullptrptr));

   // let the client know that the other threads can be allocated now
   if (aqSpilThrStatusOK != aqSpilThrAckReady()){
       ABQ_FATAL_MSG("aqSpilThrAckReady failure");
   }
   // FIX_ME send debug message that we're ready
   debugMsg("Spil ack sent")

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
   while (1) {
       accumBytesRead = 0;
       bytesRead = 0;
       bufPtr = (uint8_t *) &event;
       while( accumBytesRead < sizeof(schedEvent_t) ) {
           bytesRead = read(isoSystemPipe.end.rd,
                            &bufPtr[accumBytesRead],
                            sizeof(schedEvent_t) - accumBytesRead );

           if ( ( (ssize_t) POSIX_ERROR ) == bytesRead) {
               bytesRead = 0;
               if (EINTR != errno) {
                   ABQ_FATAL_STATUS(errno);
               }
           } else {
               accumBytesRead += (size_t) bytesRead;
           }
       }

       switch (event.type) {
           case EV_SEM_POSTED:
               ABQ_VITAL(isValidSemHandle(event.ev.sem));

               threadWasUnblocked = false;


               for (uint8_t i=2; // FIXME magic
                    i < theResources->numThreads;
                    i++ ) {

                   t = &thrTable[i];
                   VITAL_IS_OK(pthread_mutex_lock(&t->condMutex));

                   if ( ( BLOCKED_ON_SEM == t->blockage ) &&
                        ( t->barrier.sem == event.ev.sem ) ) {
                       t->blockage = NOT_BLOCKED;
                       VITAL_IS_OK(pthread_cond_signal(&t->cond));
                       VITAL_IS_OK(pthread_mutex_unlock(&t->condMutex));
                       threadWasUnblocked = true;

                   } else if (BLOCKED_ON_SEM_LIST == t->blockage) {

                       if (t->barrier.list == event.ev.sem->list) {
                           // posted sem is member of list that thread is blocked on

                           // let thread know which sem signaled
                           t->barrier.sem = event.ev.sem;
                           t->blockage = NOT_BLOCKED;
                           VITAL_IS_OK(pthread_cond_signal(&t->cond));
                           VITAL_IS_OK(pthread_mutex_unlock(&t->condMutex));
                           threadWasUnblocked = true;
                       }
                   } else {
                       /* MISRA2012-RULE-15_7 Provide 'else' after the last 'else-if' construct */
                   }

                   if (true == threadWasUnblocked) {
                       break;
                   } else {
                       VITAL_IS_OK(pthread_mutex_unlock(&t->condMutex ));
                   }
               }

               if (false == threadWasUnblocked) {

                   VITAL_IS_OK(pthread_mutex_lock(&semTableMutex));
#ifdef __APPLE__
                   dispatch_semaphore_signal(event.ev.sem->sem);
#else
                   VITAL_IS_OK(sem_post(&event.ev.sem->sem));
#endif
                   if (NULL != event.ev.sem->list) {

                       aqSpilSemList_t            *list = event.ev.sem->list;

                       // add semaphore to queue of signaled list members
                       if (NULL == list->latest) {
                           // insert into empty queue
                           event.ev.sem->subsequent = NULL;
                           list->oldest = event.ev.sem;
                       } else {
                           // insert into existing queue
                     list->latest->subsequent = event.ev.sem;
                  }
                  list->latest = event.ev.sem;
               }

               VITAL_IS_OK(pthread_mutex_unlock(&semTableMutex));
            }

            break;

          case EV_ATOMIC_OPERATION:
              VITAL_NOT_NULL(event.ev.aop.thread);
              VITAL_NOT_NULL(event.ev.aop.func);

              t = event.ev.aop.thread;

              VITAL_IS_OK(pthread_mutex_lock(&t->condMutex));

              FATAL_IF(BLOCKED_ON_ATOMIC != t->blockage);

              t->barrier.result
                  = event.ev.aop.func(event.ev.aop.cbdata);

              t->blockage = NOT_BLOCKED;
              VITAL_IS_OK(pthread_cond_signal(&t->cond));

              VITAL_IS_OK(pthread_mutex_unlock(&t->condMutex));
              break;

         default:
            ABQ_FATAL_MSG("Invalid schedEvent_t");
            break;
      }

   }
#pragma clang diagnostic pop
}

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
static aqSpilThrStatus_t aqSpilThrAckReady(void)
{
    aqSpilThrStatus_t retval = aqSpilThrStatusOK;
    aqTcb_t *thisEntry = thisThread();
    if (NULL == thisEntry) {
        retval = aqSpilThrStatusError;
    } else {
        VITAL_IS_OK(pthread_mutex_lock(&thisEntry->condMutex));
        if (BLOCKED_ON_INIT == thisEntry->blockage) {
            /*
             *  invoke callback registered by aqSpilThrAllocate that was saved
             *  in thrTable at power-up only
             */
            thisEntry->blockage = NOT_BLOCKED;
            VITAL_IS_OK(pthread_cond_signal(&thisEntry->cond));
        }
        VITAL_IS_OK(pthread_mutex_unlock(&thisEntry->condMutex));
    }
    return retval;
}


// comment maintained in aqSpil.h
aqSpilThrStatus_t aqSpilThrSleep(uint32_t period)   // in ticks
{
    aqSpilThrStatus_t retval = aqSpilThrStatusOK;
#if defined(_WIN32)  && !defined(__SYMBIAN32__)
    int32_t ms = period / 1000;
    ms = (ms > 0) ? ms : 1;
    Sleep( ms );
#else
   if( 0 != usleep(period) ) {
       retval = aqSpilThrStatusError;
   }
#endif
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
    if( ( NULL != dst ) && ( NULL != src ) ) {
        size_t sz = strlen( src );
        sz = ( sz >= (size_t) SPIL_OBJECT_NAME_SZ ) ? ( (size_t) SPIL_OBJECT_NAME_SZ - 1u ) : sz;
        (void) memset(dst, 0, SPIL_OBJECT_NAME_SZ);
        (void) memcpy(dst, src, sz);
    }
}

void aqSpilThrSetName(aqTcb_t *thr, const char * name) {
    if( (NULL != thr ) && ( NULL != name ) ) {
        aqSpilSetName(thr->name,name);
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
aqSpilSem_t      *aqSpilSemAllocate(void)
{
   static uint32_t             semAllocatedCount = 0;
   aqSpilSem_t                *returnValue = NULL;

   if( NULL != theResources ) {
       if ( POSIX_OK == pthread_mutex_lock(&semTableMutex) ) {

           // return handle of next available semaphore
           if (semAllocatedCount >= theResources->numSemas) {
               semAllocatedCount = theResources->numSemas;
           } else {
               /* The following 2 lines are retarded because of MISRA2012-RULE-13_3 */
               returnValue = &semTable[semAllocatedCount];
               semAllocatedCount++;
           }

           if ( POSIX_OK != pthread_mutex_unlock(&semTableMutex) ) {
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
aqSpilSemList_t  *aqSpilSemRegisterList(aqSpilSem_t **semCollection)
{
   aqSpilSem_t                *semListElement = NULL;
   aqSpilSemList_t            *semList     = NULL;
   uint16_t                   i=0;
   aqSpilSemList_t            *returnValue = NULL;

   if ( ( NULL != semCollection ) && ( NULL != *semCollection ) ) {

       if ( POSIX_OK == pthread_mutex_lock(&semTableMutex) ) {

           // verify that a list is available in semListTable
           for (i = 0; i < theResources->numSemLists; i++) {
               if (NULL == semListTable[i].head) {
                   semList = &semListTable[i];
                   break;
               }
           }

           // traverse and validate proposed semaphore list
           i=0;
           semListElement = semCollection[i];
           while (semListElement != NULL) {
               // validate sem handle and verify sem is not already part of a list
               if ( ( false == isValidSemHandle(semListElement) ) || ( NULL != semListElement->nextMember ) ) {
                   break;
               }
               i++;
               semListElement = semCollection[i];
           }

           /* NOTE: if previous while loop terminated properly, then semListElement will == NULL here */
           if ( ( NULL == semListElement ) && ( NULL != semList ) ) {
               // proposed list was valid and have a place to store it, so register list
               i=0;
               semList->head = semListElement;
               semListElement = semCollection[0];
               semListElement -> list = semList;

               /* Populate the semList */
               while (NULL != semListElement ) {
                   aqSpilSem_t *nextSemListElement = semCollection[i + 1u];

                   if( NULL != nextSemListElement ) {
                       /* Record next member and set next member's list */
                       semListElement->nextMember = nextSemListElement;
                       nextSemListElement->list = semList;
                   }

                   /* Move to next list element */
                   i++;
                   semListElement = semCollection[i];
               }
               returnValue = semList;
           }

           if ( POSIX_OK != pthread_mutex_unlock(&semTableMutex) ) {
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
aqSpilSemStatus_t aqSpilSemPost(aqSpilSem_t *sem)
{
    /* Insure writes to SPIL event thread is atomic. e.g. not interleaved with
     * another write to the spil event thread.
     */
#if defined(_WIN32) && !defined(__SYMBIAN32__)
#else /* !Windows */
    ABQ_VITAL( sizeof( schedEvent_t ) < (size_t) _POSIX_PIPE_BUF );
#endif /* !Windows */

    schedEvent_t               event;
    ssize_t                    bytesWritten=0;
    aqSpilSemStatus_t          spilStatus = aqSpilSemStatusOK;



    if ( ( true != isValidSemHandle(sem) ) ||
         ( ( NULL != sem->list ) &&
           ( ( sem == sem->list->latest ) ||  ( NULL != sem->subsequent ) )
           ) ) {
        spilStatus = aqSpilSemStatusError;
    }

    if( aqSpilSemStatusOK == spilStatus ) {
        // format an event message
        event.type   = EV_SEM_POSTED;
        event.ev.sem = sem;

        // send event message to pipe
        bytesWritten = write(isoSystemPipe.end.wd, &event, sizeof(event));
        while ( ( sizeof(event) != (size_t) bytesWritten ) && ( spilStatus == aqSpilSemStatusOK ) ) {
            if ( ( -1 != bytesWritten ) || ( EINTR != errno ) ) {
                /* not EINTR or odd size written. Should not happen since the
                 * write should be atomic (event is < _POSIX_PIPE_BUF )
                 */
                spilStatus = aqSpilSemStatusError;  // not caused by EINTR
            } else {
                /* Interrupted .... try again */
                bytesWritten = write(isoSystemPipe.end.wd, &event, sizeof(event));
            }
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
aqSpilSemStatus_t aqSpilSemWait(aqSpilSem_t *sem)
{

   aqTcb_t             *t = thisThread();
   posixReturnCode_t          posixCode=POSIX_OK;         // Posix return code
   aqSpilSemStatus_t sem_status = aqSpilSemStatusOK;

   VITAL_NOT_NULL(t); /* parasoft-suppress MISRAC2012-DIR_4_7-a-2 "m0049. Macro is checking for thisThread() return value." */

   if (true != isValidSemHandle(sem)) {
       sem_status = aqSpilSemStatusError;
   }

   if( aqSpilSemStatusOK == sem_status ) {
       VITAL_IS_OK(pthread_mutex_lock(&t->condMutex));

       VITAL_IS_OK(pthread_mutex_lock(&semTableMutex));

#ifdef __APPLE__
       posixCode = dispatch_semaphore_wait(sem->sem, DISPATCH_TIME_NOW);
#else
       posixCode = sem_trywait(&sem->sem);
       while ( POSIX_OK  != posixCode ) {
           if (EAGAIN == errno) {
               break;              // semaphore is locked already
           }
           if (EINTR != errno) {
               ABQ_FATAL_STATUS(errno);
           }
           posixCode = sem_trywait(&sem->sem);
       }
#endif

       if (POSIX_OK == posixCode) {
           /* This thread just successfully locked the semaphore. Remove
            * this thread from the semaphore waiters. Release the lock
            * on the semaphore table mutex, and this thread's condition
            * variable. Continue running ... not need to stop.
            */
           if ( NULL != sem->list ) {
               /** Addressing MISRA2012-RULE-13_5 "Function 'removeSemFromQueue'
                * called in the right-hand operand of a logical '&amp;&amp;'
                * operator may cause side effect". Don't try to re-optomize!
                */
               VITAL_NOT_NULL( removeSemFromQueue(sem) );
           }

           VITAL_IS_OK(pthread_mutex_unlock(&semTableMutex));

           VITAL_IS_OK(pthread_mutex_unlock(&t->condMutex));


       } else {
           /* Semaphore isn't currently pending. Chill until this
            * thread can acquire it.
            */

           VITAL_IS_OK(pthread_mutex_unlock(&semTableMutex));

           /* Set thread variables. These will be modified by the
            * SPIL thead when the targetted sem is available. and
            * this thread is highest priority of waiting threads.
            */
           t->blockage = BLOCKED_ON_SEM;
           t->barrier.sem = sem;

           /* Wait for the SPIL thread to set t->blockage back to NOT_BLOCKED
            */
           while (NOT_BLOCKED != t->blockage) {
               VITAL_IS_OK(pthread_cond_wait(&t->cond, &t->condMutex));
           }

           VITAL_IS_OK(pthread_mutex_unlock(&t->condMutex));
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
aqSpilSem_t      *aqSpilSemListWait(aqSpilSemList_t *semList)
{

   aqTcb_t             *t = thisThread();
   aqSpilSem_t                *sem=NULL;

   VITAL_NOT_NULL(t); /* parasoft-suppress MISRAC2012-DIR_4_7-a-2 "m0050. Macro is checking for thisThread() return value." */

   if (true == isValidSemListHandle(semList)) {
       VITAL_IS_OK(pthread_mutex_lock(&t->condMutex));

       VITAL_IS_OK(pthread_mutex_lock(&semTableMutex));

       sem = semList->oldest;
       if ( NULL != sem ) {
           // a semaphore has been already signaled

#ifdef __APPLE__
           dispatch_semaphore_wait(sem->sem, DISPATCH_TIME_FOREVER);
#else
           while ( POSIX_OK != sem_trywait(&sem->sem) ) {
               if (EINTR != errno) {
                   ABQ_FATAL_STATUS(errno);
               }
           }
#endif
           VITAL_NOT_NULL(removeSemFromQueue(sem));

           VITAL_IS_OK(pthread_mutex_unlock(&semTableMutex));

           VITAL_IS_OK(pthread_mutex_unlock(&t->condMutex));

       } else {
           /* No semaphores in semList are pending. This thread will block
            * until one of the sems in the semList is pending.
            */

           VITAL_IS_OK(pthread_mutex_unlock(&semTableMutex));

           t->blockage = BLOCKED_ON_SEM_LIST;
           t->barrier.list = semList;

           while (NOT_BLOCKED != t->blockage) {
               VITAL_IS_OK(pthread_cond_wait(&t->cond, &t->condMutex));
           }

           sem = t->barrier.sem; /* save result before unlocking mutex */

           VITAL_IS_OK(pthread_mutex_unlock(&t->condMutex));
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
aqSpilSemStatus_t aqSpilSemTimedWait(aqSpilSem_t *sem,
                                     uint32_t interval)
{

   aqTcb_t             *t = thisThread();

   struct timespec            to;                // Posix timespec for timeout
   aqSpilClkRtc               ts_now;            // Posix timespec - current time
   posixReturnCode_t          posixCode=0;       // Posix return code
   aqSpilSemStatus_t          sem_status = aqSpilSemStatusOK;
    // force termination if this method is used.  Requires timeout to be funtional
    assert(0);

   if (true != isValidSemHandle(sem) ) {
       sem_status = aqSpilSemStatusError;
   }

   if( aqSpilSemStatusOK != sem_status ) {
#if 0 // FIXME timed wait is now broken!
       // calculate absolute time for timeout
       VITAL_IS_OK(aqSpilClkGetRtc(&ts_now));
#endif

       to.tv_sec  += ts_now.rtc.tv_sec;  // FIX_ME range check for tv_sec & tv_nsec
       to.tv_nsec += ts_now.rtc.tv_nsec;
       if (to.tv_nsec >= 1000000000) { // FIX_ME magic number
           to.tv_nsec  -= 1000000000;   // FIX_ME magic number
           to.tv_sec++;
       }

       VITAL_IS_OK(pthread_mutex_lock(&t->condMutex));

       VITAL_IS_OK(pthread_mutex_lock(&semTableMutex));

#ifdef __APPLE__
       assert(0);
#else
       posixCode = sem_trywait(&sem->sem);
       while ( POSIX_OK != posixCode ) {
           if (EAGAIN == errno) {
               break;              // semaphore is locked already
           }
           if (EINTR != errno) {
               ABQ_FATAL_STATUS(errno);
           }
           posixCode = sem_trywait(&sem->sem);
       }
#endif

       if (POSIX_OK == posixCode) {
           /* sem was pending, cleanup and allow this thread to run */
           if ( NULL != sem->list ) {
               VITAL_NOT_NULL(removeSemFromQueue(sem));
           }

           VITAL_IS_OK(pthread_mutex_unlock(&semTableMutex));

           VITAL_IS_OK(pthread_mutex_unlock(&t->condMutex));

       } else {

           VITAL_IS_OK(pthread_mutex_unlock(&semTableMutex));

           t->blockage = BLOCKED_ON_SEM;
           t->barrier.sem = sem;

           posixCode = POSIX_OK;
           while ((!(NOT_BLOCKED == t->blockage)) && ( POSIX_OK == posixCode ) ) {
               posixCode = pthread_cond_timedwait(&t->cond, &t->condMutex, &to);
               if (POSIX_OK != posixCode ) {
                   if (ETIMEDOUT != posixCode) {
                       ABQ_FATAL_STATUS(posixCode);
                   }
                   t->blockage = NOT_BLOCKED;
               }
           }

           VITAL_IS_OK(pthread_mutex_unlock(&t->condMutex));

           if (POSIX_OK == posixCode) {
               sem_status = aqSpilSemStatusOK;
           } else {
               sem_status = aqSpilSemStatusTimeout;
           }
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
aqSpilSemStatus_t aqSpilSemGetValue(aqSpilSem_t *sem, int32_t *semValue)
{

    posixReturnCode_t          posixCode=0;         // Posix return code
    aqSpilSemStatus_t sem_status = aqSpilSemStatusOK;
    if (true != isValidSemHandle(sem)) {
        sem_status = aqSpilSemStatusError;
    } else {

        VITAL_IS_OK(pthread_mutex_lock(&semTableMutex));

#ifdef __APPLE__

        assert(0);
#else
        posixCode = sem_getvalue(&sem->sem, semValue);
#endif
        VITAL_IS_OK(pthread_mutex_unlock(&semTableMutex));

        if (POSIX_ERROR == posixCode) {
            sem_status = aqSpilSemStatusError;
        } else {
            if (*semValue < 0) {
                *semValue = 0;  // limit locked semaphore value to zero
            }
        }
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
aqSpilMtx_t      *aqSpilMtxAllocate(void)
{

   static uint32_t             mtxAllocatedCount = 0;
   aqSpilMtx_t *mutex=NULL;
   /* return handle of next available mutex */
   if( mtxAllocatedCount < theResources->numMutexes ) {
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
aqSpilMtxStatus_t aqSpilMtxLock(aqSpilMtx_t *mutex)
{
    aqSpilMtxStatus_t mtx_status = aqSpilMtxStatusOK;
    if ( (true != isValidMtxHandle(mutex) ) ||
         (POSIX_OK != pthread_mutex_lock(&mutex->mtx) ) ) {
       mtx_status = aqSpilMtxStatusError;
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
aqSpilMtxStatus_t aqSpilMtxUnlock(aqSpilMtx_t *mutex)
{
    aqSpilMtxStatus_t mtx_status = aqSpilMtxStatusOK;
    if ( (true != isValidMtxHandle(mutex) ) ||
         ( POSIX_OK != pthread_mutex_unlock(&mutex->mtx) ) ) { /* parasoft-suppress MISRAC2012-DIR_4_13-f-4 "m0051. The lock released corresponds to the lock acquired in aqSpilMtxLock()." */
       mtx_status = aqSpilMtxStatusError;
    }
    return mtx_status;
}

err_t aqSpilGetRuntimeInMillis(uint64_t *timeInMillis){
    err_t status = EUNSPECIFIED;
    static aqSpilClkContinuousTime_t now;
   // get current timestamp
    if( ( NULL != timeInMillis ) && ( 0 != spilStartTime.tv_sec ) ) {
        if(EXIT_SUCCESS == aqSpilClkGetContinuousTime(&now)){
            *timeInMillis = 1000u * ((uint64_t) now.tv_sec - (uint64_t) spilStartTime.tv_sec);

            float64_t tmp_time_in_ms = ((float64_t) now.tv_nsec - (float64_t) spilStartTime.tv_nsec);
            tmp_time_in_ms = tmp_time_in_ms / (float64_t) MS_PER_NANO_SEC;
            *timeInMillis += (uint64_t) tmp_time_in_ms;
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
aqSpilTmr_t      *aqSpilTmrAllocate(aqSpilSem_t *sem)
{
    aqSpilTmr_t                *tmr = NULL;

    if (NULL != sem) {

        VITAL_IS_OK(pthread_mutex_lock(&tmrPoolMutex));

        for (uint16_t i = 0; i < theResources->numTimerz; i++ ) {
            tmr = &tmrPool[i];
            if (NULL == tmr->sem) {
                break; // found a free timer
            }
        }

        if (tmr >= &tmrPool[theResources->numTimerz] ) {
            VITAL_IS_OK(pthread_mutex_unlock(&tmrPoolMutex));
            /* handle outside bounds of timer pool. Nullify it */
            tmr = NULL;
        } else {
            if( NULL != tmr ) {
                tmr->sem = sem;
                tmr->periodInUsecs = 0;
            }

            VITAL_IS_OK(pthread_mutex_unlock(&tmrPoolMutex));
        }
    }
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
                                 uint32_t     periodInUsecs, // in ticks
                                 bool_t      isPeriodic)
{
    aqSpilTmrStatus_t          status = aqSpilTmrStatusOK;

    if ( (NULL == timer)         // FIX_ME add check for isValidTmrHandle()
         || (0U != timer->periodInUsecs)  ||  (NULL == timer->sem)
         || (periodInUsecs < (uint32_t) SPIL_MIN_TIMER_PERIOD_IN_USEC)
         || (periodInUsecs > (uint32_t) SPIL_MAX_TIMER_PERIOD_IN_USEC) )
    {
        status = aqSpilTmrStatusError;
    } else {
// FIX_ME need range checking on timer period
        VITAL_IS_OK(pthread_mutex_lock(&tmrPoolMutex));

        timer->periodInUsecs = periodInUsecs;
        timer->isPeriodic = isPeriodic;

        // set the timeout
        (void) aqSpilClkGetContinuousTime(&timer->timeout);
        timer->timeout.tv_nsec += ((int64_t) periodInUsecs * (int64_t) NS_PER_USEC);

        VITAL_IS_OK(pthread_mutex_unlock(&tmrPoolMutex));
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
aqSpilTmrStatus_t aqSpilTmrStop(aqSpilTmr_t *timer)
{
    aqSpilTmrStatus_t status = aqSpilTmrStatusOK;

    if ( (NULL == timer)         // FIX_ME add check for isValidTmrHandle()
         ||  (NULL == timer->sem)  ) {
        status = aqSpilTmrStatusError;
    } else {

        VITAL_IS_OK(pthread_mutex_lock(&tmrPoolMutex));
        if (0U == timer->periodInUsecs) {
            status = aqSpilTmrStatusInactive;
        } else {


            timer->periodInUsecs = 0;
            timer->isPeriodic = false;

#ifdef __APPLE__
            timer->timeout.tv_sec = INT64_MAX;
#elif defined ( __CYGWIN__ )
            timer->timeout.tv_sec = INT32_MAX;
#else
            timer->timeout.tv_sec = INT32_MAX;
#endif
        }
        VITAL_IS_OK(pthread_mutex_unlock(&tmrPoolMutex));
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
aqSpilTmrStatus_t aqSpilTmrFree(aqSpilTmr_t *timer)
{
    aqSpilTmrStatus_t status = aqSpilTmrStatusOK;

    VITAL_IS_OK(pthread_mutex_lock(&tmrPoolMutex));
    if ( (NULL == timer)         // FIX_ME add check for isValidTmrHandle()
         || (NULL == timer->sem)  ) {
        status = aqSpilTmrStatusError;
    } else {
        timer->sem = NULL;
    }
    VITAL_IS_OK( pthread_mutex_unlock(&tmrPoolMutex));

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
aqSpilMq_t       *aqSpilMqAllocate(uint32_t width,
                                   uint32_t depth,
                                   uint8_t *storage,
                                   aqSpilSem_t *sem)
{
    // ASSUME: Q allocation is done at init time.  If done after threading has started, a mutex will need to be
    // added to the message queue table.
    static uint32_t mqAllocatedCount=0;

    uint32_t                    size = width * depth;
    aqSpilMq_t                 *mq = NULL;

    if ((0U != width) && (0U != depth)) {

        if (mqAllocatedCount < theResources->numQueues) {

            mq = &mqTable[mqAllocatedCount];
            mqAllocatedCount++;

            if (POSIX_OK != pthread_mutex_init(&mq->mutex, NULL)) {

                SLOG_E("pthread_mutex_init failure");
                mq = NULL;

            } else {

                mq->msgsInQueue = 0;
                mq->width = width;
                mq->depth = depth;
                mq->start = storage;
                mq->end = &storage[size];
                mq->sem = sem;
                mq->head = mq->start;
                mq->tail = mq->start;
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
aqSpilMqStatus_t  aqSpilMqEnqueue(aqSpilMq_t *msgQ,
                                  cvar_t elementAddr)
{
    aqSpilMqStatus_t           rvalue = aqSpilMqStatusOK;
    bool_t                     transitionedToNotEmpty = false;

    if ( (NULL == msgQ)       // FIX_ME add check for isValidMqHandle()
         || (0U == msgQ->width) || (NULL == elementAddr) ) {
        rvalue = aqSpilMqStatusError;
    } else {

        VITAL_IS_OK(pthread_mutex_lock(&msgQ->mutex));

        if (msgQ->depth == msgQ->msgsInQueue) { // full queue
            VITAL_IS_OK(pthread_mutex_unlock(&msgQ->mutex));
            rvalue = aqSpilMqStatusError;
        } else {

            // insert into queue
            if (msgQ->head >= msgQ->end) {
                msgQ->head = msgQ->start;
            }
            (void) memcpy((void *) msgQ->head, elementAddr, msgQ->width);
            msgQ->head = &msgQ->head[msgQ->width];

            // signal semaphore if queue transitioned from empty to not empty
            if ((0U == msgQ->msgsInQueue++) && (NULL != msgQ->sem)) {
                transitionedToNotEmpty = true;
            }

            VITAL_IS_OK(pthread_mutex_unlock(&msgQ->mutex));

            if (true == transitionedToNotEmpty) {
                if(aqSpilSemStatusOK != aqSpilSemPost(msgQ->sem)) {
                    rvalue = aqSpilMqStatusError;
                }
            }
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
aqSpilMqStatus_t  aqSpilMqDequeue(aqSpilMq_t *msgQ, void* elementAddr)
{
    aqSpilMqStatus_t rvalue = aqSpilMqStatusOK;

    if ( (NULL == msgQ)       // FIX_ME add check for isValidMqHandle()
         || (0U == msgQ->width) || (NULL == elementAddr) ) {
        rvalue = aqSpilMqStatusError;
    } else {
        VITAL_IS_OK(pthread_mutex_lock(&msgQ->mutex));
        if (0U == msgQ->msgsInQueue) { // empty queue
            rvalue = aqSpilMqStatusError;
        } else {
            // retrieve from queue
            if (msgQ->tail >= msgQ->end) {
                msgQ->tail = msgQ->start;
            }
            (void) memcpy(elementAddr, (void *) msgQ->tail, msgQ->width);
            msgQ->tail = &msgQ->tail[msgQ->width];
            msgQ->msgsInQueue--;

        }
        VITAL_IS_OK(pthread_mutex_unlock(&msgQ->mutex));
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
aqSpilMqStatus_t  aqSpilMqGetEntries(aqSpilMq_t *msgQ,
                                     uint32_t *numOfEntries)
{
    aqSpilMqStatus_t           rvalue = aqSpilMqStatusOK;
    uint32_t                    entries = 0;

    if ( (NULL == msgQ)       // FIX_ME add check for isValidMqHandle()
         || (0U == msgQ->width) || (NULL == numOfEntries) ) {
        rvalue = aqSpilMqStatusError;
    } else {

        if (POSIX_OK != pthread_mutex_lock(&msgQ->mutex)) {
            rvalue = aqSpilMqStatusError;
        } else {

            entries = msgQ->msgsInQueue;

            if (POSIX_OK != pthread_mutex_unlock(&msgQ->mutex)) {
                rvalue = aqSpilMqStatusError;
            } else {

                *numOfEntries = entries;
            }
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
aqSpilMqStatus_t  aqSpilMqFlush(aqSpilMq_t *msgQ)
{
    aqSpilMqStatus_t rvalue = aqSpilMqStatusOK;

    if ( (NULL == msgQ)      // FIX_ME add check for isValidMqHandle()
         || (0U == msgQ->width) ) {
        rvalue = aqSpilMqStatusError;
    } else {

        if (POSIX_OK != pthread_mutex_lock(&msgQ->mutex)) {
            rvalue = aqSpilMqStatusError;
        } else {

            msgQ->head = msgQ->start;
            msgQ->tail = msgQ->start;
            msgQ->msgsInQueue = 0;

            if (POSIX_OK != pthread_mutex_unlock(&msgQ->mutex)) {
                rvalue = aqSpilMqStatusError;
            }
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
int32_t aqSpilAopAtomic(aqSpilAopAtomicFunc func, cvar_t cbdata)
{

   aqTcb_t                    *t = thisThread();
   schedEvent_t               event;
   ssize_t                    bytesWritten = 0;
   int32_t                    result = EUNSPECIFIED;

   VITAL_NOT_NULL(func);
   aqTcb_t unknown_thread;
   if (NULL == t) {
       unknown_thread = (aqTcb_t) {
               .id = pthread_self(),
               .guard_one = 1,
               .cond = PTHREAD_COND_INITIALIZER,
               .guard_two = 2,
               .condMutex = PTHREAD_MUTEX_INITIALIZER,
               .guard_three = 3,
               .attr = {0},
               .guard_four = 4,
               .blockage = NOT_BLOCKED,
               .barrier = {0},
               .stackAddr = NULL,
               .stackSize = 0U,
               .priority = 0,
               .entryPoint = NULL,
               .handlesSignals = false,
               .name = "unknown"
       };
       t = &unknown_thread;
   }

   // format an event message
   event.type = EV_ATOMIC_OPERATION;
   event.ev.aop.func = func;
   event.ev.aop.cbdata = cbdata;
   event.ev.aop.thread = t;

   VITAL_IS_OK(pthread_mutex_lock(&t->condMutex));

   t->blockage = BLOCKED_ON_ATOMIC;

   /* send event message to pipe Assuming sizeof(event) is <
    * _POSIX_PIPE_BUF as defined by limits.h. Thus, if the
    * entire content didn't get written.... try [the whole thing] again.
    */
   bytesWritten = write(isoSystemPipe.end.wd, &event, sizeof(event) );
   while (sizeof(event) != (size_t) bytesWritten)
   {
       if ((-1 != bytesWritten) || (EINTR != errno)) {
           ABQ_FATAL_STATUS(abq_status_pop());
       }
       bytesWritten = write(isoSystemPipe.end.wd, &event, sizeof(event) );
   }

   while (NOT_BLOCKED != t->blockage) {
       VITAL_IS_OK(pthread_cond_wait(&t->cond, &t->condMutex));
   }

   result = t->barrier.result;

   VITAL_IS_OK(pthread_mutex_unlock(&t->condMutex));

   return result;
}

int32_t aqSpilDiaGetClientPriority(void)
{
   return (int32_t) clientThread.priority;
}

#ifdef AQ_ONGS_STATISTICS
void     aqSpilRsStartExecTiming(aqSpilRsThrID_t mask, bool_t discard)
{
#ifdef __QNXNTO__
   uint64_t  now = ClockCycles();
#else
   #error Undefined timestamp reference
#endif

   if (0 != (atomic_set_value(&activeThreads, mask) & (mask - 1)) &&
         (false == discard)) {
      uint64_t  deltaT = now - *(uint64_t*) &startTime;
      aqSpilRsCyclesUsed + deltaT > ULONG_MAX ?
         aqSpilRsCyclesUsed = ULONG_MAX : atomic_add(&aqSpilRsCyclesUsed, deltaT);
   }
   *(uint64_t*) &startTime = now;
}

void     aqSpilRsStopExecTiming(aqSpilRsThrID_t mask)
{
#ifdef __QNXNTO__
   uint64_t  now = ClockCycles();
#else
   #error Undefined timestamp reference
#endif
   uint64_t  deltaT = now - *(uint64_t*) &startTime;

   aqSpilRsCyclesUsed + deltaT > ULONG_MAX ?
      aqSpilRsCyclesUsed = ULONG_MAX : atomic_add(&aqSpilRsCyclesUsed, deltaT);
   *(uint64_t*) &startTime = now;
   atomic_clr(&activeThreads, mask);
}
#endif // ifdef AQ_ONGS_STATISTICS

/*****************************************************************************
 *
 *  SPIL private functions
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  Function Name:      posixThreadEntryPoint
 *
 *  Description:        Entry point passed to Posix for thread creation
 *
 *                      FIX_ME <detailed function description>
 *
 *  Input Arguments:    arg
 *                         Pointer to thread table entry for this thread
 *
 *  Output Arguments:   None
 *
 *  Return Value:       cvar_t
 *                         FIX_ME <detailed return value information>
 *
 *  Global Variables:   thrTable
 *                      FIX_ME <list other global variables>
 *
 *  Comments:           *** Must be re-entrant ***
 *                      The function prototype matches that specified
 *                      for pthread_create.
 *
 *****************************************************************************/
static void*  posixThreadEntryPoint(void* arg) /* parasoft-suppress MISRAC2012-RULE_8_13-a-4 "m0052. Must comply with pthread_create function callback type" */
{
    VITAL_NOT_NULL(arg);
#if 0
    /* parasoft-begin-suppress MISRA2012-RULE-11_5-4 "Converting pointer to known object type; static callback function." */
   aqTcb_t             *thread = (aqTcb_t *) arg;
    /* parasoft-end-suppress MISRA2012-RULE-11_5-4 */
   EXPECT_VALUE(thread->id, pthread_self());
   thread->id = pthread_self();
#else
   aqTcb_t             *thread = thisThread();
   ABQ_VITAL(arg == (void*)thread);
#endif
   VITAL_NOT_NULL(thread);
   ABQ_TRACE_MSG_Z("posixThreadEntryPoint: ", thread->name);


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"


    /* Inform bind OS that the thread is ready to start (for all threads but the SPIL kernel.  It
     * must do some setup before bind OS can continue.  The bind ack is called inside the SPIL
     * kernel entry */
   if (theResources->tcbs != thread) {
       ABQ_VITAL(aqSpilThrAckReady() == aqSpilThrStatusOK);
   }

   while (1) {
      // transfer control to assigned entry point
      thread->entryPoint(arg); // assumed to be valid function pointer
   }
#pragma clang diagnostic pop

   return NULL;
}

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
 *  Initialize all threads, semaphores, semaphore lists, ant timers that
 *  were passed into the SPIL at the aqSpilBindOs API call.
 *
 * @param pStatus This is a pointer to a aqSpilInitStatus_t. If not NULL,
                  the initialization result code will be populated here.
 * @return FIXME -- BAD MISRA form.  casting a arithmetic type to a pointer.
 */
static void*  initSpil(void* pStatus)
{
    aqSpilSem_t                *ste = NULL;
    aqSpilMtx_t                *mxe = NULL;
    pthread_mutexattr_t        mutexAttr;
    uint16_t                   i = 0;

    uint16_t mtx_idx = 0;
    uint16_t tmr_idx = 0;

    VITAL_IS_NULL(pStatus);

    ABQ_TRACE_MSG("initSpil-start");
   /*
    *  initialize semaphore data structures
    */
   ste = semTable;
   for (i = 0U; i < theResources->numSemas; i++) {

      ste->nextMember = NULL;
      ste->list       = NULL;
      ste->subsequent = NULL;

#ifdef __APPLE__
       {
           //dispatch_semaphore_t sem = ste->sem;
           ste->sem = dispatch_semaphore_create(0);
           VITAL_NOT_NULL(ste->sem);
       };
#else
      VITAL_IS_OK(sem_init(&ste->sem,
                           0,   // not shared between processes     // FIX_ME magic number
                           0));  // semaphore is initially 'locked'  // FIX_ME magic number
#endif
      ste++;
   }

   for (i = 0U; i < theResources->numSemLists; i++) {
       semListTable[i].head   = NULL;
       semListTable[i].oldest = NULL;
       semListTable[i].latest = NULL;
   }
   VITAL_IS_OK(pthread_mutexattr_init(&mutexAttr) );
   VITAL_IS_OK(pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE) );

   VITAL_IS_OK(pthread_mutex_init(&semTableMutex, &mutexAttr) );

   /*
    *  initialize mutex table
    */



   for (mtx_idx = 0U; mtx_idx < theResources->numMutexes; mtx_idx++) {
       mxe = &mtxTable[mtx_idx];
       VITAL_IS_OK(pthread_mutex_init(&mxe->mtx, &mutexAttr));
   }


   /*
    *  initialize timer pool
    */

    for( tmr_idx = 0U; tmr_idx < theResources->numTimerz; tmr_idx++ ) {
        tmrPool[tmr_idx].sem = NULL;
    }

    VITAL_IS_OK(pthread_mutex_init(&tmrPoolMutex, &mutexAttr) );

    VITAL_IS_OK(pthread_mutexattr_destroy(&mutexAttr));

    // Create the pipe that is used by the SPIL kernel to communicate with
    // the SPIL API.

#if defined(_WIN32) && !defined(__SYMBIAN32__)
    VITAL_IS_OK(_pipe(isoSystemPipe.fd, 512, _O_BINARY) );
#else
    VITAL_IS_OK(pipe(isoSystemPipe.fd) );
#endif

    ABQ_TRACE_MSG("initSpil-finish");
    return pStatus;
}

/** @private
 *  @brief Find thread table handle for calling thread
 *
 *  ## Global's Affected
 *  * thrTable
 *
 * @return A pointer to thread's thread control block.
 */
aqTcb_t *thisThread(void)
{
    aqTcb_t *thisEntry = NULL;

    if(NULL != theResources) { /* during Unit tests, 'theResources' may not be set */
        pthread_t thisThreadID = pthread_self();
        thisEntry = &thrTable[theResources->numThreads];
        // find correct entry in thread table
        while (thisEntry-- > thrTable) {
            if (0 != pthread_equal(thisThreadID, thisEntry->id)) {
                break;
            }
        }

        if (thisEntry < thrTable) {
            // not in thread table, look at client thread
            thisEntry = &clientThread;
            if (0 == pthread_equal(thisThreadID, thisEntry->id)) {
                // FIX_ME send error message to log
                thisEntry = NULL; // entry was not found!
            }
        }
    }
    return thisEntry;
}

/** @private
 *  @brief Remove semaphore from signaled semaphore queue
 *
 * ## Globals affected
 * * semTable
 * *semListTable
 *
 * ## Comments
 * * Thanks to KB
 * * The semaphore table needs to be locked outside this function.
 *
 * @param s Pointer to semaphore table entry
 * @return <pre>s</pre> if <pre>s</pre> could be removed from the
 *         semaphore queue.  NULL otherwise.
 */
static aqSpilSem_t *removeSemFromQueue(aqSpilSem_t *s)
{

   aqSpilSem_t                *previous = NULL;
   aqSpilSem_t                *retval = NULL;
   err_t status = EXIT_SUCCESS;

   // list is empty or has invalid parameters
   if ( (NULL == s->list->oldest) || (NULL == s->list->latest) ) {
       status = EUNSPECIFIED;
   }

   if( EXIT_SUCCESS == status ) {
       if (s->list->oldest == s) {
           // it's the first node

           previous = NULL;

           s->list->oldest = s->subsequent;  // remove node
       } else {
           // it's not the first node

           previous = s->list->oldest;

           while (previous->subsequent != s) {
               previous = previous->subsequent;

               // error -- did not find node
               if (previous == NULL) {
                   status = EUNSPECIFIED;
                   break;
               }
           }
           if( EXIT_SUCCESS == status ) {
               previous->subsequent = s->subsequent;  // remove node
           }
       }
   }

   if( EXIT_SUCCESS == status ) {
       if (s->list->latest == s) {
           // it's the last node
           s->list->latest = previous;

           // error -- list does not terminate correctly
           if (s->subsequent != NULL) {
               status = EUNSPECIFIED;
           }
       }
   }

   if( EXIT_SUCCESS == status ) {
       s->subsequent = NULL;
       retval = s;
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
static bool_t  isValidSemHandle(const aqSpilSem_t *sem)
{
   bool_t retval = true;
   // FIX_ME add more rigorous tests
   if ( ( sem < &semTable[0] ) || ( sem >= &semTable[theResources->numSemas] ) ) {
       retval = false;
   }

   return retval;
}

/** @private
 * @brief Check that sem list handle is valid
 *
 * ## Globals affected
 * * semListTable
 *
 * @param list Pointer to semaphore list table entry
 * @return <pre>true</pre> if the provided semaphore list pointer is a
 *         valid handle.  <pre>false</pre> otherwise.
 */
static bool_t  isValidSemListHandle(const aqSpilSemList_t *list)
{
    bool_t retval = true;
    // FIX_ME add more rigorous tests
    if ( ( list < &semListTable[0] ) || ( list >= &semListTable[theResources->numSemLists] ) ) {
        retval = false;
    }
    return retval;
}

/** @private
 * @brief Check that mutex handle is valid
 *
 * ## Globals affected
 * * mtxTable
 *
 * @param mtx Pointer to mutex pool entry
 * @return <pre>true</pre> if the provided mutex pointer is a
 *         valid handle.  <pre>false</pre> othewise.
 */
static bool_t  isValidMtxHandle(const aqSpilMtx_t *mtx)
{
   bool_t retval = true;

   // FIX_ME add more rigorous tests
   if ( (mtx < &mtxTable[0]) || (mtx >= &mtxTable[theResources->numMutexes] ) ) {
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
static aqSpilThrStatus_t  allocateThread( void ) {
    aqSpilThrStatus_t retval = aqSpilThrStatusOK;
    static int32_t entriesAllocated = 0;
    aqTcb_t *thisEntry = &thrTable[entriesAllocated];
    static uint8_t    stack_iso[STACK_SIZE_ISO];
#ifdef RT_THREADS
    struct sched_param schedParams = {0};
#endif
    posixReturnCode_t posixCode = POSIX_OK;         // Posix return code

    if ( entriesAllocated >= (int32_t) theResources->numThreads ) {
        retval = aqSpilThrStatusError;
    } else {

        entriesAllocated++;

        // initialize thread table entries with common values
        thisEntry->blockage = BLOCKED_ON_INIT;
        thisEntry->barrier.sem = NULL;
        thisEntry->guard_one=1;
        thisEntry->guard_two=2;
        thisEntry->guard_three=3;
        thisEntry->guard_four=4;
        if ( ( POSIX_OK != pthread_cond_init(&thisEntry->cond, NULL) ) ||
             ( POSIX_OK != pthread_mutex_init(&thisEntry->condMutex, NULL) ) ||
             ( POSIX_OK != pthread_attr_init(&thisEntry->attr))) {
            retval = aqSpilThrStatusError;
        }
    }


    if( aqSpilThrStatusOK == retval ) {
        if (thrTable == thisEntry) { // first iteration only
            aqTcb_t *c = &clientThread;
            aqTcb_t *t = thrTable;
            int32_t clientSchedPolicy = 0;
            struct sched_param clientSchedParams;
            aqSpilThreadAttr_t *a = theResources->threadAttrs;

            /*
             *  initialize partial thread table entry for client
             */

            (void) memset(c, 0, sizeof(aqTcb_t));  // zero out unused elements
            (void) memset(stack_iso, 0, sizeof(stack_iso));  // zero out unused elements

            c->id = pthread_self();
            c->blockage = NOT_BLOCKED;
            if ( (POSIX_OK !=
                  pthread_getschedparam(c->id, &clientSchedPolicy, &clientSchedParams ) ) ||
                 // ( clientSchedParams.sched_priority >= THREAD_PRIORITY_CP )  ||  FIXME: client (caller of bind os) priority
                 ( POSIX_OK != pthread_cond_init(&c->cond, NULL ) ) ||
                 ( POSIX_OK != pthread_mutex_init(&c->condMutex, NULL ) ) ) {
                retval = aqSpilThrStatusError;
            }


            if( aqSpilThrStatusOK == retval ) {
                c->priority = clientSchedParams.sched_priority;

                /*
                 *  initialize elements in thread table that are not common to all threads
                 *
                 *  note: threads must be assigned from highest priority to lowest priority
                 */

                t->priority = (int32_t) THREAD_PRIORITY_ISO;
                t->handlesSignals = false;
                t->stackAddr = (byte_t*) stack_iso;
                t->stackSize = STACK_SIZE_ISO;
                aqSpilThrSetName(t, "ISO");
                t->entryPoint = aqSpilEntryPoint;
                t++; /* MISRA hates (t++)->entryPoint = ... */

                t->priority = (int32_t) THREAD_PRIORITY_AUD;
                t->handlesSignals = false;
                t->stackAddr = (byte_t*) stack_aud;
                t->stackSize = STACK_SIZE_AUD;
                aqSpilThrSetName(t, "AUD");
                t->entryPoint = aqSpilTimerThreadEntryPoint;
                t++; /* MISRA hates (t++)->entryPoint = ... */

                uint16_t n = 0;
                for (n = 0; n < (theResources->numAppThreads); n++) {
                    t->priority = a->priority;
                    t->handlesSignals = false;
                    t->stackAddr = a->stackAddr;
                    t->stackSize = a->stackSize;
                    t->entryPoint = a->entryPoint;
                    aqSpilThrSetName(t, a->name);
                    t++;
                    a++;
                }


                for(n = 0; n < theResources->numWorkerThreads; n++ ) {
                    t->priority = 10; // FIXME: need a worker thread default priority.
                    t->handlesSignals = false;
                    t->stackAddr = (byte_t*) &theResources->workerThreadStacks[n * ((theResources->workerThreadStackSz + 3U) / 4U)];
                    t->stackSize = theResources->workerThreadStackSz;
                    t->entryPoint = workerThreadEntry;
                    t++;
                }

                ABQ_DEBUG_MSG_X( "schedMin: ",
                        sched_get_priority_min(SCHED_FIFO));
                ABQ_DEBUG_MSG_X( "schedMax: ",
                        sched_get_priority_max(SCHED_FIFO));
            }
        }
    }

    if( aqSpilThrStatusOK == retval ) { /* blah */
        // set up thread attribute values for this thread
        ABQ_TRACE_MSG_Z("pthread_create: ", thisEntry->name);

#if defined(_WIN32) && !defined(__SYMBIAN32__)
#else
#ifndef __APPLE__
#ifndef RT_THREADS
        ABQ_DEBUG_MSG_PX("stack: ", thisEntry->stackAddr, thisEntry->stackSize);
        posixCode = pthread_attr_setstack(&thisEntry->attr,
                thisEntry->stackAddr, thisEntry->stackSize);
        if ( POSIX_OK != posixCode ) {
            retval = aqSpilThrStatusError;
        }
#else
        schedParams.sched_priority = thisEntry->priority;
        posixCode = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        if ( POSIX_OK == posixCode ) {
            posixCode = pthread_attr_setschedparam(&attr, &schedParams);
            if ( POSIX_OK == posixCode ) {
                posixCode = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                if( POSIX_OK == posixCode ) {
                    posixCode = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
                    if ( POSIX_OK == posixCode ) {
                        posixCode = pthread_attr_setstack(&attr, thisEntry->stackAddr, thisEntry->stackSize);
                    }
                }
            }
        }
#endif /* RT_PTHREADS */
        if( aqSpilThrStatusOK == retval ) {

            if( POSIX_OK != posixCode ) {
                ABQ_DUMP_ERROR(posixCode, "Error setting thread attributes");
                errno = posixCode;
                retval = aqSpilThrStatusError;
            }

            // fill stack with known test pattern
            (void) memset(thisEntry->stackAddr, (int8_t) STACK_PATTERN, thisEntry->stackSize);
        }
#endif /* ifndef __APPLE__ */
#endif /* if not Windows */
    }

    if( aqSpilThrStatusOK == retval ) {

        posixCode = pthread_create(&thisEntry->id, &thisEntry->attr,
                                   posixThreadEntryPoint, thisEntry);
        // create thread
        if ( POSIX_OK != posixCode ) {
            ABQ_DUMP_ERROR(posixCode, "unable to create thread");
            retval = aqSpilThrStatusError;
        }
    }
    return retval;
}

#define MAX_IF_NAME_SZ (16U)
err_t aqSpilGetIfAddr( const char* if_name, char* addr, uint8_t addr_sz )
{
    err_t retval = EINVAL;

    if( (NULL != if_name) && (NULL != addr) && (addr_sz > 0U) &&
        (strlen( if_name ) < MAX_IF_NAME_SZ) ) {

        const char *if_prefix = "/sys/class/net/";
        const char *if_postfix = "/address";
        if (utf8_starts_with(if_name,
                (int32_t)MAX_IF_NAME_SZ, "HW_OCOTP", 8)) {
            if_prefix= "/sys/fsl_otp/";
            if_postfix = "";
        }

        /* Assume that total length of if_prefix + if_postfix <= 32 */
        char if_path[ 32U + MAX_IF_NAME_SZ + 1U];

        (void) memset( addr, 0, addr_sz );
        (void) strcpy( if_path, if_prefix );
        (void) strcat( if_path, if_name );
        (void) strcat( if_path, if_postfix );

        sf_file_t f = {0};
        if( 0 == sf_open_file( if_path, SF_MODE_READ, &f ) ) {
            size_t total_bytes_read = 0;
            size_t bytes_read = 0;
            do {
                if( 0 !=
                    sf_read_buffer( &f, (uint8_t*) &addr[total_bytes_read], addr_sz - total_bytes_read, &bytes_read ) ) {
                    break;
                } else {
                    if( bytes_read > 0U ) {
                        total_bytes_read += bytes_read;
                    }
                }
            } while( bytes_read > 0U );
            retval = EXIT_SUCCESS;
            (void) sf_close( &f );
        } else {
            retval = EUNSPECIFIED;
        }
    }
    return retval;
}

#define ADDR_PREFIX_SZ (14U)
#define TMP_ADDR_SZ ( (SPIL_DEVICE_ADDR_BUF_SZ - 1U) - ADDR_PREFIX_SZ )

static size_t strip_cr_lf( char* tmp, size_t len) {
    size_t i = 0U;
    while (i < len) {
        if ((tmp[i] == (char) 0x0d)
                || (tmp[i] == (char) 0x0a)
                || (tmp[i] == (char) 0x00)) {
            tmp[i] = '\0';
            break;
        }
        i++;
    }
    return i;
}


const char* aqSpilGetDeviceAddr( void )
{
    // would like to make return to aqSpilGetDeviceAddr a const char* ... but
    // items that need to consume this were created earlier with char*
    // expected.  Thus, double declaring so the cached copy remains untouched
    // ASSUMING something isn't trashing memory!
    static char device_addr[SPIL_DEVICE_ADDR_BUF_SZ] = {0};

    char tmp[TMP_ADDR_SZ]={0};
    /* If device_address has alredy been determined once, return the same value */
    if( 0 == strcmp( device_addr, "" ) ) {
        bool got_cpuid=false;

        // Query some devices that shoudl have unique identification to get the device specific
        // device id and prepend a device specific prefix to make for global uniqueness.
        if( EXIT_SUCCESS == aqSpilGetIfAddr("HW_OCOTP_CFG1", tmp, TMP_ADDR_SZ) ) {
            size_t len = strip_cr_lf( tmp, strnlen(tmp, TMP_ADDR_SZ-1U) );
            if(len >= sizeof(tmp)) {
                // Overflow
            } else {
                tmp[len] = ':';
                len++;
                // IMX CPU id is in two parts, concatenate the two to create the 64-bit ID
                if( EXIT_SUCCESS == aqSpilGetIfAddr("HW_OCOTP_CFG0",
                        &tmp[len], ((uint8_t)sizeof(tmp) - (uint8_t)len)) ) {
                    got_cpuid = true;
                }
            }

        }

        if( got_cpuid ) {
            /* do nothing */
        } else if( EXIT_SUCCESS == aqSpilGetIfAddr( "wlan0", tmp, TMP_ADDR_SZ ) ) {
            /* do nothing */
        } else if (EXIT_SUCCESS == aqSpilGetIfAddr( "eth0", tmp,  TMP_ADDR_SZ ) ) {
            /* do nothing */
        } else if (EXIT_SUCCESS == aqSpilGetIfAddr( "enp0s3", tmp, TMP_ADDR_SZ ) ) {
            /* do nothing */
        } else {
            /* not wlan0, eth0, enp0s3 ... make up a device id */
            (void) strcpy( tmp, "f0:0b:ar:01:02:03" );
        }

        /* Strip CR and or LF */
        (void) strip_cr_lf( tmp, strnlen(tmp, TMP_ADDR_SZ-1U) );

        /* copy temporary device id to the module static device_addr */
        (void) strcpy( device_addr, tmp );
    }

    // return the address
    return device_addr;
}

