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

/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

/**
 * @file spil_crypt.h
 *
 * This file defines crypto primitive functions Security Manager must implement.
 * Implementation of these functions depends on the specific platform where OTAmatic client
 * is targeting to run on.
 */

#ifndef SPIL_CRYPT_H_
#define SPIL_CRYPT_H_

/**
 * @public
 * @brief Initialize Security Manager.
 *
 * This function must be called before all other functions.
 * Return value of EXIT_SUCCESS means that CRYPT is ready for operation.
 *
 * @retval EXIT_SUCCESS or error code
 */
#ifndef CRYPT_LOAD_ROOTCA_FROM_FILE
err_t crypt_init(cstr_t root_ca, size_t root_ca_len);
#else
err_t crypt_init(void);
#endif

/**
 * @public
 * @brief De-initialize Security Manager and cleanup.
 *
 * This function should be called to cleanup after done with Security Manager. All resources
 * allocated will be freed. It is the caller's responsibility to make sure no further CRYPT
 * function gets called until next crypt_init()
 *
 * @retval None
 */
void crypt_deinit(void);

/**
 * @brief Generate random numbers.
 *
 * This function fills the *rng* buffer with *num* bytes of random data. Data in the buffer is only
 * guaranteed to be valid when return value is EXIT_SUCCESS.
 * Caller must make sure the buffer is big enough to contain all the data.
 *
 * @public
 * @param [out] rng buffer for the random number
 * @param [in] num number of bytes of random number to be generated
 * @retval EXIT_SUCCESS or error code
 */
err_t crypt_random_number(uint8_t *rng, size_t num);

/**
 * @brief Re-seed RNG for random data generation.
 *
 * This functions stirs the random data entropy pool.
 * Optional extra data can be provided by caller to add to entropy pool.
 * And this function may choose to persist some entropy seeds in platform dependent way, in case
 * entropy pool is not random enough during system reboot time.
 *
 * @public
 * @param [in] extra_entropy random data to stir the entropy pool. NULL if no extra data provided.
 * @param [in] entropy_count number of bytes in the extra_entropy buffer
 * @retval None
 */
void crypt_reseed_rng(const uint8_t *extra_entropy, size_t entropy_count);

/**
 * @brief Generate UUID.
 *
 * @public
 * Caller must make sure the buffer is big enough to contain a string CRYPT_UUID_LEN long,
 * plus the ending NUL.
 *
 * @param [out] uuid buffer for the UUID string
 * @param [in] buf_size size of the buffer
 * @retval EXIT_SUCCESS or error code
 */
err_t crypt_generate_uuid(char *uuid, size_t buf_size);

/**
 * @brief Initialize cipher operation.
 *
 * @public
 * This function starts a symmetric cipher operation, to encrypt or decrypt data.
 * This function is called with the key data: *((NULL != key) && (0 != key_len))*
 *
 * Optionally this function can support named key. In that case, the function will be called
 * with *((NULL != key) && (0 == key_len))*, and *key* is a NUL-terminated string for the key name.
 * The named key feature is not used by OTAmatic client at this time.
 *
 * @param [out] ctx Pointer to get the handle of initialized cipher context
 * @param [in] key used to do cipher operation
 * @param [in] key_len length of the key data.
 * @param [in] action Action of the operation
 * @param [in] alg Algorithm of the operation
 * @param [in] mode Mode of the operation
 * @param [in] padding Padding of the operation
 * @param [in] iv Initial vector
 * @retval EXIT_SUCCESS or error code
 */
err_t crypt_cipher_start(CRYPT_CIPHER_CTX *ctx, const uint8_t *key, size_t key_len,
        crypt_cipher_action_t action,
		crypt_cipher_algorithm_t alg,
		crypt_cipher_mode_t mode,
		crypt_padding_t padding,
		const uint8_t *iv);

/**
 * @brief Do cipher operation on a block of data.
 *
 * Encrypt/decrypt the data passed in and put result data (if any) in the output buffer.
 *
 * @public
 * @param [in] ctx Cipher context handled from crypt_cipher_start()
 * @param [in] input Input data to be encrypted/decrypted
 * @param [in] input_len Size of the input data
 * @param [in] output Buffer for the output data
 * @param [in] output_buf_len Size of the output buffer
 * @param [out] result_len Size of the output data
 * @retval EXIT_SUCCESS or error code
 */
err_t crypt_cipher_update(CRYPT_CIPHER_CTX ctx, const uint8_t *input, size_t input_len,
		uint8_t *output, size_t output_buf_len, size_t *result_len);

