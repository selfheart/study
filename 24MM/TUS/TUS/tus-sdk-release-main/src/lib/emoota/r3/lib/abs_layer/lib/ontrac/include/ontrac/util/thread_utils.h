/****************************************************************************
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
 ****************************************************************************/
/**
 * @file util/thread_utils.h
 * @date Jun 30, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief Meant to be a wrapper for spil_os when it is available
 *
 */

#ifndef ONTRAC_ONTRAC_THREAD_UTILS_H_
#define ONTRAC_ONTRAC_THREAD_UTILS_H_

#include "platform/platformconfig.h"

/** typedef for ontrac_mutex_ptr */
typedef cvar_t ontrac_mutex_ptr;

/**
 * Function type definition for mutual exclusion lock callback.
 *
 * @param mutex: pointer to a mutex to be locked by current thread
 * @retval 0 on Success.
 * @retval Non-zero on error
 */
typedef int32_t (*mutex_locking_func_t) (ontrac_mutex_ptr mutex);
/**
 * Function type definition for mutual exclusion unlock callback.
 *
 * @param mutex: pointer to a mutex to be unlocked by current thread
 * @retval 0 on Success.
 * @retval Non-zero on error
 */
typedef int32_t (*mutex_unlocking_func_t) (ontrac_mutex_ptr mutex);

/**
 * @brief a container used to perform various duties that a mutex is expected to perform
 */
typedef struct {
	ontrac_mutex_ptr mutex;
	mutex_locking_func_t lock;
	mutex_unlocking_func_t unlock;
} abq_mutex_t;

/**
 * System mutex semaphore that will be utilized to protect data structures
 * internal to the Orchestrator.  This mutex will be locked/unlocked via
 * the callback functions registered via orch_reg_lock_mutex() and orch_reg_unlock_mutex.
 *
 * If neither of callbacks are registered and the mutex is not registered, it
 * is assumed that the underlying implementation insures that the DLM API is only called
 * within the context of a single thread.
 *
 * @param new_mutex A pointer to the system mutex semaphore which will be locked/unlocked via
 *              the registered callback functions.
 * @param new_lock The callback function to be used to lock a mutex semaphore
 * @param new_unlock The callback function to be used to unlock a mutex semaphore
 * @retval 0 on success
 * @retval Non-zero on error
 */
extern err_t ontrac_reg_global_mutex(ontrac_mutex_ptr new_mutex,
        mutex_locking_func_t new_lock, mutex_unlocking_func_t new_unlock);
/**
 * #brief returns a pointer to a mutex if available
 * If a global mutex has been registered with 'ontrac_reg_global_mutex' returns that mutex
 *
 * @return pointer to a mutex if available, else NULL
 */
extern abq_mutex_t* ontrac_mutex_get( void );

#endif /* ONTRAC_ONTRAC_THREAD_UTILS_H_ */
