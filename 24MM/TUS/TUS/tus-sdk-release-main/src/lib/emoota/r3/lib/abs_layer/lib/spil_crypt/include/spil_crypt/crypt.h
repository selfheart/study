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

#ifndef CRYPT_MGR_IF_CRYPT_MGR_H
#define CRYPT_MGR_IF_CRYPT_MGR_H

#include <platform/platformconfig.h>

#ifdef __cplusplus
extern "C" {
#endif
/* General overview (including use case and sequence diagrams) in ../doc/mainpage.dox */

/**
 * @file crypt.h
 *
 * Header file for Security Manager CRYPT part. This file mainly include constant definitions.
 */
#ifndef USE_SYS_REALTIME_WO_TIME_SERVER
#include "crypt_timeserver.h"
#endif
/**
 * @brief Enumeration used to specify the action of cryptographic cipher operation
 *
 * *Some values are not required to be supported at this time but defined for future use*
 */
typedef enum {
	CRYPT_CIPHER_ENCRYPT = 1, ///< Encrypt input data.
	CRYPT_CIPHER_DECRYPT      ///< Decrypt input data. **Required**
} crypt_cipher_action_t;

/**
 * @brief Enumeration used to specify the algorithm of cryptographic cipher operation
 *
 * *Some values are not required to be supported at this time but defined for future use*
 */
typedef enum {
    CRYPT_CIPHER_ALG_NONE = 0, ///< Algorithm N/A
    CRYPT_CIPHER_ALG_AES,      ///< AES. **Required**
    CRYPT_CIPHER_ALG_DES,      ///< DES. **Required**
    CRYPT_CIPHER_ALG_3DES,     ///< 3DES
    CRYPT_CIPHER_ALG_CAMELLIA, ///< Camellia
    CRYPT_CIPHER_ALG_BLOWFISH, ///< Blowfish
    CRYPT_CIPHER_ALG_ARC4      ///< ARC4
} crypt_cipher_algorithm_t;

/**
 * @brief Enumeration used to specify the mode of cryptographic cipher operation
 *
 * *Some values are not required to be supported at this time but defined for future use*
 */
typedef enum {
    CRYPT_CIPHER_MODE_NONE = 0,    ///< Mode N/A
    CRYPT_CIPHER_MODE_ECB,         ///< ECB
    CRYPT_CIPHER_MODE_CBC,         ///< CBC. **Required**
    CRYPT_CIPHER_MODE_CFB,         ///< CFB
    CRYPT_CIPHER_MODE_OFB,         ///< OFB
    CRYPT_CIPHER_MODE_CTR,         ///< CTR
    CRYPT_CIPHER_MODE_GCM,         ///< GCM
    CRYPT_CIPHER_MODE_STREAM,      ///< STREAM
    CRYPT_CIPHER_MODE_CCM,         ///< CCM
} crypt_cipher_mode_t;

/**
 * @brief Enumeration used to specify the padding/encoding mode of cryptographic operation
 *
 * This enum included padding mode for symmetric cipher operations as well as
 * encoding for some asymmetric operations such as RSA
 * *Some values are not required to be supported at this time but defined for future use*
 */
typedef enum {
    CRYPT_PADDING_PKCS7 = 0,     ///< PKCS5/PKCS7 padding. **Required**
    CRYPT_PADDING_ONE_AND_ZEROS, ///< ISO/IEC 7816-4 padding.
    CRYPT_PADDING_ZEROS_AND_LEN, ///< ANSI X.923 padding.
    CRYPT_PADDING_ZEROS,         ///< zero padding (not reversible).
    CRYPT_PADDING_NONE,          ///< never pad (full blocks only).
    CRYPT_RSA_PKCS_V15,          ///< Use PKCS-1 v1.5 encoding for RSA. **Required**
    CRYPT_RSA_PKCS_V21,          ///< Use PKCS-1 v2.1 encoding for RSA. **Required**
} crypt_padding_t;

/**
 * @brief Enumeration used to specify the mode of cryptographic hash/HMAC operation
 *
 * *Some values are not required to be supported at this time but defined for future use*
 */
typedef enum {
	CRYPT_HASH_MODE_NONE = 0,    ///< hash mode N/A.
	CRYPT_HASH_MODE_MD5 = 1,     ///< hash data with MD5.
	CRYPT_HASH_MODE_SHA1,        ///< hash data with SHA1. **Required**
	CRYPT_HASH_MODE_SHA224,      ///< hash data with SHA224.
	CRYPT_HASH_MODE_SHA256,      ///< hash data with SHA256. **Required**
	CRYPT_HASH_MODE_SHA384,      ///< hash data with SHA384. **Required**
	CRYPT_HASH_MODE_SHA512       ///< hash data with SHA512.
} crypt_hash_mode_t;

/**
 * @brief Enumeration of supported asymmetric crypto methods
 */
typedef enum {
    CRYPT_METHOD_RSA = 1,         ///< RSA. **Required**
	CRYPT_METHOD_ECDSA = 2,       ///< ECDSA. **Required**
} crypt_crypt_mode_t;

#define CRYPT_UUID_LEN 36UL

typedef void *CRYPT_HASH_CTX;
typedef void *CRYPT_CIPHER_CTX;

/**
 * @internal
 * Function type definition for mutual exclusion lock callback.
 */
typedef int32_t (*crypt_lock_mutex_t) (void *mutex);

/**
 * @internal
 * Function type definition for mutual exclusion unlock callback.
 */
typedef int32_t (*crypt_unlock_mutex_t) (void *mutex);

#include "spil_crypt.h"

#ifdef __cplusplus
}
#endif
#endif /* CRYPT_MGR_IF_CRYPT_MGR_H */