/**
 * @brief Finalize cipher operation.
 *
 * Finalize encryption/decryption and write the rest of result data (if any) into output buffer.
 * This function also cleans up the resource allocated for this cipher context.
 *
 * @public
 * @param [in] ctx Cipher context handled from crypt_cipher_start()
 * @param [in] output Buffer for the output data
 * @param [in] output_buf_len Size of the output buffer
 * @param [out] result_len Size of the output data
 * @retval EXIT_SUCCESS or error code
 */
err_t crypt_cipher_finalize(CRYPT_CIPHER_CTX ctx,
		uint8_t *output, size_t output_buf_len, size_t *result_len);

/**
 * @brief Initialize hash operation.
 *
 * @public
 * @param [out] ctx Pointer to get the handle of initialized hash context
 * @param [in] mode Mode of the operation
 * @retval EXIT_SUCCESS or error code
 */
err_t crypt_hash_init(CRYPT_HASH_CTX *ctx, crypt_hash_mode_t mode);

/**
 * @brief Proceed hash operation.
 *
 * Hash a block of input data.
 *
 * @public
 * @param [in] ctx hash context handle from crypt_hash_init()
 * @param [in] input Input data to be hashed
 * @param [in] input_len Size of the input data
 * @retval EXIT_SUCCESS or error code
 */
err_t crypt_hash_update(CRYPT_HASH_CTX ctx, const uint8_t *input, size_t input_len);

/**
 * @brief Finalize hash operation.
 *
 * Hash the rest of input data (if any) and finalize the hash operation.
 * Result hash would be put in the *hash* buffer.
 * Resources allocated for the hash context would released.
 *
 * @public
 * @param [in] ctx hash context handle from crypt_hash_init()
 * @param [in] input Input data to be hashed. NULL indicates no more input data.
 * @param [in] input_len Size of the input data
 * @param [in] hash Buffer for the output hash
 * @param [in] hash_buf_len Size of the output buffer
 * @param [out] hash_len Size of the output hash
 * @retval EXIT_SUCCESS or error code
 */
err_t crypt_hash_finalize(CRYPT_HASH_CTX ctx, const uint8_t *input, size_t input_len,
		uint8_t *hash, size_t hash_buf_len, size_t *hash_len);

/**
 * @brief Initialize HMAC operation.
 *
 * @public
 * This function is called with the key data: *((NULL != key) && (0 != key_len))*
 *
 * Optionally this function can support named key. In that case, the function will be called
 * with *((NULL != key) && (0 == key_len))*, and *key* is a NUL-terminated string for the key name.
 * The named key feature is not used by OTAmatic client at this time.
 *
 * @param [out] ctx Pointer to get the handle of initialized hash context
 * @param [in] key used to do HMAC operation
 * @param [in] key_len length of the key data.
 * @param [in] mode Mode of the operation
 * @retval EXIT_SUCCESS or error code
 */
err_t crypt_hmac_init(CRYPT_HASH_CTX *ctx, const void *key, size_t key_len, crypt_hash_mode_t mode);

/**
 * @brief Proceed HMAC operation.
 *
 * Hash a block of input data.
 *
 * @public
 * @param [in] ctx hash context handle from crypt_hmac_init()
 * @param [in] input Input data to be hashed
 * @param [in] input_len Size of the input data
 * @retval EXIT_SUCCESS or error code
 */
err_t crypt_hmac_update(CRYPT_HASH_CTX ctx, const uint8_t *input, size_t input_len);

/**
 * @brief Finalize HMAC operation.
 *
 * Hash the rest of input data (if any) and finalize the hash operation.
 * Result hash would be put in the *hash* buffer.
 * Resources allocated for the hash context would released.
 *
 * @public
 * @param [in] ctx hash context handle from crypt_hmac_init()
 * @param [in] input Input data to be hashed. NULL indicates no more input data.
 * @param [in] input_len Size of the input data
 * @param [in] hash Buffer for the output hash
 * @param [in] hash_buf_len Size of the output buffer
 * @param [out] hash_len Size of the output hash
 * @retval EXIT_SUCCESS or error code
 */
err_t crypt_hmac_finalize(CRYPT_HASH_CTX ctx, const uint8_t *input, size_t input_len,
		uint8_t *hash, size_t hash_buf_len, size_t *hash_len);

#ifdef TRUST_CHAIN_VERIFY_CERT
/**
 * @internal
 * @brief Update CRL for trustchain verification.
 *
 * @param [in] crl NUL-terminated CRL string
 * @retval EXIT_SUCCESS or error code
 */
err_t crypt_update_crl(const char *crl);

/**
 * @internal
 * @brief Verify certificate.
 *
 * Verify certificate against:
 * 		Trust anchors:
 *	 		Root certificate built into the Security Manager
 *	 		Certificates appended into the trustchain for this session
 *	 	CRL for this session
 * @param [in] cert NUL-terminated certificate string
 * @retval EXIT_SUCCESS or error code
 */
