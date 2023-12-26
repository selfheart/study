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

#include <platform/platformconfig.h>
#include <ontrac/ontrac/primitives.h>
#include <ontrac/ontrac/status_codes.h>
#include <spil_os/aqSpil.h>
#include <spil_crypt/crypt.h>
#include <spil_file/abq_files.h>

#include <openssl/rand.h>

#include "../crypt_plat.h"
#include "crypt_ctx.h"

err_t crypt_plat_rng_init(void) {
    err_t err = EXIT_SUCCESS;

    const char *devid = aqSpilGetDeviceAddr();
    crypt_reseed_rng((const uint8_t*)devid, (size_t)utf8_byte_length(devid, -1)); // Load and update seed file

    return err;
}

void crypt_plat_rng_cleanup(void) {
}

err_t crypt_random_number(uint8_t *rng, size_t num) {
    err_t err = EXIT_SUCCESS;

    if (1 != RAND_bytes(rng, (int32_t)num)) {
        err = EINVAL;
    }
    if (EXIT_SUCCESS != err) {
        SLOG_E("Security Manager operation failed");
    }
    return err;
}

#define RNG_SEED_FILENAME "openssl_rng_seed"
void crypt_reseed_rng(const uint8_t *extra_entropy, size_t entropy_count) {
    byte_t filepath[PLATFORM_MAX_FILE_PATH_LEN];
    ABQ_VITAL(0 < sf_path_join(OTAMATIC_RESOURCES_DIR, RNG_SEED_FILENAME,
                               filepath, sizeof(filepath), abq_file_sys_path));
    (void)RAND_poll();

    if ((NULL != extra_entropy) && (0U != entropy_count)) {
        RAND_seed((const void *)extra_entropy, (int32_t)entropy_count);
    }
    if (0 >= RAND_load_file(filepath, -1L)) {
        SLOG_W("Failed to load OpenSSL RNG SEED file");
    }
    (void)RAND_write_file(filepath);
}
