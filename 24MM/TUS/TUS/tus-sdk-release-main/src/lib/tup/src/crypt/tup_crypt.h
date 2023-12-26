/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */
#ifndef TUP_CRYPT_H
#define TUP_CRYPT_H

/* system header */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* user header */

/* object macro */
#define TUP_MD_MAX_SIZE (64)
#define TUP_HMAC_SHA256_128 (0x184)
#define TUP_ECDSA_NIST384P (0x1060)

/* function macro */

/* typedef definition */

/* enum definition */

/* struct/union definition */

/* extern variable */

/* function prototype */
bool tup_check_icv_algorithm(uint32_t algorithm);
int tup_get_icv_size(uint32_t algorithm);
bool tup_check_icv_key(uint64_t keyinfo);
int tup_get_icv_key_size(uint64_t keyinfo, size_t *psize);
int tup_get_icv_key(uint64_t keyinfo, uint8_t *pkey);
bool tup_check_rooticv_key(const uint8_t keyinfo[64]);
int tup_get_rooticv_key_size(const uint8_t keyinfo[64], size_t *psize);
int tup_get_rooticv_key(const uint8_t keyinfo[64], uint8_t *pkey);
bool tup_check_sign_algorithm(uint32_t algorithm);
int tup_get_sign_len(uint32_t algorithm, size_t *psize);
int tup_verify_signature(uint32_t algorithm, uint32_t blocksize,
  const uint8_t keyinfo[64], const uint8_t *data, size_t data_len,
  const uint8_t *signature, size_t sign_len);
int tup_verify_ecdsa(const uint8_t keyinfo[64], const uint8_t *data,
  size_t data_len, const uint8_t *signature, size_t sign_len);
int tup_generate_icv(uint32_t, const uint8_t *, size_t, const uint8_t *, size_t,
  uint8_t *, size_t *);
int tup_generate_hmac(uint32_t algorithm, const uint8_t *key, size_t key_len,
  const uint8_t *data, size_t data_len, uint8_t *icv, size_t *icv_len);
int tup_constant_time_memcmp(const void *, const void *, size_t);

#endif /* !TUP_CRYPT_H */
