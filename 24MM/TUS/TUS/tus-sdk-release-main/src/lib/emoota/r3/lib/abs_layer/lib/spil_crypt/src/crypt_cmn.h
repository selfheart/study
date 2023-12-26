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

#ifndef _CRYPT_CMN_H_
#define _CRYPT_CMN_H_

#include <spil_file/abq_files.h>

void crypt_cmn_set_trusted_time_ms(uint64_t trusted);

err_t crypt_verify_signature_ex(const char *cert,
		const uint8_t *digest, size_t digest_len,
		const char *signature, bool_t skip_crl);

void crypt_mutex_lock(void);
void crypt_mutex_unlock(void);

// check if cert validity check should be skipped for a host
bool_t tls_skip_valid_period_check(cstr_t host);
// Tell crypt_cmn that a TLS connection is either established or failed.
void tls_handshake_done(cstr_t host);

#endif /* _CRYPT_CMN_H_ */
