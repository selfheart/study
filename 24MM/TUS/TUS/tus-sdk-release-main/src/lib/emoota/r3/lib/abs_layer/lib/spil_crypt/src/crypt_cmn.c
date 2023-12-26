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
 *  Copyright (c) 2017 Airbiquity Inc.  All rights reserved.
 *
 * ****************************************************************************
 */

/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#include <spil_os/aqSpilImpl.h>
#include <ontrac/ontrac/abq_str.h>
#include <ontrac/ontrac/traverser.h>
#include <ontrac/ontrac/primitives.h>
#include <ontrac/ontrac/status_codes.h>
#include <ontrac/rest/item_readers.h>
#include <ontrac/rest/item_writers.h>
#include <spil_crypt/crypt.h>
#include <spil_crypt/aq_master_key.h>

#include "crypt_plat.h"
#include "crypt_cmn.h"
#ifdef CRYPT_LOAD_ROOTCA_FROM_FILE
#include <spil_file/abq_files.h>
#define CRYPT_ROOT_CA_FILE_NAME "rootca.pem"
byte_t *rootca = NULL;
#endif

/**
 * This implementation of client authentication loads both CA and private key from
 * /data/aqconfig/client_cert.pem
 * If this file does not exist, the client would continue without client cert.
 * Backend will decide if the connection can be made or not when there is no client cert.
 */
#define CRYPT_CLIENT_CA_FILE_NAME "client_cert.pem"
byte_t *clicert = NULL;

#ifndef CRYPT_LOAD_ROOTCA_FROM_FILE
err_t crypt_init(cstr_t root_ca, size_t root_ca_len)
#else
err_t crypt_init(void)
#endif
{
    err_t err = EXIT_SUCCESS;
    size_t readlen = 0U;
    size_t clicrtlen = 0U;

    err = abq_config_filesize(CRYPT_CLIENT_CA_FILE_NAME, &clicrtlen);
    if (EXIT_SUCCESS == err) {
        // Allocate an extra byte for string terminator: '\0'
        clicrtlen ++;
        clicert = abq_malloc_ex(clicrtlen, NULL);
        if (NULL == clicert) {
            err = ENOMEM;
        } else {
            err = abq_config_read(CRYPT_CLIENT_CA_FILE_NAME,
                    ptr2uint8(clicert), clicrtlen, &readlen);
            clicert[clicrtlen] = '\0'; // Ensure the string ends with a terminator
        }
    }
    if (EXIT_SUCCESS != err) {
        SLOG_W("Failed to load client cert");
        if (NULL != clicert) {
            abq_free(clicert);
        }
        clicrtlen = 0U;
        clicert = NULL;
        // Client does not really require the client certificate. It is the server that would enforce it.
        // So ignore the error and give client a chance to still connect to a backend not requireing client certificate.
        err = EXIT_SUCCESS;
    }

#ifndef CRYPT_LOAD_ROOTCA_FROM_FILE
    if (EXIT_SUCCESS == err) {
        err = crypt_plat_init(root_ca, root_ca_len, clicert, clicrtlen);
    }
#else
    size_t rootcalen = 0U;
    if (EXIT_SUCCESS == err) {
        err = abq_config_filesize(CRYPT_ROOT_CA_FILE_NAME, &rootcalen);
    }
    if (EXIT_SUCCESS == err) {
        // Allocate an extra byte for string terminator: '\0'
        rootcalen ++;
        rootca = abq_malloc_ex(rootcalen, NULL);
        if (NULL == rootca) {
            err = ENOMEM;
        } else {
            err = abq_config_read(CRYPT_ROOT_CA_FILE_NAME,
                    ptr2uint8(rootca), rootcalen, &readlen);
        }
    }
    if (EXIT_SUCCESS == err) {
        rootca[readlen] = '\0'; // Ensure the string ends with a terminator
        err = crypt_plat_init(rootca, rootcalen, clicert, clicrtlen);
    }
#endif
    if (EXIT_SUCCESS == err) {
        err = crypt_plat_rng_init();
    }

    if (EXIT_SUCCESS == err) {
        err = crypt_plat_init_master_key();
    }
    if (EXIT_SUCCESS != err) {
        SLOG_E("Security Manager initialization NOT fully successful");
    }
    return err;
}

