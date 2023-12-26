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
 *  Copyright (c) 2019 Airbiquity Inc.  All rights reserved.
 *
 * ****************************************************************************
 */

/*
 * This file defines some internal APIs each platform needs to implement
 */

#ifndef _CRYPT_PLAT_H_
#define _CRYPT_PLAT_H_

/**
 * Initialize platform dependent context for crypto library
 * @return 0 for success, <0 for error
 */
err_t crypt_plat_init(cstr_t trusted_cas, size_t length, cstr_t clientcert, size_t clicrtlen);

/**
 * cleanup platform dependent context for crypto library
 */
void crypt_plat_cleanup(void);

/**
 * Initialize platform dependent context for RNG
 * @return 0 for success, <0 for error
 */
err_t crypt_plat_rng_init(void);

/**
 * cleanup platform dependent context for RNG
 */
void crypt_plat_rng_cleanup(void);

/**
 * Initialize platform dependent context for master key
 * @return 0 for success, <0 for error
 */
err_t crypt_plat_init_master_key(void);

/**
 * cleanup platform dependent context for master key
 */
void crypt_plat_release_master_key(void);

#endif /* _CRYPT_PLAT_H_ */
