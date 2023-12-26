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

/* Outside interface definitions */
#include <stddef.h>
#include <stdint.h>
#include <pthread.h>
#ifdef __APPLE__
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#endif
#include <limits.h>

// undo define ISO_DEBUG for production
//#define ISO_DEBUG

#ifdef ISO_DEBUG
   #include "aqDebugOutput"
#endif

/** @public
 * @brief Calculate minimum required stack size based upon provided desired stack size.
 *
 * Where the desired size is less than the underlying system's minimum stack size,
 * insure that the system's minimum is asserted.
 *
 * @param desiredSz The desired stack size indicated by the application.
 */
#define SPIL_STACK_SZ( desiredSz ) ( ( ( ( desiredSz ) < ( PTHREAD_STACK_MIN ) ) ? ( PTHREAD_STACK_MIN ) : ( desiredSz ) ) )

/** @brief the total space including NULL termination character that is allocated for
 * SPIL object names.
 *
 * TODO: Add names to other SPIL objects.  Currently only the TCB has a name property.
 */
#define SPIL_OBJECT_NAME_SZ ( 10u )

/*****************************************************************************
 *
 *  Airbiquity base types
 *
 *****************************************************************************/

// debug message macros
#ifdef ISO_DEBUG
   #include <stdio.h>
   #define debugMsg(dbgString) \
        { byte_t msg[] = dbgString "\r\n"; \
          aqts_DebugOutput(msg, sizeof msg - 1); }
   #define debugMsgArg(fmtString, arg) \
        { byte_t fmt[] = fmtString "\r\n"; \
          static byte_t msg[1500]; int32_t msgSize; \
          msgSize = sprintf(msg, fmt, arg); \
          aqts_DebugOutput(msg, msgSize); }
   #define debugMsg2Arg(fmtString, arg1, arg2) \
        { byte_t fmt[] = fmtString "\r\n"; \
          static byte_t msg[1500]; int32_t msgSize; \
          msgSize = sprintf(msg, fmt, arg1, arg2); \
          aqts_DebugOutput(msg, msgSize); }
#else
   #define debugMsg(dbgString)
   #define debugMsgArg(fmtString, arg)
   #define debugMsg2Arg(fmtString, arg1, arg2)
#endif

typedef int posixReturnCode_t;

/*****************************************************************************
 *
 *  Initialization
 *
 *****************************************************************************/

typedef enum aqSpilInitStatus {
   aqSpilInitStatusOK          =        0,        // explicitly zero
   aqSpilInitStatusError
} aqSpilInitStatus_t;

/*****************************************************************************
 *
 *  Thread Services
 *
 *****************************************************************************/
// operating system isolation thread (ISO)
// audio device driver abstraction thread (AUD)
// init thread (INI) (called from ISO context during system intiialization) 
#define THREAD_PRIORITY_ISO  ( 15 )
#define THREAD_PRIORITY_AUD  ( 14 )

/* Removing usage of SPIL_STACK_SZ macro to reduce MISRA errors. Desired
 * stack sizes so far end up being bumped up to PTHREAD_STACK_MIN bytes, so
 * the calculation seems redundant anyway.
 */
#define STACK_SIZE_ISO ( PTHREAD_STACK_MIN )
#define STACK_SIZE_AUD ( PTHREAD_STACK_MIN )
#define STACK_SIZE_INI ( STACK_SIZE_ISO )

/** @private
 *
 */
typedef enum thrBlockageFlag {
   NOT_BLOCKED                =        0,
   BLOCKED_ON_INIT,                              // waiting on thread allocation
   BLOCKED_ON_SEM,                               // blocked on single semaphore
   BLOCKED_ON_SEM_LIST,                          // blocked on semaphore list
   BLOCKED_ON_ATOMIC                             // blocked on atomic operation
} thrBlockageFlag_t;

/** @private
 *
 */
typedef union {
   aqSpilSem_t                *sem;               // when BLOCKED_ON_SEM
   aqSpilSemList_t            *list;              // when BLOCKED_ON_SEM_LIST
   int32_t                    result;            // when BLOCKED_ON_ATOMIC
} thrBarrier_t;

typedef void (*thrEntry)(cvar_t);
typedef err_t (*initFunc_t) (cvar_t);