void crypt_deinit(void) {
    crypt_plat_rng_cleanup();
    crypt_plat_cleanup();

    crypt_plat_release_master_key();

    if (NULL != clicert) {
        abq_free(clicert);
        clicert = NULL;
    }

#ifdef CRYPT_LOAD_ROOTCA_FROM_FILE
    if (NULL != rootca) {
        abq_free(rootca);
        rootca = NULL;
    }
#endif
}

err_t crypt_generate_uuid(char *uuid, size_t buf_size)
{
	err_t err = EXIT_SUCCESS;
	uint8_t rng[CRYPT_UUID_LEN / 2U];
	uint32_t i = 0;

    if ((NULL == uuid) || (CRYPT_UUID_LEN >= buf_size)) {
        err = EINVAL;
	}

    if (EXIT_SUCCESS == err) {
        err = crypt_random_number(rng, sizeof(rng));
    }

    if (EXIT_SUCCESS == err) {
        rng[7] = (rng[7] & 0x0FU) | 0x40U;  /* version 4 */
        rng[9] = (rng[9] & 0xF3U) | 0x08U;  /* variant 1 */

        i = 0;
        while (i < CRYPT_UUID_LEN) {
            uint8_t c = rng[i / 2U];
            if ((i % 2U) == 0U) { c >>= 4U; }
            c &= 0x0FU;

            if (c <= 9U) { uuid[i] = '0' + c; }
            else { uuid[i] = ('a' + c) - 10U; }
            i++;
        }

        uuid[8] = '-';
        uuid[13] = '-';
        uuid[18] = '-';
        uuid[23] = '-';
        uuid[CRYPT_UUID_LEN] = '\0';
    }
    if (EXIT_SUCCESS != err) {
        SLOG_E("Security Manager operation failed");
    }
	return err;
}

typedef struct {
    aqSpilClkContinuousTime_t monotonic_timestamp;
    number_t trusted_timestamp;
} trusted_time_data_t;

static trusted_time_data_t *trusted_time_data(void) {
    static trusted_time_data_t data = {0};

    return &data;
}

#ifndef USEC_PER_MSEC
#define USEC_PER_MSEC (1000.0)
#endif

#ifdef USE_SYS_REALTIME_WO_TIME_SERVER
#define PREV_TIME_FILENAME "previous_trusted_time.json"
#endif

void crypt_cmn_set_trusted_time_ms(uint64_t trusted_time) {

    ABQ_VITAL(0U != trusted_time);
#ifdef USE_SYS_REALTIME_WO_TIME_SERVER
    SLOG_W("Using system real time as trusted time. This function should not be called");
#else
    trusted_time_data_t *timedata = trusted_time_data();

    (void)aqSpilClkGetContinuousTime(&timedata->monotonic_timestamp);
    timedata->trusted_timestamp = (number_t)trusted_time;
    number_t ts_sec = (number_t)trusted_time / 1000.;
    aqSpilClkRtc_t rtc = {0};
    byte_t timeStr[32];
    (void)aqSpilClkGetRtc(&rtc);
    EXPECT_IS_OK(aqSpilClkRtcToString(&rtc, timeStr, sizeof(timeStr)));
    ABQ_INFO_MSG_Z("Set trusted-time: ", timeStr);
    ABQ_INFO_MSG_X("Realtime offset(seconds): ", (ts_sec - rtc.rtc.tv_sec));
#endif
    // Time is a random source for RNG seed
    crypt_reseed_rng((const uint8_t *)&trusted_time, sizeof(trusted_time));
}