err_t crypt_verify_cert(const char *cert);

/**
 * @internal
 * @brief Verify certificate and append it to the trustchain for further use.
 *
 * @param [in] cert NUL-terminated certificate string
 * @retval EXIT_SUCCESS or error code
 */
err_t crypt_cert_append_trustchain(const char *cert);

/**
 * @internal
 * @brief Verify signature.
 *
 * Verify signature with PKI in built-in trust chain, as well as the CRL and certificates added to
 * trust chain by crypt_update_crl()/crypt_cert_append_trustchain()
 *
 * @param [in] cert NUL-terminated certificate string
 * @param [in] digest Digest of the data to be verified, binary format
 * @param [in] digest_len Size of the digest
 * @param [in] signature Signature of the digest to verify against. Base64 format NUL-terminated.
 * @retval EXIT_SUCCESS or error code
 */
err_t crypt_verify_signature(const char *cert,
		const uint8_t *digest, size_t digest_len,
		const char *signature);
#endif

#ifdef EXTRACT_CERT
/**
 * @brief Extract public key in DER format from certificate PEM
 * @public
 * @param [in] pem_cert NUL-terminated certificate string
 * @param [in] der_pubkey buffer to store public key
 * @param [in] buflen output buffer size
 * @param [out] derlen Size of the output DER data
 * @retval EXIT_SUCCESS or error code
 */
err_t crypt_extract_pubkey_from_cert(const char *pem_cert, uint8_t *der_pubkey, size_t buflen,
		size_t *derlen);
#endif

/**
 * @brief Verify signature with public key.
 *
 * Verify signature of the data against public key.
 * This function calculates digest of the data before verifying the signature.
 * Key/data/signature are all decoded bytes in binary format.
 *
 * @public
 * @param [in] hashmode method used to hash the data before calculate signature
 * @param [in] method used for the signature
 * @param [in] pubkey buffer for the public key
 * @param [in] pubkey_len size of the public key
 * @param [in] data buffer for the data
 * @param [in] datalen size of the data
 * @param [in] signature buffer for the signature
 * @param [in] sig_len size of the signature
 * @retval EXIT_SUCCESS or error code
 */
err_t crypt_pk_verify_signature(crypt_hash_mode_t hashmode, crypt_crypt_mode_t method,
        const uint8_t *pubkey, size_t pubkey_len, const uint8_t *data, size_t datalen,
        const uint8_t *signature, size_t sig_len);

/**
 * @brief Get trusted time
 *
 * This function returns trusted time for the OTAmatic client to use.
 *   - The trusted time should never rollback. This means for consecutive calls of this function,
 *     the later call will always return a value **no less** than the previous call.
 *      + this must be enforced across power cycle of the client device.
 *   - The trusted time should be reasonably close to real time clock.
 *     + It needs to be within the period when the certificates and metadata files from OTAmatic
 *       backend can be validated.
 *     + It also needs to allow client to generate valid app token for HTTP requests, so the client
 *       won't get out of sync with the server, if the app-token is used.
 *     + Any inaccuracy in trusted time would be reflected in the notification timestamps uploaded
 *       to OTAmatic SDP.
 *
 * If above requirements cannot be satisfied, this function should return an error code to
 * indicate trusted time not valid.
 *
 * @public
 * @param [out] time buffer to store time, milliseconds since 01/01/1970 12:00:00AM
 * @retval EXIT_SUCCESS or error code
 */
err_t crypt_get_time(uint64_t *time);

/**
 * @internal
 * Register a callback function used to lock a mutex semaphore object.
 * @param lock_mutex_cb The callback function to be used to lock a mutex semaphore
 * @retval EXIT_SUCCESS or error code
 */
err_t crypt_reg_lock_mutex(crypt_lock_mutex_t lock_mutex_cb);

/**
 * @internal
 * Register a callback function used to unlock a mutex semaphore object.
 * @param unlock_mutex_cb The callback function to be used to unlock a mutex semaphore
 * @retval EXIT_SUCCESS or error code
 */
err_t crypt_reg_unlock_mutex(crypt_unlock_mutex_t unlock_mutex_cb);

/**
 * @internal
 * System mutex semaphore that will be utilized to protect data structures
 * internal to the Download manager.  This mutex will be locked/unlocked via
 * the callback functions registered via dm_reg_lock_mutex() and dm_reg_unlock_mutex.
 *
 * If both neither of callbacks are registered and the mutex is not registered, it
 * is assumed that the underlying implementation insures that the DM API is only called
 * within the context of a single thread.
 * @param mutex A pointer to the system mutex semaphore which will be locked/unlocked via
 *              the registered callback functions.
 * @retval EXIT_SUCCESS or error code
 */
err_t crypt_reg_mutex(void *mutex);

#endif