/** @private
 *  the following members must be protected by the .condMutex:
 *
 *     cond, blockage, barrier
 *
 *  note: threads must be assigned from highest priority to lowest priority
 */
struct thrTableEntry {
   pthread_t                  id;                ///< Posix thread id
    uint32_t                   guard_one;
   pthread_cond_t             cond;              ///< Posix condition variable
    uint32_t                   guard_two;
   pthread_mutex_t            condMutex;         ///< Posix mutex for cond var
    uint32_t                   guard_three;
   pthread_attr_t              attr;             ///< Posix thread attributes
    uint32_t                   guard_four;
   thrBlockageFlag_t          blockage;          ///< tri-state value to record
                                                 ///< if thread is blocked and why
   thrBarrier_t               barrier;           ///< member (.sem or .list) to
                                                 ///< use determined by value
                                                 ///< of blockage

   byte_t                    *stackAddr;         ///< Posix thread stack addr
   size_t                     stackSize;         ///< Posix thread stack size

   int32_t                    priority;          ///< Posix thread priority
   thrEntry                   entryPoint;        ///< thread entry point

   bool_t                     handlesSignals;    ///< thread handles signals
    char                      name[SPIL_OBJECT_NAME_SZ]; //< name of the TCB entry
};

/** @private
 *
 */
enum stackCharacteristics {
   STACK_PATTERN              =     0xCC
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
#ifdef __APPLE__
    dispatch_semaphore_t    sem;
#else
    sem_t                   sem;
#endif
   aqSpilSem_t         *nextMember;        ///< next member of sem list
   aqSpilSemList_t     *list;              ///< sem list that sem is member
   aqSpilSem_t         *subsequent;        ///< subsequent signaled sem
};

/** @private
 *
 */
struct aqSpilSemList {
   aqSpilSem_t                *head;              ///< head of membership list
   aqSpilSem_t                *oldest;            ///< oldest signaled sem
   aqSpilSem_t                *latest;            ///< latest signaled sem
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
   pthread_mutex_t            mtx;               ///< Posix Pthread mutex
};

/** @private
 *
 */
struct aqSpilTmr {
    aqSpilSem_t                *sem;              ///< notification
    uint32_t                    periodInUsecs;     ///< in micro-seconds
    aqSpilClkContinuousTime_t  timeout;           ///< When timer will timeout
    bool_t                     isPeriodic;        ///< Determine if timer gets reloaded on timeout
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
   uint8_t                    *start;
   uint8_t                    *end;
   uint8_t                    *head;
   uint8_t                    *tail;
   size_t                     width;             ///< message size
   uint32_t                    depth;             ///< max number of messages
   uint32_t                    msgsInQueue;       ///< number of enqueued messages
   pthread_mutex_t            mutex;             ///< Posix mutex for queue
};

/*****************************************************************************
 *
 *  Pipes
 *
 *****************************************************************************/

typedef int posixFileDescriptor_t;

/** @private
 *
 */
typedef union pipeFdSet {
   posixFileDescriptor_t  fd[2];  ///< set of file descriptors needed by pipe()
   struct {
      posixFileDescriptor_t  rd;  ///< file descriptor for reading
      posixFileDescriptor_t  wd;  ///< file descriptor for writing
   } end;
} pipeFdSet;

/*****************************************************************************
 *
 *  Signals
 *
 *****************************************************************************/

/** @private
 *
 */
enum sigCharacteristics {
   // required range is within SIGRTMIN to SIGRTMAX
   SIGNAL_ISO                 =       55
};

/*****************************************************************************
 *
 *  Inter-thread Communication
 *
 *****************************************************************************/

/** @private
 *
 */
enum eventType {
   EV_SEM_POSTED,
   EV_ATOMIC_OPERATION,
   EV_UNEXPECTED_SIGNAL
};

/** @private
 * @class schedEvent_t
 * @brief data structure that is passed from the SPIL API to the SPIL kernel thread.
 */
typedef struct {
   enum eventType             type;
   union {
      aqSpilSem_t             *sem;
      struct {
         aqSpilAopAtomicFunc   func;
         cvar_t                cbdata;
         aqTcb_t              *thread;
      } aop;
      struct {
        int32_t              signo;
        int32_t              code;
      } unexpected;
   } ev;
} schedEvent_t;

#endif // _SPIL_OS_PLATFORM_H_