static err_t crypt_cmn_get_trusted_time_ms(uint64_t *time_ms)
{
    VITAL_NOT_NULL(time_ms);
    err_t err = EXIT_SUCCESS;
#ifdef USE_SYS_REALTIME_WO_TIME_SERVER
    byte_t jsbuf[BUFFER_SMALL] = {0};
    aqSpilClkRtc_t tts;
    crypt_mutex_lock();

    // Read system RTC and use it as the "secure time"
    if ( 0 == aqSpilClkGetRtc(&tts) ) {
        *time_ms = ((uint64_t) tts.rtc.tv_sec * 1000UL) + ((uint64_t) tts.rtc.tv_nsec / 1000000UL);
    } else {
        err = EINVAL;
    }

    /* The system RTC is not really "secure" by its nature on most of OS. Following code tries
     * to make it more secure and mimic meeting the requirement of crypt_get_time().
     * Most likely it is still not really "secure". For example, user could change the system RTC
     * or delete the "previous time" file outside the OTAmatic Client. So it is just an example
     * of implementation when the end-to-end architecture does not include the time server.
     *
     * OEM should NOT use this function as is. When time server not used, the target platform must
     * have its own secured time source, and this function must be implemented according to that
     * secured time source.
     */
    if ((EXIT_SUCCESS == err) && (1596265200000UL > *time_ms)) { // August 1, 2020 12:00:00 AM GMT-07:00
        /* This is an example of that secure time should be reasonably close to real world time.
         * The time compared with is just an example, no special meaning. It is the month when
         * this function was implemented.
         */
        SLOG_E("Trusted time does not seem to be valid");
        err = ETIME;
    }
    /* Make sure the secure time does not rollback.
     * Reading/writing a file every time when crypt_get_time() gets called is definitely not the
     * most efficient implementation. However, it's the most clear way to be an example of what the
     * requirement this function is trying to satisfy.
     */
    if (EXIT_SUCCESS == err) {
        uint64_t prevtime = 0U;
        size_t readlen = 0U;
        err = abq_resource_read(PREV_TIME_FILENAME, jsbuf, sizeof(jsbuf), &readlen);
        if (EXIT_SUCCESS == err) {
            ptree_t *pt = ptree_resolve(str_parse_json(jsbuf, (int32_t)readlen, NULL));
            if (NULL != pt) {
                cvar_t ptp = ptree_get(pt, "previous_trusted_time");
                if (NULL != ptp) {
                    prevtime = (uint64_t)number_resolve(ptp);
                }
            }
            EXPECT_IS_OK(obj_release_self(pt));
        }
        if (0U == prevtime) {
            SLOG_I("Previous trusted time has not been written yet");
            // When failed to read the previous time, treat it as if this was the first time running the app.
            // Because it is hard for the client app to really determine what went wrong.
            // as a reference design using some not really secure time source as "secure" time, this
            // is just an example anyway.
            err = EXIT_SUCCESS;
        } else {
            if (prevtime > *time_ms) {
                SLOG_E("Time rolled back!!!");
                err = ETIME;
            }
        }
    }
    if (EXIT_SUCCESS == err) {
        ptree_t *pt = ptree_create(NULL);
        number_ptr tn = number_create((number_t) *time_ms);
        if((pt == NULL) || (tn == NULL)) {
            err= abq_status_take(ENOMEM);
        } else {
            EXPECT_IS_OK(ptree_put(pt, "previous_trusted_time", tn));
            int32_t jslen = json_format_str(pt, jsbuf, (int32_t)sizeof(jsbuf));
            ABQ_VITAL(0 < jslen);
            EXPECT_IS_OK(abq_resource_write(PREV_TIME_FILENAME, jsbuf, jslen));
        }
        EXPECT_IS_OK(obj_release_self(tn));
        EXPECT_IS_OK(obj_release_self(pt));
    }
    crypt_mutex_unlock();
#else
    // calculates delta (current continuous time - continuous time when secure time last set) and applies this delta
    // to the last set secure time
    int64_t diff_in_usec = 0;
    trusted_time_data_t *timedata = trusted_time_data();

    aqSpilClkContinuousTime_t current_timestamp;
    (void)aqSpilClkGetContinuousTime(&current_timestamp);
    if(aqSpilClkStatusOK ==
            aqSpilDiffContinuousTime(&current_timestamp,
                    &timedata->monotonic_timestamp, &diff_in_usec)) {
        number_t span = ((number_t)diff_in_usec) / ((number_t)USEC_PER_MSEC);
        if (0. != timedata->trusted_timestamp) {
            *time_ms = (uint64_t) timedata->trusted_timestamp + (uint64_t) span;
        } else {
            err = EINVAL;
        }
    } else {
        err = EINVAL;
    }
#endif
    return err;
}

