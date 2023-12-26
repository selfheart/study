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
 *  Copyright (c) 2018 Airbiquity Inc.  All rights reserved.
 *
 * ***************************************************************************
 */

#ifndef AQSPILRAUC_H
#define AQSPILRAUC_H

#include <spil_os/aqSpil.h>

/** @public
 * @brief Status codes returned from SPIL RAUC function
 *
 * @see aqSpilRaucInstallBundle(), aqSpilRaucCheckInstallProgress()
 */
typedef enum {
    aqSpilRaucStatusOK        = 0,         ///< explicitly zero.  Request successful.
    aqSpilRaucStatusInProgress,
    aqSpilRaucStatusSucceeded,
    aqSpilRaucStatusFailed,
    aqSpilRaucStatusError                    ///< An error occurred processing the request.
} aqSpilRaucStatus_t;

/** @public
 * @brief Start the installation of the RAUC bundle
 *
 * @param bundle String buffer with the bundle path
 * @param fd Pointer to int to return file descriptor of RAUC process stdout exit pipe
 * @return aqSpilRaucStatusOK == success, aqError on failure (bad parameters).
 */
aqSpilRaucStatus_t aqSpilRaucInstallBundle(char * bundle, int32_t *fd);

/** @public
 * @brief Get RAUC progress to report back to Update Agent
 *
 * @param percent Variable to report back progress percentage
 * @return aqSpilRaucStatusOK == success, aqError on failure (bad parameters).
 */
aqSpilRaucStatus_t aqSpilRaucCheckInstallProgress(uint32_t * percent);

/** @public
 * @brief Check RAUC process from stdout
 *
 * @param buf Buffer read from stdout
 * @return aqSpilRaucStatusInProgress == success, aqError on failure (bad parameters).
 * @return aqSpilRaucStatusucceded == RAUC process finished, ready for reboot.
 */
aqSpilRaucStatus_t aqSpilRaucUpdateInstallProgress(char * buf);

int32_t aqSpilRaucReadFd(fd_type fd, char *buffer, size_t length);

int32_t aqSpilRaucCloseFd(fd_type fd);

aqSpilRaucStatus_t aqSpilRaucTriggerReboot(void);

int32_t aqSpilRaucTerminateInstall(void);

#endif //AQSPILRAUC_H
