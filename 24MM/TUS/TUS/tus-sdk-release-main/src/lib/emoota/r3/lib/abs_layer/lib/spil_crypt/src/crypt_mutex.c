/* ****************************************************************************
 *
 *         AIRBIQUITY INC. PROPRIETARY AND CONFIDENTIAL INFORMATION
 *
 *  The software and information contained herein is the proprietary and
 *  confidential property of Airbiquity Inc. and shall not be disclosed
 *  in whole or in part. Possession, use, reproduction or transfer of
 *  this software and information is prohibited without the express written
 *  permission of Airbiquity Inc.
 *
 *  Copyright (c) 2020 Airbiquity Inc.  All rights reserved.
 *
 * ****************************************************************************
 */

#include <spil_os/aqSpil.h>
#include <ontrac/ontrac/abq_str.h>
#include <ontrac/ontrac/traverser.h>
#include <ontrac/ontrac/primitives.h>
#include <ontrac/ontrac/status_codes.h>
#include <ontrac/rest/item_readers.h>
#include <ontrac/rest/item_writers.h>
#include <spil_crypt/crypt.h>

#include "crypt_plat.h"
#include "crypt_cmn.h"

#ifdef CRYPT_USE_CUSTOMIZED_MUTEX
err_t crypt_reg_lock_mutex(crypt_lock_mutex_t lock_mutex_cb) { // parasoft-suppress CODSTA-88-3 "Pointer to callback function desired"
    VITAL_NOT_NULL(lock_mutex_cb);
	return 0;
}

err_t crypt_reg_unlock_mutex(crypt_unlock_mutex_t unlock_mutex_cb) { // parasoft-suppress CODSTA-88-3 "Pointer to callback function desired"
    VITAL_NOT_NULL(unlock_mutex_cb);
	return 0;
}

/* parasoft-begin-suppress MISRAC2012-RULE_8_13-a-4 "mutex may be changed later" */
err_t crypt_reg_mutex(void *mutex) {
/* parasoft-end-suppress MISRAC2012-RULE_8_13-a-4 */
    VITAL_NOT_NULL(mutex);
	return 0;
}
#endif /* CRYPT_USE_CUSTOMIZED_MUTEX */

// Above functions not implemented and not used yet. Just use default mutex for now.
static abq_mutex_t* crypt_mutex = NULL;
void crypt_mutex_lock(void) {
    if (NULL == crypt_mutex) {
        crypt_mutex = ontrac_mutex_get();
    }
    if (NULL != crypt_mutex) {
        EXPECT_IS_OK(crypt_mutex->lock(crypt_mutex->mutex));
    }
}

void crypt_mutex_unlock(void) {
    if (NULL != crypt_mutex) {
        EXPECT_IS_OK(crypt_mutex->unlock(crypt_mutex->mutex));
    }
}
