/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#ifndef SDK_ABST_CONFIG_H
#define SDK_ABST_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define SLC_CFG_MATCH "/vsms/client/api/1.0/light/config"
#define SLC_SYNC_CHECK "/vsms/client/api/2.0/light/sync"
#define SLC_GET_OTA_CMD "/vsms/client/api/1.0/light/command"
#define SLC_MPU_INIT "/datacollection/api/2.0/upload/initiate"
#define SLC_MPU_COMP "/datacollection/api/2.0/upload/complete"
#define SLC_PUT_NFY "/datacollection/api/2.0/events/"

#ifndef INSTALL_BASE
#define CLIENT_BASE "/data"
#else
#define CLIENT_BASE INSTALL_BASE
#endif
#define UPTANE_BASE CLIENT_BASE "/uptane"
#define CONFIG_BASE CLIENT_BASE "/aqconfig"
#define PERSIST_BASE CLIENT_BASE "/t1_app/"
#define UPT_CACHED "/cached"
#define UPT_TRUST_ROOT "/trust_root"
#define UPT_PACKAGE "/package"

#define UPT_IMAGE_REPO "/image"
#define UPT_DIRECTOR_REPO "/director"

#ifdef __cplusplus
}
#endif
#endif /* SDK_ABST_CONFIG_H */
