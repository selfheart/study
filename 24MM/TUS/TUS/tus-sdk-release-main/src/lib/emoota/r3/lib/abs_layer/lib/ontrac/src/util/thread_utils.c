//#line 2 "util/thread_utils.c"
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
 * @file ontrac/util/thread_utils.c
 * @date Jun 30, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {Help Me}
 *
 */

#include <ontrac/util/thread_utils.h>
#include <ontrac/ontrac/status_codes.h>

static abq_mutex_t global_mutex = {
        .mutex = NULL,
        .lock = NULL,
        .unlock = NULL,
};

err_t ontrac_reg_global_mutex(ontrac_mutex_ptr new_mutex,
                              mutex_locking_func_t new_lock,
                              mutex_unlocking_func_t new_unlock){
    err_t retval = EXIT_SUCCESS;
    ontrac_mutex_ptr old_mutex = global_mutex.mutex;
    mutex_unlocking_func_t old_lock  = global_mutex.lock;
    mutex_unlocking_func_t old_unlock  = global_mutex.unlock;
    // wait for other threads to release the previous mutex
    if((NULL != old_lock) && (NULL != old_unlock)) {
        (void) old_lock(old_mutex);
    }
    if ((NULL == new_lock) || (NULL == new_unlock)) {
        retval = EFAULT;
    } else if((new_mutex == global_mutex.mutex)
            && (new_lock == global_mutex.lock)
            && (new_unlock == global_mutex.unlock)) {
        retval = EALREADY; // So that unit-tests don't fail
    } else {
        // Overwrite the new mutex
        global_mutex.mutex = new_mutex;
        global_mutex.lock = new_lock;
        global_mutex.unlock = new_unlock;
    }
    // Release the old mutex
    if((NULL != old_lock) && (NULL != old_unlock)) {
        (void) old_unlock(old_mutex);
    }
    return retval;
}

static int32_t default_mutex_lock(ontrac_mutex_ptr mutex) {
    int32_t retval = (int32_t) EXIT_SUCCESS;
    if((default_mutex_lock != global_mutex.lock)
            || (mutex != global_mutex.mutex)) {
        retval = (int32_t) EINVAL;
    }
    return retval;
}
static int32_t default_mutex_unlock(ontrac_mutex_ptr mutex) {
    int32_t retval = (int32_t) EXIT_SUCCESS;
    if((default_mutex_unlock != global_mutex.unlock)
            || (mutex != global_mutex.mutex)) {
        retval = (int32_t) EINVAL;
    }
    return retval;
}
abq_mutex_t* ontrac_mutex_get( void ){
    if (NULL == global_mutex.lock) {
        err_t status = ontrac_reg_global_mutex(NULL,
                default_mutex_lock, default_mutex_unlock);
        if (status_code_is_error(status)) {
            ABQ_FATAL_STATUS(status);
        }
    }
    return &global_mutex;
}