err_t crypt_get_time(uint64_t *time_ms) {  // parasoft-suppress  MISRAC2012-RULE_8_7-a-4 "Function for other components to use"
    VITAL_NOT_NULL(time_ms);
    return crypt_cmn_get_trusted_time_ms(time_ms);
}

#ifdef TIME_SERVER_OVER_TLS
static cstr_t skip_validity_fqdn = NULL;
static cstr_t skip_validity_proxy = NULL;
err_t crypt_disable_valid_per_verify(cstr_t fqdn, cstr_t proxy) {
    err_t err = EXIT_SUCCESS;

    if (NULL == fqdn) {
        // Any pending skipping set before will be canceled at the end of this call.
        err = ECANCELED;
    }

    if (EXIT_SUCCESS == err) {
        uint64_t time_ms = 0U;
        if (EXIT_SUCCESS == crypt_get_time(&time_ms)) {
            // Already have trusted time
            err = EINVAL;
        }
    }
    if (EXIT_SUCCESS == err) {
        if (NULL != skip_validity_fqdn) {
            (void) obj_release_self(skip_validity_fqdn);
        }
        ABQ_WARN_MSG_Z("Temporarily disable validity period check for", fqdn);
        skip_validity_fqdn = str_create(fqdn, -1, false);
        if (NULL == skip_validity_fqdn) {
            err = ENOMEM;
        }
    }
    if ((EXIT_SUCCESS == err) && (NULL != proxy)) {
        if (NULL != skip_validity_proxy) {
            (void) obj_release_self(skip_validity_proxy);
        }
        ABQ_WARN_MSG_Z("Temporarily disable validity period check for", proxy);
        skip_validity_proxy = str_create(proxy, -1, false);
        if (NULL == skip_validity_proxy) {
            err = ENOMEM;
        }
    }

    if (EXIT_SUCCESS != err) {
        if (NULL != skip_validity_fqdn) {
            (void) obj_release_self(skip_validity_fqdn);
            skip_validity_fqdn = NULL;
        }
        if (NULL != skip_validity_proxy) {
            (void) obj_release_self(skip_validity_proxy);
            skip_validity_proxy = NULL;
        }

        if (ECANCELED == err) {
            err = EXIT_SUCCESS;
        }
    }
    return err;
}

bool_t tls_skip_valid_period_check(cstr_t host) {
    bool_t skip = true;

    uint64_t time_ms = 0U;
    if (EXIT_SUCCESS == crypt_get_time(&time_ms)) {
        // Already have trusted time
        skip = false;
    }

    if ((NULL == skip_validity_fqdn) || (NULL == host)) {
        // No remote host to be skipped
        skip = false;
    }
    if (skip) {
        if (0 != utf8_compare_insensitive(host, skip_validity_fqdn, -1)) {
            if ((NULL == skip_validity_proxy) ||
                    (0 != utf8_compare_insensitive(host, skip_validity_proxy, -1))) {
                skip = false;
            }
        }
    }

    if (skip) {
        ABQ_WARN_MSG_Z("Skip validity period check for", host);
    }

    return skip;
}

void tls_handshake_done(cstr_t host) {
    bool_t release_proxy = false;
    bool_t release_fqdn = false;

    if (NULL != host) {
        if (NULL == skip_validity_fqdn) {
            release_proxy = true;
        } else {
            if (0 == utf8_compare_insensitive(host, skip_validity_fqdn, -1)) {
                release_fqdn = true;
                release_proxy = true;
            } else if ((NULL != skip_validity_proxy) &&
                    (0 == utf8_compare_insensitive(host, skip_validity_proxy, -1))) {
                release_proxy = true;
            } else {
                // Do nothing, MISRA requires `else` after `else if`
            }
        }
    }

    if (release_fqdn) {
        if (NULL != skip_validity_fqdn) {
            ABQ_INFO_MSG_Z("Next time cert validity period will be enforced:", skip_validity_fqdn);
            (void) obj_release_self(skip_validity_fqdn);
            skip_validity_fqdn = NULL;
        }
    }
    if (release_proxy) {
        if (NULL != skip_validity_proxy) {
            ABQ_INFO_MSG_Z("Next time cert validity period will be enforced:", skip_validity_proxy);
            (void) obj_release_self(skip_validity_proxy);
            skip_validity_proxy = NULL;
        }
    }
}
#endif
