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
 *  Copyright (c) 2021 Airbiquity Inc.  All rights reserved.
 *
 * ****************************************************************************
 */

/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "tiny-json.h"
#include "emoota_utility.h"
#include "itsy_bits.h"
#include "codec.h"
#include "utf8_enc.h"
#include "hex_enc.h"
#include "string_db.h"

#include <stdio.h>
#ifdef DEBUG_BUILD
#define DEBUG_PRINT(...)  ((void)printf(__VA_ARGS__))    /* parasoft-suppress MISRAC2012-DIR_4_9-a-4 CERT_C-PRE00-a-3 "using macro instead of function in order to comply with customer requirements" */
#else
#define DEBUG_PRINT(...)                        /* parasoft-suppress MISRAC2012-DIR_4_9-a-4  CERT_C-PRE00-a-3  "using macro instead of function in order to comply with customer requirements" */
#endif

#ifdef __cplusplus
}
#endif

/* parasoft-begin-suppress MISRAC2012-RULE_15_1-a-4  MISRAC2012-RULE_15_4-a-4 "ABQ validated that all uses of 'goto' meet the requirements in order to allow the use of 'goto'" */
/* parasoft-begin-suppress CERT_C-MSC41-a-1 "This string does not contain sensitive information."  */
/* parasoft-begin-suppress CERT_C-DCL06-a-3 "Keeping hard coded values in favor of code readability" */
/* parasoft-begin-suppress CERT_C-DCL02-a-3 "Keeping camelCase formatting in favor of code readability" */
/* parasoft-begin-suppress CERT_C-DCL19-a-3 "ABQ code review concluded that variables are sufficiently localized" */


static inline uint64_t MIN(const uint64_t a, const uint64_t b) {
    return ((a) < (b)) ? (a) : (b);
}

static size_t strnlength(const byte_t *const s, const size_t n) {
    size_t len = UINT_ZERO;
    const byte_t *src = s;
    if (NULL != src) {
        while (len < n) {
            if ((int8_t)*src == (int8_t) '\0') {
                break;
            }
            src = &src[1];
            len++;
        }
    }
    return len;
}

static cstr_t strncopy(str_t const dest_in, cstr_t const src_in, const size_t n_in) {

    str_t dest = dest_in;
    cstr_t src = src_in;
    size_t n = n_in;
    str_t rval = NULL;
    if ((dest == NULL) || (src == NULL) || (n == UINT_ZERO)) {
        rval = NULL;
    } else {
        rval = dest;
        while ((*src != '\0') && (n > UINT_ZERO)) {
            *dest = *src;
            dest = &dest[1];
            src = &src[1];
            n--;
        }
        if(n > UINT_ZERO){
            *dest = '\0';
        }
    }
    return rval;
}

#define LAST_SET_STR_VAL_PTR        out_str_val_ptr

/* parasoft-begin-suppress MISRAC2012-DIR_4_9-a-4 CERT_C-PRE00-a-3 "Function like macros are used in order to maintain code readability" */
#define ADD_JSON_PAIR_HEX(encoder, key, value, val_sz)    if(UTIL_NO_ERROR == rval) { if(EXIT_SUCCESS!=encode_json_pair_hex((encoder), (key), (value), (val_sz))) { rval = UTIL_ERR_NO_RESOURCE;}}
#define ADD_JSON_PAIR_STR(encoder, key, value, val_sz)    if(UTIL_NO_ERROR == rval) { if(EXIT_SUCCESS!=encode_json_pair_str((encoder), (key), (value), (val_sz))) { rval = UTIL_ERR_NO_RESOURCE;}}
#define ADD_JSON_PAIR_UINT(encoder, key, value)    if(UTIL_NO_ERROR == rval) { if(EXIT_SUCCESS!=encode_json_pair_unumber((encoder), (key), (value))) { rval = UTIL_ERR_NO_RESOURCE;}}
#define ADD_JSON_PAIR_SINT(encoder, key, value)    if(UTIL_NO_ERROR == rval) { if(EXIT_SUCCESS!=encode_json_pair_number((encoder), (key), (value))) { rval = UTIL_ERR_NO_RESOURCE;}}
#define ADD_JSON_PAIR_BOOL(encoder, key, value)    if(UTIL_NO_ERROR == rval) { if(EXIT_SUCCESS!=encode_json_pair_bool((encoder), (key), (value))) { rval = UTIL_ERR_NO_RESOURCE;}}
#define ADD_JSON_RAW(encoder, value)  if(UTIL_NO_ERROR == rval) { if(EXIT_SUCCESS!=abq_raw_codec.encode((encoder), (value))) { rval = UTIL_ERR_NO_RESOURCE;}}
#define ADD_JSON_KEY(encoder, key)    if(UTIL_NO_ERROR == rval) { if(EXIT_SUCCESS!=encode_json_str((encoder), (key), SINT_MINUS_ONE)) { rval = UTIL_ERR_NO_RESOURCE;}}
#define ADD_JSON_HEX(encoder, value, val_sz)    if(UTIL_NO_ERROR == rval) { if(EXIT_SUCCESS!=encode_json_hex((encoder), (value), (val_sz))){ rval = UTIL_ERR_NO_RESOURCE;}}
#define ADD_JSON_INT(encoder, value) if(UTIL_NO_ERROR == rval) { if(EXIT_SUCCESS!=abq2_encode_uint((encoder), (value), DECIMAL_RADIX)) { rval = UTIL_ERR_NO_RESOURCE;}}
#define ADD_JSON_STR(encoder, value, val_sz) if(UTIL_NO_ERROR == rval) { if(EXIT_SUCCESS!=encode_json_str((encoder), (value), (val_sz))) { rval = UTIL_ERR_NO_RESOURCE;}}

#define ERROR_ON_NULL(subs)  \
if (NULL == (subs)) {DEBUG_PRINT("ERR: Failed to find expected JSON object \n");rval = UTIL_ERR_INVALID_PARAMETER;goto error;}

#define CLEAR_OUTPUT_STRUCTURE(structure, size) (abq_bytes_set((structure), '\0', (size)))

/**
 * Macro that performs a lookup for a given key (db_key).
 * If key is found it will type check for JSON_TEXT
 * If type is correct, the length is checked against 'max'
 * If length is within allowed bounds, the value will be copied to 'out'
 *
 * If icd policy is set to mandatory and the requested key is not found,
 * the macro will jump to the error label
 */
#define COPY_TEXT_VAL_FOR_MNDTRY_KEY(property, db_key, out, max)  \
obj = json_getProperty((property), getDBstr((db_key)));\
if (obj != NULL) {if(json_getType(obj) != JSON_TEXT) {  \
DEBUG_PRINT("ERR: Wrong value type for key: %s\n", getDBstr((db_key)));\
rval = UTIL_ERR_INVALID_PARAMETER; goto error;} else {\
if (strnlength(json_getValue(obj), (max)+UINT_ONE) > (max)) {                \
DEBUG_PRINT("ERR: Value for key: %s exceeds maximum of: %i \n", getDBstr((db_key)),(max));\
rval = UTIL_ERR_INVALID_PARAMETER; goto error; } \
else {(void) strncopy((out), json_getValue(obj), (max));}}} else {  \
DEBUG_PRINT("ERR: Missing key: %s\n", getDBstr((db_key)));\
rval = UTIL_ERR_INVALID_PARAMETER; goto error;  }

/**
 * Macro that performs a lookup for a given key (db_key).
 * If key is found it will type check for JSON_TEXT
 * If type is correct, the length is checked against 'max'
 * If length is within allowed bounds, the value will be copied to 'out'
 */
#define COPY_TEXT_VAL_FOR_OPTNL_KEY(property, db_key, out, max)  \
obj = json_getProperty((property), getDBstr((db_key)));\
if (obj != NULL) {if(json_getType(obj) != JSON_TEXT) {  \
DEBUG_PRINT("ERR: Wrong value type for key: %s\n", getDBstr((db_key)));\
rval = UTIL_ERR_INVALID_PARAMETER; goto error;} else {\
if (strnlength(json_getValue(obj), (max)+UINT_ONE) > (max)) {                \
DEBUG_PRINT("ERR: Value for key: %s exceeds maximum of: %i \n", getDBstr((db_key)),(max));\
rval = UTIL_ERR_INVALID_PARAMETER; goto error; } \
else {(void) strncopy((out), json_getValue(obj), (max));}}}

/**
 * Macro that performs a lookup for a given key (db_key).
 * If key is found it will type check for JSON_INTEGER
 * If type is correct, the value will be copied to 'out'
 *
 * If icd policy is set to mandatory and the requested key is not found,
 * the macro will jump to the error label
 */
#define COPY_UINT_VAL_FOR_MNDTRY_KEY(property, db_key, out, type, max)                                 \
    obj = json_getProperty((property), getDBstr((db_key)));                                            \
    if (obj != NULL) {                                                                                 \
        if (json_getType(obj) != JSON_INTEGER) {                                                       \
            DEBUG_PRINT("ERR: Wrong value type for key: %s\n", getDBstr((db_key)));                    \
            rval = UTIL_ERR_INVALID_PARAMETER;                                                         \
            goto error;                                                                                \
        } else {                                                                                       \
            const int64_t value = json_getInteger(obj);                                                \
            if ((value < SINT_ZERO) || ((uint64_t) value > (uint64_t)(max))) {                         \
                DEBUG_PRINT("ERR: Input value exceeds target storage size: %s\n", getDBstr((db_key))); \
                rval = UTIL_ERR_INVALID_PARAMETER;                                                     \
                goto error;                                                                            \
            } else {                                                                                   \
                (out) = (type) value;                                                                  \
            }                                                                                          \
        }                                                                                              \
    } else {                                                                                           \
        DEBUG_PRINT("ERR: Missing key: %s\n", getDBstr((db_key)));                                     \
        rval = UTIL_ERR_INVALID_PARAMETER;                                                             \
        goto error;                                                                                    \
    }

/**
 * Macro that performs a lookup for a given key (db_key).
 * If key is found it will type check for JSON_INTEGER
 * If type is correct, the value will be copied to 'out'
 */
#define COPY_UINT_VAL_FOR_OPTNL_KEY(property, db_key, out, type, max)                                  \
    obj = json_getProperty((property), getDBstr((db_key)));                                            \
    if (obj != NULL) {                                                                                 \
        if (json_getType(obj) != JSON_INTEGER) {                                                       \
            DEBUG_PRINT("ERR: Wrong value type for key: %s\n", getDBstr((db_key)));                    \
            rval = UTIL_ERR_INVALID_PARAMETER;                                                         \
            goto error;                                                                                \
        } else {                                                                                       \
            const int64_t value = json_getInteger(obj);                                                \
            if ((value < SINT_ZERO) || ((uint64_t) value > (max))) {                                   \
                DEBUG_PRINT("ERR: Input value exceeds target storage size: %s\n", getDBstr((db_key))); \
                rval = UTIL_ERR_INVALID_PARAMETER;                                                     \
                goto error;                                                                            \
            } else {                                                                                   \
                (out) = (type) value;                                                                  \
            }                                                                                          \
        }                                                                                              \
    }

/**
 * Macro that performs a lookup for a given key (db_key).
 * If key is found it will type check for JSON_INTEGER
 * If type is correct, the value will be copied to 'out'
 */
#define COPY_SINT_VAL_FOR_OPTNL_KEY(property, db_key, out, type, max, min)  \
obj = json_getProperty((property), getDBstr((db_key)));  \
if (obj != NULL) {if(json_getType(obj) != JSON_INTEGER) {    \
DEBUG_PRINT("ERR: Wrong value type for key: %s\n", getDBstr((db_key)));   \
rval = UTIL_ERR_INVALID_PARAMETER; goto error; } \
else { const int64_t value = json_getInteger(obj);  if((value > (max))||(value < (min))){        \
DEBUG_PRINT("ERR: Input value exceeds target storage size: %s\n", getDBstr((db_key)));      \
rval = UTIL_ERR_INVALID_PARAMETER; goto error;      \
} else { (out) = (type)value;} }}


/**
 * Macro that performs a lookup for a given key (db_key).
 * If key is found it will type check for JSON_BOOLEAN
 * If type is correct, the value will be copied to 'out'
 *
 * If icd policy is set to mandatory and the requested key is not found,
 * the macro will jump to the error label
 */
#define COPY_BOOL_VAL_FOR_MNDTRY_KEY(property, db_key, out)  \
obj = json_getProperty((property), getDBstr((db_key)));  \
if (obj != NULL) {if(json_getType(obj) != JSON_BOOLEAN) {    \
DEBUG_PRINT("ERR: Wrong value type for key: %s\n", getDBstr((db_key)));   \
rval = UTIL_ERR_INVALID_PARAMETER; goto error; } \
else { (out) = json_getBoolean(obj); }} else {  \
DEBUG_PRINT("ERR: Missing key: %s\n", getDBstr((db_key)));\
rval = UTIL_ERR_INVALID_PARAMETER; goto error; }


/**
 * Macro that performs a lookup for a given key (db_key).
 * If key is found it will type check for JSON_TEXT
 * If type is correct, a cstr_t 'out' pointer is created to access the value
 *
 * If icd policy is set to mandatory and the requested key is not found,
 * the macro will jump to the error label
 */
#define GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(property, db_key)  \
 obj = json_getProperty((property), getDBstr(db_key));   \
if ((obj) != NULL) {if(json_getType(obj) != JSON_TEXT) {   \
DEBUG_PRINT("ERR: Wrong value type for key: %s\n", getDBstr((db_key)));  \
rval = UTIL_ERR_INVALID_PARAMETER; goto error;} out_str_val_ptr = json_getValue(obj); } else { \
out_str_val_ptr = NULL; \
DEBUG_PRINT("ERR: Missing key: %s\n", getDBstr((db_key)));\
rval = UTIL_ERR_INVALID_PARAMETER; goto error; }


/**
 * Macro that performs a lookup for a given key (db_key).
 * If key is found it will type check for JSON_TEXT
 * If type is correct, a cstr_t 'out' pointer is created to access the value
 */
#define GET_TEXT_VAL_PTR_FOR_OPTNL_KEY(property, db_key)  \
 obj = json_getProperty((property), getDBstr(db_key));   \
if ((obj) != NULL) {if(json_getType(obj) != JSON_TEXT) {   \
DEBUG_PRINT("ERR: Wrong value type for key: %s\n", getDBstr((db_key)));  \
rval = UTIL_ERR_INVALID_PARAMETER; goto error;} out_str_val_ptr = json_getValue(obj); } \
else { out_str_val_ptr = NULL; }

/**
 * Macro that performs a type check for JSON_TEXT
 * If type is correct, a cstr_t 'out' pointer is created to access the value
 */
#define GET_TEXT_VALUE_PTR_FOR_OBJ(element) \
if ((element) != NULL) {if(json_getType((element)) != JSON_TEXT) {   \
DEBUG_PRINT("ERR: Wrong value type for object\n");  \
rval = UTIL_ERR_INVALID_PARAMETER; goto error;}                \
out_str_val_ptr = json_getValue(element); } \
else { out_str_val_ptr = NULL; }

/* parasoft-end-suppress MISRAC2012-DIR_4_9-a-4 CERT_C-PRE00-a-3 */

/* parasoft-end-suppress MISRAC2012-DIR_4_9-a-4 */

#define PREFIX_SIGNED_BODY "\"signed\":"
#define PREFIX_SIGNATURES_BODY "\"signatures\":"


static int64_t memcomp(cstr_t const s1_in, cstr_t const s2_in, const size_t n) {
    cstr_t s1 = s1_in;
    cstr_t s2 = s2_in;
    int64_t rval = SINT_ZERO;
    if ((s1 != NULL) && (s2 != NULL) && (n > UINT_ZERO)) {
        for (size_t i = UINT_ZERO; i < n; i++) {
            const uint8_t u1 = (const uint8_t) (*s1);
            const uint8_t u2 = (const uint8_t) (*s2);
            if (u1 != u2) {
                rval = ((int64_t) u1 - (int64_t) u2);
                break;
            }
            s1 = &s1[1];
            s2 = &s2[1];
        }
    } else {
        rval = SINT_MINUS_ONE;
    }
    return rval;
}

/**
 * Input is decoded as UTF8 and encoded as specified by the encoder.
 * @param encoder the encoder to use
 * @param utf8 input data
 * @param length maximum input length to encode. Use -1 to encode to first encountered null-terminator
 * @return EXIT_SUCCESS on success. Error code on error
 */
static err_t encode_json_str(abq_encoder_t *const encoder, cstr_t const utf8, int32_t const length) {
    err_t retval = CHECK_NULL(encoder);
    if ((EXIT_SUCCESS != retval) || (NULL == utf8)) {
        retval = EXIT_FAILURE;
    } else {
        retval = abq_raw_codec.encode(encoder, '\"');
        if (EXIT_SUCCESS == retval) {
            if(getUTF8Codec == encoder->codec) {
                const size_t orig_max = encoder->max;
                retval = abq2_encode_text(encoder, &abq_utf8_codec, utf8, length);
                if (EXIT_SUCCESS != retval) {
                    // Return error as is
                } else if (encoder->max != orig_max) {
                    // JSON string has been terminated
                    encoder->max = orig_max;
                } else {
                    retval = abq_raw_codec.encode(encoder, '\"');
                }
            } else {
                retval = EXIT_FAILURE;
            }
        }
    }
    return retval;
}

/**
 * Add a JSON Key-Value pair to the encoder buffer with the value being a JSON string
 * @param encoder the encoder to use
 * @param key string to encode as the 'key' part of a json Key-Value pair
 * @param value string to encode as the 'value' part of a json Key-Value pair
 * @param val_sz size of the value to encode. Use -1 to encode to first encountered null-terminator
 * @return EXIT_SUCCESS on success. Error code on error
 */
static err_t encode_json_pair_str(abq_encoder_t *const encoder, cstr_t const key, cstr_t const value, const int32_t val_sz) {
    err_t retval = CHECK_NULL(encoder);
    if ((EXIT_SUCCESS != retval) || (NULL == key) || (NULL == value)) {
        retval = EXIT_FAILURE;
    } else {
        retval = encode_json_str(encoder, key, SINT_MINUS_ONE);   /* always encode full key length */
        if (EXIT_SUCCESS == retval) {
            retval = abq_raw_codec.encode(encoder, ':');
            if (EXIT_SUCCESS == retval) {
                retval = encode_json_str(encoder, value, val_sz); /* limit byte to encode to 'length' */
            }
        }
    }
    return retval;
}

/**
 *  Add a JSON Key-Value pair to the encoder buffer with the value being a JSON number (signed)
 * @param encoder the encoder to use
 * @param key string to encode as the 'key' part of a json Key-Value pair
 * @param value string to encode as the 'value' part of a json Key-Value pair
 * @return EXIT_SUCCESS on success. Error code on error
 */
static err_t encode_json_pair_number(abq_encoder_t *const encoder, cstr_t const key, const number_t value) {
    err_t retval = CHECK_NULL(encoder);
    if ((EXIT_SUCCESS != retval) || (NULL == key)) {
        retval = EXIT_FAILURE;
    } else {
        retval = encode_json_str(encoder, key, SINT_MINUS_ONE);
        if (EXIT_SUCCESS == retval) {
            retval = abq_raw_codec.encode(encoder, ':');
            if (EXIT_SUCCESS == retval) {
                retval = abq2_encode_number(encoder, value);
            }
        }
    }
    return retval;
}

/**
 * Add a JSON Key-Value pair to the encoder buffer with the value being a JSON long (unsigned)
 * @param encoder the encoder to use
 * @param key string to encode as the 'key' part of a json Key-Value pair
 * @param value value to encode as the 'value' part of a json Key-Value pair
 * @return EXIT_SUCCESS on success. Error code on error
 */
static err_t encode_json_pair_unumber(abq_encoder_t *const encoder, cstr_t const key, const uint64_t value) {
    err_t retval = CHECK_NULL(encoder);
    if ((EXIT_SUCCESS != retval) || (NULL == key)) {
        retval = EXIT_FAILURE;
    } else {
        retval = encode_json_str(encoder, key, SINT_MINUS_ONE);
        if (EXIT_SUCCESS == retval) {
            retval = abq_raw_codec.encode(encoder, ':');
            if (EXIT_SUCCESS == retval) {
                retval = abq2_encode_uint(encoder, value, DECIMAL_RADIX);
            }
        }
    }
    return retval;
}

/**
 * Add a JSON Key-Value pair to the encoder buffer with the value being a JSON long (unsigned)
 * @param encoder the encoder to use
 * @param key string to encode as the 'key' part of a json Key-Value pair
 * @param value value to encode as the 'value' part of a json Key-Value pair
 * @return EXIT_SUCCESS on success. Error code on error
 */
static err_t encode_json_pair_bool(abq_encoder_t *const encoder, cstr_t const key, const bool_t value) {
    err_t retval = CHECK_NULL(encoder);
    if ((EXIT_SUCCESS != retval) || (NULL == key)) {
        retval = EXIT_FAILURE;
    } else {
        retval = encode_json_str(encoder, key, SINT_MINUS_ONE);
        if (EXIT_SUCCESS == retval) {
            retval = abq_raw_codec.encode(encoder, ':');
            if (EXIT_SUCCESS == retval) {
                if (value == true) {
                    retval = abq_raw_codec.encode(encoder, 't');
                    if (EXIT_SUCCESS == retval) {
                        retval = abq_raw_codec.encode(encoder, 'r');
                    }
                    if (EXIT_SUCCESS == retval) {
                        retval = abq_raw_codec.encode(encoder, 'u');
                    }
                    if (EXIT_SUCCESS == retval) {
                        retval = abq_raw_codec.encode(encoder, 'e');
                    }
                } else {
                    retval = abq_raw_codec.encode(encoder, 'f');
                    if (EXIT_SUCCESS == retval) {
                        retval = abq_raw_codec.encode(encoder, 'a');
                    }
                    if (EXIT_SUCCESS == retval) {
                        retval = abq_raw_codec.encode(encoder, 'l');
                    }
                    if (EXIT_SUCCESS == retval) {
                        retval = abq_raw_codec.encode(encoder, 's');
                    }
                    if (EXIT_SUCCESS == retval) {
                        retval = abq_raw_codec.encode(encoder, 'e');
                    }
                }
            }
        }
    }
    return retval;
}

/**
 * Add a JSON Key-Value pair to the encoder buffer with the value being a
 * hexadecimal converted number represented as a JSON string
 * @param encoder the encoder to use
 * @param key string to encode as the 'key' part of a json Key-Value pair
 * @param value value to encode as the 'value' part of a json Key-Value pair
 * @param val_sz size of the value to encode. Use -1 to encode to first encountered null-terminator
 * @return EXIT_SUCCESS on success. Error code on error
 */
static err_t encode_json_pair_hex(abq_encoder_t *const encoder, cstr_t const key, const uint8_t *const value, const size_t val_sz) {
    err_t retval = CHECK_NULL(encoder);
    if ((EXIT_SUCCESS != retval) || (NULL == key) || (NULL == value)) {
        retval = EXIT_FAILURE;
    } else {
        retval = encode_json_str(encoder, key, SINT_MINUS_ONE); /* always encode full key length */
        if (EXIT_SUCCESS == retval) {
            retval = abq_raw_codec.encode(encoder, ':');
            if (EXIT_SUCCESS == retval) {
                retval = abq_raw_codec.encode(encoder, '\"');
            }
            if (EXIT_SUCCESS == retval) {
                for (size_t i = UINT_ZERO; i < val_sz; ++i) {
                    retval = abq2_encode_hex(encoder, (byte_t) value[i]);
                    if (EXIT_SUCCESS != retval) {
                        break;
                    }
                }
            }
            retval = abq_raw_codec.encode(encoder, '\"');
        }
    }
    return retval;
}

static err_t encode_json_hex(abq_encoder_t *const encoder, const uint8_t *const value, const size_t val_sz) {
    err_t retval = CHECK_NULL(encoder);
    if ((EXIT_SUCCESS != retval) || (NULL == value)) {
        retval = EXIT_FAILURE;
    } else {
        retval = abq_raw_codec.encode(encoder, '\"');
        if (EXIT_SUCCESS == retval) {
            for (size_t i = UINT_ZERO; i < val_sz; ++i) {
                retval = abq2_encode_hex(encoder, (byte_t) value[i]);
                if (EXIT_SUCCESS != retval) {
                    break;
                }
            }
        }
        retval = abq_raw_codec.encode(encoder, '\"');
    }
    return retval;
}



static byte_t *GetHead(cbyte_t *const json_root_str, uint64_t *const length, cbyte_t *const prefix,
                       cbyte_t *const start_chr, cbyte_t *const end_chr) {
    size_t i;
    size_t count = 0;
    byte_t *head_atr = NULL;
    byte_t *head = NULL;
    byte_t *tail = NULL;
    bool_t error_flag = false;

    if ((json_root_str == NULL) || (length == NULL) || (prefix == NULL) || (start_chr == NULL) || (end_chr == NULL)) {
        goto error;
    }
#ifdef __cplusplus
    head_atr = strstr((str_t)json_root_str, (cstr_t)prefix);    /* parasoft-suppress MISRAC2012-RULE_11_8-a-2 CERT_C-EXP40-a-3 CERT_C-EXP32-a-2 "function does not alter pointer" */
#else
    head_atr = strstr((cstr_t)json_root_str, (cstr_t)prefix);
#endif
    if (NULL == head_atr) {
        DEBUG_PRINT("JSON string does not have specified string as prefix.\n");
        goto error;
    }

    head_atr = &head_atr[strlen(prefix)];
    head = strstr(head_atr, start_chr);
    if (NULL == head) {
        // return to avoid SEGFAULT in case of invalid input format
        goto error;
    }
    tail = head;
    for (i = 0; i <= strlen(head); i++) {
        if (*start_chr == *tail) {
            count++;
        } else if (*end_chr == *tail) {
            if (0 >= count) {
                error_flag = true;
                DEBUG_PRINT("Invalid parenthesis sequence\n");
                goto error;
            }
            count--;
        } else {
            //Nop
        }

        if (0 == count) {
            break;
        }
        tail = &tail[1];
    }
    if (0 != count) {
        error_flag = true;
        DEBUG_PRINT("The number of open/close bracket does not match with the number of close/open bracket.\n");
        goto error;
    }

    *length = i + 1;
    error:
        if(error_flag == true) {
            head = NULL;
        }
    return head;
}

static byte_t *
GetHeadBody(cbyte_t *const json_root_str, uint64_t *const length, cbyte_t *const prefix) {
    byte_t *rval = NULL;
    if ((json_root_str != NULL) && (length != NULL) && (prefix != NULL)) {
        rval = GetHead(json_root_str, length, prefix, "{", "}");
    }
    return rval;
}

static byte_t *
GetHeadArray(cbyte_t *const json_root_str, uint64_t *const length, cbyte_t *const prefix) {
    byte_t *rval = NULL;
    if ((json_root_str != NULL) && (length != NULL) && (prefix != NULL)) {
        rval = GetHead(json_root_str, length, prefix, "[", "]");
    }
    return rval;
}

static byte_t *GetHeadSignedBody(cbyte_t *const json_root_str, uint64_t *const length) {
    byte_t *rval = NULL;
    if ((json_root_str != NULL) && (length != NULL)) {
        rval = GetHeadBody(json_root_str, length, PREFIX_SIGNED_BODY);
    }
    return rval;
}

static byte_t *GetHeadSignaturesBody(cbyte_t *const json_root_str, uint64_t *const length) {
    byte_t *rval = NULL;
    if ((json_root_str != NULL) && (length != NULL)) {
        rval = GetHeadArray(json_root_str, length, PREFIX_SIGNATURES_BODY);
    }
    return rval;
}

utilErr_t util_ParseDataEncryptionKey(str_t const str,
                                      const size_t length,
                                      util_json_t mem[],
                                      const uint16_t qty,
                                      dataEncryptionKey_t *const out) {
    utilErr_t rval = UTIL_NO_ERROR;
    jsonErr_t jsonRet = JSON_NO_ERROR;
    json_t const *jsonRoot = NULL;
    json_t const *obj = NULL;
    cstr_t out_str_val_ptr = NULL;


    // Check populated parameters
    if (NULL == str) {
        DEBUG_PRINT("ERR: Target string is null.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    if (UINT_TWO > length) {
        DEBUG_PRINT("ERR: Length of target string is 0 or is less than 0.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    if (NULL == mem) {
        DEBUG_PRINT("ERR: Array of json properties to allocate is null.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (UINT_ZERO >= qty) {
        DEBUG_PRINT("ERR: Number of elements on array of json properties to allocate is 0 or is less than 0.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    if (NULL == out) {
        DEBUG_PRINT("ERR: Pointer to the data struct for storing parsed data is null.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    CLEAR_OUTPUT_STRUCTURE(out, sizeof(dataEncryptionKey_t));
    jsonRet = json_create_ex(&jsonRoot, str, length, mem, qty);
    switch (jsonRet) {
        case JSON_ERR_INVALID_PARAMETER:
            DEBUG_PRINT("ERR: Target string is not valid.\n");
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
            break;
        case JSON_ERR_NO_RESOURCE:
            DEBUG_PRINT("ERR: Array of json properties to allocate is too short for this JSON string.\n");
            rval = UTIL_ERR_NO_RESOURCE;
            goto error;
            break;

        case JSON_NO_ERROR:
            //No additional operation is required for non-error case.
            break;

        default:
            /* MISRA __NOP comment */
            break;
    }

    if (JSON_OBJ != json_getType(jsonRoot)) {
        DEBUG_PRINT("ERR: DataEncryptionKey should have object.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    COPY_TEXT_VAL_FOR_MNDTRY_KEY(jsonRoot, icd_key_value, out->value, MAX_BASE64_ENCRYPTED_DEK_SZ)

    GET_TEXT_VAL_PTR_FOR_OPTNL_KEY(jsonRoot, icd_key_cipherMode)
    /* always evaluates to true if 'icd_mandatory' is used */
    if (NULL != LAST_SET_STR_VAL_PTR) {
        cstr_t const ecbString = "ECB";
        if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) ecbString, strlen(ecbString))) {
            out->cipherMode = CIPHER_MODE_ECB;
        } else {
            out->cipherMode = CIPHER_MODE_UNKNOWN;
        }
    }

    GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(jsonRoot, icd_key_padding)
    /* always evaluates to true if 'icd_mandatory' is used */
    if (NULL != LAST_SET_STR_VAL_PTR) {
        cstr_t const sha1PaddingString = "OAEPWithSHA1AndMGF1Padding";
        cstr_t const sha256PaddingString = "OAEPWithSHA256AndMGF1Padding";
        cstr_t const sha384PaddingString = "OAEPWithSHA384AndMGF1Padding";
        if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) sha1PaddingString, strlen(sha1PaddingString))) {
            out->padding = OAEP_SHA1_MGF1;
        } else if (SINT_ZERO ==
                   memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) sha256PaddingString, strlen(sha256PaddingString))) {
            out->padding = OAEP_SHA256_MGF1;
        } else if (SINT_ZERO ==
                   memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) sha384PaddingString, strlen(sha384PaddingString))) {
            out->padding = OAEP_SHA384_MGF1;
        } else {
            out->padding = PADDING_UNKNOWN;
            DEBUG_PRINT("ERR: Value mismatches expectation for: %s\n", getDBstr(icd_key_padding));
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
        }
    }

    COPY_TEXT_VAL_FOR_OPTNL_KEY(jsonRoot, icd_key_iv, out->iv, MAX_BASE64_IV_SZ)

error:
    if (UTIL_NO_ERROR != rval) {
        CLEAR_OUTPUT_STRUCTURE(out, sizeof(dataEncryptionKey_t));
    }
    return rval;
}


utilErr_t util_CreateGetOtaCmdBody(const getOtaCmd_t *const getOtaCmdInfo,
                                   str_t const buf, const size_t bufLen) {
    utilErr_t rval = UTIL_NO_ERROR;

    if ((NULL != getOtaCmdInfo) && (NULL != buf) && (UINT_ZERO < bufLen)) {
        ABQ_ENCODER(encoder, getUTF8Codec, buf, bufLen);
        ABQ_CLEAR_ENCODER(encoder);
        ADD_JSON_RAW(&encoder, '{')
        ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_primaryIdentifier), getOtaCmdInfo->primarySerialNum,
                          (int32_t) MAX_SERIAL_NUM_SZ)
        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_vehicleIdentifier), getOtaCmdInfo->vin, (int32_t) MAX_VIN_SZ)
        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_UINT(&encoder, getDBstr(icd_key_lastCompletedCommandId), getOtaCmdInfo->lastUid)
        ADD_JSON_RAW(&encoder, '}')

        if (rval != UTIL_NO_ERROR) {
            ABQ_CLEAR_ENCODER(encoder);
        }
    } else {
        rval = UTIL_ERR_INVALID_PARAMETER;
    }
    return rval;
}

utilErr_t util_FindBundleSections(cstr_t const jsonStr,
                                  const size_t strLen,
                                  metaInfo_t *const metaInfo) {

    utilErr_t rval = UTIL_NO_ERROR;
    if (NULL == jsonStr) {
        DEBUG_PRINT("ERR: Input string is null.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (UINT_TWO > strLen) {
        DEBUG_PRINT("ERR: Length of target string is 0 or is less than 0.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (NULL == metaInfo) {
        DEBUG_PRINT("ERR: Output structure pointer is null.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    metaInfo->headAugment = GetHeadBody(jsonStr, &metaInfo->AugmentLength,
                                        "augmentedMetadata");
    if (NULL == metaInfo->headAugment) {
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    metaInfo->headTimestamp = GetHeadBody(jsonStr, &metaInfo->TimestampLength,
                                          "timestampMetadata");
    if (NULL == metaInfo->headTimestamp) {
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    metaInfo->headSnapshot = GetHeadBody(jsonStr, &metaInfo->SnapshotLength,
                                         "snapshotMetadata");
    if (NULL == metaInfo->headSnapshot) {
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    metaInfo->headTargets = GetHeadArray(jsonStr, &metaInfo->TargetsLength,
                                         "targetsMetadata");
    if (NULL == metaInfo->headTargets) {
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

error:
    return rval;
}

utilErr_t util_CreateSignatureBlock(const sigInfo_t sigInfo[], const uint16_t numSig, str_t const buf,
                                    const size_t bufLen) {
    utilErr_t rval = UTIL_NO_ERROR;
    if ((NULL != sigInfo) && (NULL != buf) && (UINT_ZERO < bufLen) && (UINT_ZERO < numSig)) {
        ABQ_ENCODER(encoder, getUTF8Codec, buf, bufLen);
        ABQ_CLEAR_ENCODER(encoder);
        ADD_JSON_RAW(&encoder, '[')

        for (size_t i = SINT_ZERO; i < MIN(MAX_KEY_NUM_PER_ROLE, (uint64_t) numSig); ++i) {

            if(sigInfo[i].value_len > MAX_SIG_LENGTH) {
                DEBUG_PRINT("ERR: Value length exceeds max defined\n");
                rval = UTIL_ERR_INVALID_PARAMETER;
                goto error;
            }

            if (i > UINT_ZERO) {
                ADD_JSON_RAW(&encoder, ',')
            }

            ADD_JSON_RAW(&encoder, '{')
            ADD_JSON_PAIR_HEX(&encoder, getDBstr(icd_key_keyid), sigInfo[i].keyid, SHA256_LENGTH)

            switch (sigInfo[i].methType) {
                case MT_SHA256withRSA:
                    ADD_JSON_RAW(&encoder, ',')
                    ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_method), getDBstr(icd_key_SHA256withRSA), SINT_MINUS_ONE)
                    break;
                case MT_SHA256withECDSA:
                    ADD_JSON_RAW(&encoder, ',')
                    ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_method), getDBstr(icd_key_SHA256withECDSA),
                                      SINT_MINUS_ONE)
                    break;
                case MT_NONEwithRSA:
                    ADD_JSON_RAW(&encoder, ',')
                    ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_method), getDBstr(icd_key_NONEwithRSA),
                                      SINT_MINUS_ONE)
                    break;
                case MT_NONEwithECDSA:
                    ADD_JSON_RAW(&encoder, ',')
                    ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_method), getDBstr(icd_key_NONEwithECDSA),
                                      SINT_MINUS_ONE)
                    /* comment for misra compliance */
                    break;
                default:
                    /* comment for misra compliance */
                    break;
            }

            ADD_JSON_RAW(&encoder, ',')
            ADD_JSON_KEY(&encoder, getDBstr(icd_key_hash))
            ADD_JSON_RAW(&encoder, ':')
            ADD_JSON_RAW(&encoder, '{')
            ADD_JSON_PAIR_HEX(&encoder, getDBstr(icd_key_digest), sigInfo[i].hash.digest, SHA256_LENGTH)
            switch (sigInfo[i].hash.funcType) {
                case FT_SHA256:
                    ADD_JSON_RAW(&encoder, ',')
                    ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_function), getDBstr(icd_key_SHA256), SINT_MINUS_ONE)
                    break;
                default:
                    /* comment for misra compliance */
                    break;
            }
            ADD_JSON_RAW(&encoder, '}')
            ADD_JSON_RAW(&encoder, ',')
            size_t len = MIN(MAX_SIG_LENGTH, sigInfo[i].value_len);
            if (len <= MAX_SIG_LENGTH) {
                ADD_JSON_PAIR_HEX(&encoder, getDBstr(icd_key_value), sigInfo[i].value, len)
            }
            ADD_JSON_RAW(&encoder, '}')
        }
        ADD_JSON_RAW(&encoder, ']')

        error:
        if (rval != UTIL_NO_ERROR) {
            ABQ_CLEAR_ENCODER(encoder);
        }
    } else {
        rval = UTIL_ERR_INVALID_PARAMETER;
    }
    return rval;
}

utilErr_t util_ParseTrustRootMetadata(str_t const signedStr, const size_t length,
                                      util_json_t mem[],
                                      const uint16_t qty,
                                      rootMeta_t *const out) {
    utilErr_t rval = UTIL_NO_ERROR;
    // Check populated parameters
    if (NULL == signedStr) {
        DEBUG_PRINT("ERR: Input string is null.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (UINT_TWO > length) {
        DEBUG_PRINT("ERR: Length of target string is 0 or is less than 0.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (NULL == mem) {
        DEBUG_PRINT("ERR: Array of json properties to allocate is null\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (UINT_ZERO >= qty) {
        DEBUG_PRINT("ERR: Number of elements on array of json properties to allocate is 0 or is less than 0\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (NULL == out) {
        DEBUG_PRINT("ERR: Structure for storing parsed data is null\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    rval = util_ParseUptRootMetadata(signedStr, length, mem, qty, out);

error:
    if (rval != UTIL_NO_ERROR) {
        CLEAR_OUTPUT_STRUCTURE(out, sizeof(rootMeta_t));
    }
    return rval;
}


static utilErr_t addKeyInfo(abq_encoder_t *const encoder, const keyInfo_t *const ki) {
    utilErr_t rval = UTIL_NO_ERROR;
    ADD_JSON_RAW(encoder, '{')
    ADD_JSON_PAIR_HEX(encoder, getDBstr(icd_key_publicKeyId), ki->id, SHA256_LENGTH)

    ADD_JSON_RAW(encoder, ',')
    size_t len = MIN(ki->value_len, MAX_KEY_LENGTH);
    if (len <= MAX_KEY_LENGTH) {
        ADD_JSON_PAIR_HEX(encoder, getDBstr(icd_key_publicKeyValue), ki->value, len)
    }
    switch (ki->key_type) {
        case KT_RSA:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_publicKeyType), getDBstr(icd_key_RSA), SINT_MINUS_ONE)
            break;
        case KT_ECDSA:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_publicKeyType), getDBstr(icd_key_ECDSA), SINT_MINUS_ONE)
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }
    ADD_JSON_RAW(encoder, '}')
    return rval;
}

static utilErr_t addRoleInfo(abq_encoder_t *const encoder, const roleInfo_t *const ri) {
    utilErr_t rval = UTIL_NO_ERROR;
    ADD_JSON_RAW(encoder, '{')
    ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_threshold), ri->threshold)

    switch (ri->role) {
        case RT_ROOT:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_role), getDBstr(icd_key_root), SINT_MINUS_ONE)
            break;
        case RT_TARGETS:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_role), getDBstr(icd_key_targets), SINT_MINUS_ONE)
            break;
        case RT_SNAPSHOT:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_role), getDBstr(icd_key_snapshot), SINT_MINUS_ONE)
            break;
        case RT_TIMESTAMP:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_role), getDBstr(icd_key_timestamp), SINT_MINUS_ONE)
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }

    if (ri->idsNum > MAX_KEY_NUM_PER_ROLE) {
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    } else if (ri->idsNum > UINT_ZERO) {
        ADD_JSON_RAW(encoder, ',')
        ADD_JSON_KEY(encoder, getDBstr(icd_key_keyids))
        ADD_JSON_RAW(encoder, ':')
        ADD_JSON_RAW(encoder, '[')
        for (size_t i = SINT_ZERO; i < MIN(MAX_KEY_NUM_PER_ROLE, (uint64_t) ri->idsNum); ++i) {
            if (i > UINT_ZERO) {
                ADD_JSON_RAW(encoder, ',')
            }
            ADD_JSON_HEX(encoder, &ri->ids[i][SINT_ZERO], SHA256_LENGTH)
        }
        ADD_JSON_RAW(encoder, ']')
    } else {
        /* nothing */
    }
    ADD_JSON_RAW(encoder, '}')
    error:
    return rval;
}

utilErr_t util_CreateFromTrustRootMeta(const rootMeta_t *const rootMeta, str_t const buf,
                                       const size_t bufLen) {
    utilErr_t rval = UTIL_NO_ERROR;

    if ((NULL != rootMeta) && (NULL != buf) && (UINT_ZERO < bufLen)) {
        ABQ_ENCODER(encoder, getUTF8Codec, buf, bufLen);
        ABQ_CLEAR_ENCODER(encoder);
        ADD_JSON_RAW(&encoder, '{')
        ADD_JSON_PAIR_UINT(&encoder, getDBstr(icd_key_version), rootMeta->version)
        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_UINT(&encoder, getDBstr(icd_key_expires), rootMeta->expires)

        switch (rootMeta->type) {
            case RT_ROOT:
                ADD_JSON_RAW(&encoder, ',')
                ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_type), getDBstr(icd_key_root), SINT_MINUS_ONE)
                break;
            default:
                /* MISRA __NOP comment */
                break;
        }

        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_KEY(&encoder, getDBstr(icd_key_body))
        ADD_JSON_RAW(&encoder, ':')
        ADD_JSON_RAW(&encoder, '{')

        if (rootMeta->rootMetaBody.keysNum > (MAX_TOTAL_KEY_NUM)) {
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
        } else if (rootMeta->rootMetaBody.keysNum > UINT_ZERO) {
            ADD_JSON_KEY(&encoder, getDBstr(icd_key_keys))
            ADD_JSON_RAW(&encoder, ':')
            ADD_JSON_RAW(&encoder, '[')
            for (size_t i = UINT_ZERO; i < MIN((uint64_t) rootMeta->rootMetaBody.keysNum, (MAX_TOTAL_KEY_NUM)); ++i) {
                if (i > UINT_ZERO) {
                    ADD_JSON_RAW(&encoder, ',')
                }
                if (UTIL_NO_ERROR == rval) {
                    if(rootMeta->rootMetaBody.keys[i].value_len > MAX_KEY_LENGTH) {
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    }
                    rval = addKeyInfo(&encoder, &rootMeta->rootMetaBody.keys[i]);
                }
                if (rval != UTIL_NO_ERROR) {
                    goto error;
                }
            }
            ADD_JSON_RAW(&encoder, ']')
        } else {
            /*  nothing */
        }

        if (rootMeta->rootMetaBody.rolesNum > (uint8_t) NUM_ROLES) {
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
        } else if (rootMeta->rootMetaBody.keysNum > UINT_ZERO) {
            ADD_JSON_RAW(&encoder, ',')
            ADD_JSON_KEY(&encoder, getDBstr(icd_key_roles))
            ADD_JSON_RAW(&encoder, ':')
            ADD_JSON_RAW(&encoder, '[')
            for (size_t i = UINT_ZERO; i < MIN((uint64_t) rootMeta->rootMetaBody.rolesNum, (uint64_t) NUM_ROLES); ++i) {
                if (i > UINT_ZERO) {
                    ADD_JSON_RAW(&encoder, ',')
                }
                if (UTIL_NO_ERROR == rval) {
                    rval = addRoleInfo(&encoder, &rootMeta->rootMetaBody.roles[i]);
                }
                if (rval != UTIL_NO_ERROR) {
                    goto error;
                }
            }
            ADD_JSON_RAW(&encoder, ']')
        } else {
            /*  nothing */
        }
        ADD_JSON_RAW(&encoder, '}')

        ADD_JSON_RAW(&encoder, '}')

        error:
        if (rval != UTIL_NO_ERROR) {
            ABQ_CLEAR_ENCODER(encoder);
        }
    } else {
        rval = UTIL_ERR_INVALID_PARAMETER;
    }
    return rval;
}

utilErr_t util_FindSignatureSignedBlock(cstr_t const jsonStr,
                                        const size_t strLen,
                                        secInfo_t *const secInfo) {
    utilErr_t rval = UTIL_NO_ERROR;
    if (NULL == jsonStr) {
        DEBUG_PRINT("ERR: Input string is null.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (UINT_TWO > strLen) {
        DEBUG_PRINT("ERR: Length of target string is 0 or is less than 0.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (NULL == secInfo) {
        DEBUG_PRINT("ERR: Output structure pointer is null.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    secInfo->headSigned = GetHeadSignedBody(jsonStr, &(secInfo->signedLength));
    if (NULL == secInfo->headSigned) {
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    secInfo->headSignatures = GetHeadSignaturesBody(jsonStr, &(secInfo->signaturesLength));
    if (NULL == secInfo->headSignatures) {
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    error:
    return rval;
}

utilErr_t util_CreateMpuInitBody(const mpuInit_t *const mpuInit, str_t const buf,
                                 const size_t bufLen) {
    utilErr_t rval = UTIL_NO_ERROR;

    if ((NULL != mpuInit) && (NULL != buf) && (UINT_ZERO < bufLen)) {
        ABQ_ENCODER(encoder, getUTF8Codec, buf, bufLen);
        ABQ_CLEAR_ENCODER(encoder);
        ADD_JSON_RAW(&encoder, '{')
        ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_primaryIdentifier), mpuInit->primarySerialNum,
                          (int32_t) MAX_SERIAL_NUM_SZ)

        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_vehicleIdentifier), mpuInit->vin, (int32_t) MAX_VIN_SZ)

        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_UINT(&encoder, getDBstr(icd_key_currentTime), mpuInit->currentTime)

        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_STR(&encoder, "device-id", mpuInit->deviceId, (int32_t) MAX_DEVICE_ID_SZ)

        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_STR(&encoder, "client-type", mpuInit->clientType, (int32_t) MAX_CLIENT_TYPE_SZ)

        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_description), mpuInit->description, (int32_t) MAX_DESC_SZ)

        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_STR(&encoder, "begin-time", mpuInit->beginTime, (int32_t) FORMATED_TIME_SZ)

        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_STR(&encoder, "end-time", mpuInit->endTime, (int32_t) FORMATED_TIME_SZ)

        switch (mpuInit->category) {
            case CATEGORY_LOG:
                ADD_JSON_RAW(&encoder, ',')
                ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_category), getDBstr(icd_key_log), SINT_MINUS_ONE)
                break;
            default:
                /* MISRA __NOP comment */
                break;
        }

        switch (mpuInit->trsprtType) {
            case TRSPRT_WIFI:
                ADD_JSON_RAW(&encoder, ',')
                ADD_JSON_PAIR_STR(&encoder, "transport-type", getDBstr(icd_key_wifi), SINT_MINUS_ONE)
                break;
            case TRSPRT_CELLULAR:
                ADD_JSON_RAW(&encoder, ',')
                ADD_JSON_PAIR_STR(&encoder, "transport-type", getDBstr(icd_key_cellular), SINT_MINUS_ONE)
                break;
            case TRSPRT_BLUETOOTH:
                ADD_JSON_RAW(&encoder, ',')
                ADD_JSON_PAIR_STR(&encoder, "transport-type", getDBstr(icd_key_bluetooth), SINT_MINUS_ONE)
                break;
            default:
                /* MISRA __NOP comment */
                break;
        }

        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_STR(&encoder, "client-upload-id", mpuInit->clientUploadId, (int32_t) MAX_CLIENT_UPLOAD_ID_SZ)

        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_STR(&encoder, "report-id", mpuInit->reportId, (int32_t) MAX_REPORT_ID_SZ)

        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_STR(&encoder, "file-name", mpuInit->fileName, (int32_t) MAX_FILE_NAME_SZ)

        if (mpuInit->eventUuidNum > MAX_UUID_NUM) {
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
        } else if (mpuInit->eventUuidNum > UINT_ZERO) {
            ADD_JSON_RAW(&encoder, ',')
            ADD_JSON_KEY(&encoder, getDBstr(icd_key_eventUuid))
            ADD_JSON_RAW(&encoder, ':')
            ADD_JSON_RAW(&encoder, '[')
            for (size_t i = SINT_ZERO; i < MIN(MAX_KEY_NUM_PER_ROLE, (uint64_t) mpuInit->eventUuidNum); ++i) {
                if (i > UINT_ZERO) {
                    ADD_JSON_RAW(&encoder, ',')
                }
                ADD_JSON_STR(&encoder, &mpuInit->eventUuids[i][SINT_ZERO], (int32_t) UUID_LEN)
            }
            ADD_JSON_RAW(&encoder, ']')
        } else {
            /* nothing */
        }

        if (mpuInit->uplPartReqNum > MAX_UPLOAD_PARTS_REQ_NUM) {
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
        } else if (mpuInit->uplPartReqNum > UINT_ZERO) {
            ADD_JSON_RAW(&encoder, ',')
            ADD_JSON_KEY(&encoder, getDBstr(icd_key_uploadParts))
            ADD_JSON_RAW(&encoder, ':')
            ADD_JSON_RAW(&encoder, '[')
            for (size_t i = SINT_ZERO; i < MIN(MAX_UPLOAD_PARTS_REQ_NUM, (uint64_t) mpuInit->uplPartReqNum); ++i) {
                if (i > UINT_ZERO) {
                    ADD_JSON_RAW(&encoder, ',')
                }
                ADD_JSON_RAW(&encoder, '{')
                ADD_JSON_PAIR_UINT(&encoder, getDBstr(icd_key_partNumber),
                                   (uint64_t) mpuInit->uploadPartReq[i].partNumber)
                ADD_JSON_RAW(&encoder, ',')
                ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_md5CheckSum), mpuInit->uploadPartReq[i].md5CheckSum,
                                  (int32_t) MAX_BASE64_MD5_SZ)
                ADD_JSON_RAW(&encoder, '}')
            }
            ADD_JSON_RAW(&encoder, ']')
        } else {
            /* nothing */
        }

        ADD_JSON_RAW(&encoder, '}')
        error:
        if (rval != UTIL_NO_ERROR) {
            ABQ_CLEAR_ENCODER(encoder);
        }
    } else {
        rval = UTIL_ERR_INVALID_PARAMETER;
    }
    return rval;
}

utilErr_t util_ParseMpuInitResponse(str_t const str, const size_t length,
                                    util_json_t mem[],
                                    const uint16_t qty,
                                    mpuInitResponse_t *const out) {
    utilErr_t rval = UTIL_NO_ERROR;
    jsonErr_t json_ret = JSON_NO_ERROR;
    json_t const *root = NULL;
    json_t const *obj = NULL;

    // Check populated parameters
    if (NULL == str) {
        DEBUG_PRINT("ERR: Input string is null.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (UINT_TWO > length) {
        DEBUG_PRINT("ERR: Length of target string is 0 or is less than 0.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (NULL == mem) {
        DEBUG_PRINT("ERR: Array of json properties to allocate is null\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (UINT_ZERO >= qty) {
        DEBUG_PRINT("ERR: Number of elements on array of json properties to allocate is 0 or is less than 0\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (NULL == out) {
        DEBUG_PRINT("ERR: Structure for storing parsed data is null\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    json_ret = json_create_ex(&root, str, length, mem, qty);
    switch (json_ret) {
        case JSON_ERR_INVALID_PARAMETER:
            DEBUG_PRINT("ERR: Target string is not valid.\n");
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
            break;
        case JSON_ERR_NO_RESOURCE:
            DEBUG_PRINT("ERR: Array of json properties to allocate is too short for this JSON string.\n");
            rval = UTIL_ERR_NO_RESOURCE;
            goto error;
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }
    CLEAR_OUTPUT_STRUCTURE(out, sizeof(mpuInitResponse_t));
    if (NULL != root) {

        COPY_TEXT_VAL_FOR_MNDTRY_KEY(root, icd_key_uploadContext, out->uploadContext, MAX_UPLOAD_CONTEXT_SZ)

        const json_t *const uploadParts = json_getProperty(root, getDBstr(icd_key_uploadParts));
        ERROR_ON_NULL(uploadParts)
        if (JSON_ARRAY != json_getType(uploadParts)) {
            DEBUG_PRINT("ERR: %s should be a JSON array\n", json_getName(uploadParts));
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
        }

        uint16_t cnt = UINT_ZERO;
        const json_t *uplPart = json_getChild(uploadParts);
        while (NULL != uplPart) {
            if (MAX_UPLOAD_PARTS_RES_NUM > cnt) {
                if (JSON_OBJ != json_getType(uplPart)) {
                    DEBUG_PRINT("ERR: %s should be a JSON object\n", json_getName(uplPart));
                    rval = UTIL_ERR_INVALID_PARAMETER;
                    goto error;
                }
                COPY_UINT_VAL_FOR_MNDTRY_KEY(uplPart, icd_key_partNumber, out->uploadPartRes[cnt].partNumber, uint16_t, UINT16_MAX)
                COPY_TEXT_VAL_FOR_MNDTRY_KEY(uplPart, icd_key_md5CheckSum, out->uploadPartRes[cnt].md5CheckSum,
                                             MAX_BASE64_MD5_SZ)
                COPY_TEXT_VAL_FOR_MNDTRY_KEY(uplPart, icd_key_uploadURL, out->uploadPartRes[cnt].uploadURL,
                                             MAX_UPLD_URL_SZ)

            } else {
                DEBUG_PRINT("No resource for uploadParts info.\n");
                rval = UTIL_ERR_NO_RESOURCE;
                goto error;
                break;
            }
            uplPart = json_getSibling(uplPart);
            cnt++;
        }
        if (rval == UTIL_NO_ERROR) {
            out->upPartResNum = cnt;
        }
    }

    error:
    if (rval != UTIL_NO_ERROR) {
        CLEAR_OUTPUT_STRUCTURE(out, sizeof(mpuInitResponse_t));
    }
    return rval;
}


utilErr_t util_CreateMpuCompBody(const mpuComp_t *const mpuComp, str_t const buf,
                                 const size_t bufLen) {
    utilErr_t rval = UTIL_NO_ERROR;

    if ((NULL != mpuComp) && (NULL != buf) && (UINT_ZERO < bufLen)) {
        ABQ_ENCODER(encoder, getUTF8Codec, buf, bufLen);
        ABQ_CLEAR_ENCODER(encoder);
        ADD_JSON_RAW(&encoder, '{')
        ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_primaryIdentifier), mpuComp->primarySerialNum,
                          (int32_t) MAX_SERIAL_NUM_SZ)

        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_vehicleIdentifier), mpuComp->vin, (int32_t) MAX_VIN_SZ)

        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_UINT(&encoder, getDBstr(icd_key_currentTime), mpuComp->currentTime)

        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_uploadContext), mpuComp->uploadContext,
                          (int32_t) MAX_UPLOAD_CONTEXT_SZ)

        if (mpuComp->partETagNum > MAX_PART_ETAG_NUM) {
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
        } else if (mpuComp->partETagNum > UINT_ZERO) {
            ADD_JSON_RAW(&encoder, ',')
            ADD_JSON_KEY(&encoder, getDBstr(icd_key_partETags))
            ADD_JSON_RAW(&encoder, ':')
            ADD_JSON_RAW(&encoder, '[')
            for (size_t i = UINT_ZERO; i < MIN(MAX_PART_ETAG_NUM, (uint64_t) mpuComp->partETagNum); ++i) {
                if (i > UINT_ZERO) {
                    ADD_JSON_RAW(&encoder, ',')
                }
                ADD_JSON_RAW(&encoder, '{')
                ADD_JSON_PAIR_UINT(&encoder, getDBstr(icd_key_partNumber), (uint64_t) mpuComp->partETags[i].partNumber)
                ADD_JSON_RAW(&encoder, ',')
                ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_etag), mpuComp->partETags[i].etag, (int32_t) MAX_ETAG_SZ)
                ADD_JSON_RAW(&encoder, '}')
            }
            ADD_JSON_RAW(&encoder, ']')
        } else {
            /* nothing */
        }

        ADD_JSON_RAW(&encoder, '}')
        error:
        if (rval != UTIL_NO_ERROR) {
            ABQ_CLEAR_ENCODER(encoder);
        }
    } else {
        rval = UTIL_ERR_INVALID_PARAMETER;
    }
    return rval;
}

utilErr_t util_ParseGetOtaCmdRes(str_t const signedStr,
                                 const size_t length,
                                 util_json_t mem[], const uint16_t qty,
                                 getOtaCmdRes_t *const out) {
    utilErr_t rval = UTIL_NO_ERROR;
    jsonErr_t json_ret = JSON_NO_ERROR;
    json_t const *root = NULL;
    json_t const *obj = NULL;
    cstr_t out_str_val_ptr = NULL;


    // Check populated parameters
    if (NULL == signedStr) {
        DEBUG_PRINT("ERR: Input string is null.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (UINT_TWO > length) {
        DEBUG_PRINT("ERR: Length of target string is 0 or is less than 0.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (NULL == mem) {
        DEBUG_PRINT("ERR: Array of json properties to allocate is null\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (UINT_ZERO >= qty) {
        DEBUG_PRINT("ERR: Number of elements on array of json properties to allocate is 0 or is less than 0\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (NULL == out) {
        DEBUG_PRINT("ERR: Structure for storing parsed data is null\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    json_ret = json_create_ex(&root, signedStr, length, mem, qty);
    switch (json_ret) {
        case JSON_ERR_INVALID_PARAMETER:
            DEBUG_PRINT("ERR: Target string is not valid.\n");
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
            break;
        case JSON_ERR_NO_RESOURCE:
            DEBUG_PRINT("ERR: Array of json properties to allocate is too short for this JSON string.\n");
            rval = UTIL_ERR_NO_RESOURCE;
            goto error;
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }
    CLEAR_OUTPUT_STRUCTURE(out, sizeof(getOtaCmdRes_t));
    if (NULL != root) {

        COPY_UINT_VAL_FOR_MNDTRY_KEY(root, icd_key_version, out->version, uint32_t, UINT32_MAX)
        COPY_UINT_VAL_FOR_MNDTRY_KEY(root, icd_key_expires, out->expires, uint64_t, UINT64_MAX)
        GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(root, icd_key_type)
        /* always evaluates to true if 'icd_mandatory' is used */
        if (LAST_SET_STR_VAL_PTR != NULL) {
            if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_targets),
                             strlen(getDBstr(icd_key_targets)))) {
                out->type = RT_TARGETS;
            } else {
                out->type = RT_UNKNOWN;
                DEBUG_PRINT("ERR: Metadata Type mismatches expectation: %s \n", getDBstr(icd_key_targets));
                rval = UTIL_ERR_INVALID_PARAMETER;
                goto error;
            }
        }

        const json_t *const body = json_getProperty(root, getDBstr(icd_key_body));
        ERROR_ON_NULL(body)
        if (JSON_OBJ != json_getType(body)) {
            DEBUG_PRINT("ERR: %s should be a JSON object\n", json_getName(body));
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
        }

        COPY_TEXT_VAL_FOR_MNDTRY_KEY(body, icd_key_vehicleIdentifier, out->getOtaCmdResBody.vin, MAX_VIN_SZ)
        COPY_UINT_VAL_FOR_OPTNL_KEY(body, icd_key_commandId, out->getOtaCmdResBody.cmdId, uint64_t, UINT64_MAX)
        GET_TEXT_VAL_PTR_FOR_OPTNL_KEY(body, icd_key_command)
        if (LAST_SET_STR_VAL_PTR != NULL) {
            if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_UpldVehclCnfg),
                             strlen(getDBstr(icd_key_UpldVehclCnfg)))) {
                out->getOtaCmdResBody.getOtaCmdType = OTA_CMD_TYPE_CONFIG_MATCH;
            } else if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_UpldVehclLog),
                                    strlen(getDBstr(icd_key_UpldVehclLog)))) {
                out->getOtaCmdResBody.getOtaCmdType = OTA_CMD_TYPE_UPLD_LOG;
            } else {
                out->getOtaCmdResBody.getOtaCmdType = OTA_CMD_TYPE_NOT_APPLICABLE;
            }
        }
    }

    error:
    if (rval != UTIL_NO_ERROR) {
        CLEAR_OUTPUT_STRUCTURE(out, sizeof(getOtaCmdRes_t));
    }
    return rval;
}

utilErr_t
util_ParseSignature(str_t const sigStr, const size_t length, util_json_t mem[], const uint16_t qty, sigInfo_t sigs[], const uint16_t maxSigsNum,
                    uint16_t *const validSigNum) {
    utilErr_t rval = UTIL_NO_ERROR;
    uint16_t i = SINT_ZERO;
    json_t const *obj = NULL;
    cstr_t out_str_val_ptr = NULL;

    json_t const *sig_root = NULL;
    json_t const *sig = NULL;
    jsonErr_t json_ret = JSON_NO_ERROR;

    // Check populated parameters
    if (NULL == sigStr) {
        DEBUG_PRINT("ERR: Target string is null.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    if (UINT_TWO > length) {
        DEBUG_PRINT("ERR: Length of target string is 0 or is less than 0.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    if (NULL == mem) {
        DEBUG_PRINT("ERR: Array of json properties to allocate is null.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    if (UINT_ZERO >= qty) {
        DEBUG_PRINT("ERR: Number of elements on array of json properties to allocate is 0 or is less than 0.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    if (NULL == sigs) {
        DEBUG_PRINT("ERR: Array for storing parsed signature data is null.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    if (NULL == validSigNum) {
        DEBUG_PRINT("ERR: validSigNum is NULL.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    if (UINT_ZERO >= maxSigsNum) {
        DEBUG_PRINT("ERR: Size of array for storing parsed signature data is 0 or is less than 0.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    json_ret = json_create_ex(&sig_root, sigStr, length, mem, qty);
    switch (json_ret) {
        case JSON_ERR_INVALID_PARAMETER:
            DEBUG_PRINT("ERR: Target string is not valid.\n");
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
            break;
        case JSON_ERR_NO_RESOURCE:
            DEBUG_PRINT("ERR: Array of json properties to allocate is too short for this JSON string.\n");
            rval = UTIL_ERR_NO_RESOURCE;
            goto error;
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }

    ERROR_ON_NULL(sig_root)
    if (JSON_ARRAY != json_getType(sig_root)) {
        DEBUG_PRINT("ERR: Signature should be array object.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    i = SINT_ZERO;
    sig = json_getChild(sig_root);
    while (NULL != sig) {
        if (maxSigsNum >= i) {
            if (JSON_OBJ != json_getType(sig)) {
                DEBUG_PRINT("ERR: %s should be a JSON object\n", json_getName(sig));
                rval = UTIL_ERR_INVALID_PARAMETER;
                goto error;
            }
            const json_t *const hash_f = json_getProperty(sig, getDBstr(icd_key_hash));
            ERROR_ON_NULL(hash_f)
            if (json_getType(hash_f) != JSON_OBJ) {
                DEBUG_PRINT("ERR: Hash should be object type.\n");
                rval = UTIL_ERR_INVALID_PARAMETER;
                goto error;
            }

            GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(hash_f, icd_key_digest)
            /* always evaluates to true if 'icd_mandatory' is used */
            if (NULL != LAST_SET_STR_VAL_PTR) {
                if ((strnlength(LAST_SET_STR_VAL_PTR, ((SHA256_LENGTH * UINT_TWO) + UINT_ONE))) != (SHA256_LENGTH * UINT_TWO)) {
                    DEBUG_PRINT("ERR: Incorrect digest input length\n");
                    rval = UTIL_ERR_INVALID_PARAMETER;
                    goto error;
                }
                const int32_t ret = abq_hex_decode(LAST_SET_STR_VAL_PTR,
                                             (int32_t)(SHA256_LENGTH * UINT_TWO),
                                             abq_ptr2raw(sigs[i].hash.digest),
                                             (int32_t) SHA256_LENGTH);
                if (ret < SINT_ZERO) {
                    DEBUG_PRINT("ERR: Hex decode to binary failed\n");
                    rval = UTIL_ERR_INVALID_PARAMETER;
                    goto error;
                }
            }

            GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(hash_f, icd_key_function)
            /* always evaluates to true if 'icd_mandatory' is used */
            if (LAST_SET_STR_VAL_PTR != NULL) {
                if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_SHA256),
                                         strlen(getDBstr(icd_key_SHA256)))) {
                    sigs[i].hash.funcType = FT_SHA256;
                } else {
                    sigs[i].hash.funcType = FT_UNKNOWN;
                    DEBUG_PRINT("ERR: Value mismatches expectation for: %s\n", getDBstr(icd_key_function));
                    rval = UTIL_ERR_INVALID_PARAMETER;
                    goto error;
                }
            }

            GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(sig, icd_key_keyid)
            /* always evaluates to true if 'icd_mandatory' is used */
            if (NULL != LAST_SET_STR_VAL_PTR) {
                if ((strnlength(LAST_SET_STR_VAL_PTR, ((SHA256_LENGTH * UINT_TWO) + UINT_ONE))) != (SHA256_LENGTH * UINT_TWO)) {
                    DEBUG_PRINT("ERR: Incorrect key_id input length\n");
                    rval = UTIL_ERR_INVALID_PARAMETER;
                    goto error;
                }
                const int32_t ret = abq_hex_decode(LAST_SET_STR_VAL_PTR,
                                             (int32_t)(SHA256_LENGTH * UINT_TWO),
                                             abq_ptr2raw(sigs[i].keyid),
                                             (int32_t) SHA256_LENGTH);
                if (ret < SINT_ZERO) {
                    DEBUG_PRINT("ERR: Hex decode to binary failed\n");
                    rval = UTIL_ERR_INVALID_PARAMETER;
                    goto error;
                }
            }

            GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(sig, icd_key_method)
            /* always evaluates to true if 'icd_mandatory' is used */
            if (NULL != LAST_SET_STR_VAL_PTR) {
                cstr_t const rsa256_string = getDBstr(icd_key_SHA256withRSA);
                cstr_t const rsa_string = getDBstr(icd_key_NONEwithRSA);
                cstr_t const ecdsa256_string = getDBstr(icd_key_SHA256withECDSA);
                cstr_t const ecdsa_string = getDBstr(icd_key_NONEwithECDSA);
                DEBUG_PRINT("method: %s\n", LAST_SET_STR_VAL_PTR);
                if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) rsa256_string, strlen(rsa256_string))) {
                    sigs[i].methType = MT_SHA256withRSA;
                } else if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) ecdsa256_string, strlen(ecdsa256_string))) {
                    sigs[i].methType = MT_SHA256withECDSA;
                } else if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) rsa_string, strlen(rsa_string))) {
                    sigs[i].methType = MT_NONEwithRSA;
                } else if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) ecdsa_string, strlen(ecdsa_string))) {
                    sigs[i].methType = MT_NONEwithECDSA;
                } else {
                    sigs[i].methType = MT_UNKNOWN;
                    DEBUG_PRINT("ERR: Value mismatches expectation for: %s\n", getDBstr(icd_key_method));
                    rval = UTIL_ERR_INVALID_PARAMETER;
                    goto error;
                }
            }
            GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(sig, icd_key_value)
            /* always evaluates to true if 'icd_mandatory' is used */
            if (NULL != LAST_SET_STR_VAL_PTR) {
                DEBUG_PRINT("value: %s\n", LAST_SET_STR_VAL_PTR);
                // get actual input length and allow +1 byte than target storage size
                const size_t sz = strnlength(LAST_SET_STR_VAL_PTR, ((MAX_SIG_LENGTH * UINT_TWO) + UINT_ONE));
                // will it fit into target storage area?
                if (sz > (MAX_SIG_LENGTH * UINT_TWO)) {
                    DEBUG_PRINT("ERR:sigs.value buffer is smaller than input sig length\n");
                    rval = UTIL_ERR_INVALID_PARAMETER;
                    goto error;
                }
                // input must be even number of characters
                if ((sz % UINT_TWO) != UINT_ZERO) {
                    DEBUG_PRINT("ERR: Signature value size/length is not multiple of 2\n");
                    rval = UTIL_ERR_INVALID_PARAMETER;
                    goto error;
                }
                sigs[i].value_len = (sz / UINT_TWO);
                const int32_t ret = abq_hex_decode(LAST_SET_STR_VAL_PTR,
                                             (int32_t) sz,
                                             abq_ptr2raw(sigs[i].value),
                                             (int32_t) MAX_SIG_LENGTH);
                if (ret < SINT_ZERO) {
                    DEBUG_PRINT("ERR: Hex decode to binary failed\n");
                    rval = UTIL_ERR_INVALID_PARAMETER;
                    goto error;
                }
            }
        } else {
            DEBUG_PRINT("No resource for signatures\n");
            rval = UTIL_ERR_NO_RESOURCE;
            break;
        }
        sig = json_getSibling(sig);
        i++;
    }
    if ((rval == UTIL_NO_ERROR) && (validSigNum != NULL)) {
        *validSigNum = i;
    }

    error:
    return rval;
}

utilErr_t util_ParseUptTimeMetadata(str_t const signedStr,
                                    const size_t length,
                                    util_json_t mem[],
                                    const uint16_t qty,
                                    timeMeta_t *const out) {
    utilErr_t rval = UTIL_NO_ERROR;
    jsonErr_t json_ret = JSON_NO_ERROR;
    json_t const *root = NULL;
    json_t const *obj = NULL;
    cstr_t out_str_val_ptr = NULL;


    // Check populated parameters
    if (NULL == signedStr) {
        DEBUG_PRINT("ERR: Input string is null.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (UINT_TWO > length) {
        DEBUG_PRINT("ERR: Length of target string is too short for valid JSON\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (NULL == mem) {
        DEBUG_PRINT("ERR: Array of json properties to allocate is null\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (UINT_ZERO >= qty) {
        DEBUG_PRINT("ERR: Number of elements on array of json properties to allocate is 0 or is less than 0\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (NULL == out) {
        DEBUG_PRINT("ERR: Structure for storing parsed data is null\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    json_ret = json_create_ex(&root, signedStr, length, mem, qty);
    switch (json_ret) {
        case JSON_ERR_INVALID_PARAMETER:
            DEBUG_PRINT("ERR: Target string is not valid.\n");
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
            break;
        case JSON_ERR_NO_RESOURCE:
            DEBUG_PRINT("ERR: Array of json properties to allocate is too short for this JSON string.\n");
            rval = UTIL_ERR_NO_RESOURCE;
            goto error;
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }
    CLEAR_OUTPUT_STRUCTURE(out, sizeof(timeMeta_t));
    if (NULL != root) {

        COPY_UINT_VAL_FOR_MNDTRY_KEY(root, icd_key_version, out->version, uint32_t, UINT32_MAX)
        COPY_UINT_VAL_FOR_MNDTRY_KEY(root, icd_key_expires, out->expires, uint64_t, UINT64_MAX)
        GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(root, icd_key_type)
        /* always evaluates to true if 'icd_mandatory' is used */
        if (LAST_SET_STR_VAL_PTR != NULL) {
            if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_timestamp),
                             strlen(getDBstr(icd_key_timestamp)))) {
                out->type = RT_TIMESTAMP;
            } else {
                out->type = RT_UNKNOWN;
                DEBUG_PRINT("ERR: Metadata Type mismatches expectation: %s \n", getDBstr(icd_key_timestamp));
                rval = UTIL_ERR_INVALID_PARAMETER;
                goto error;
            }
        }

        const json_t *const body = json_getProperty(root, getDBstr(icd_key_body));
        ERROR_ON_NULL(body)
        if (JSON_OBJ != json_getType(body)) {
            DEBUG_PRINT("ERR: %s should be a JSON object\n", json_getName(body));
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
        }

        COPY_TEXT_VAL_FOR_MNDTRY_KEY(body, icd_key_filename, out->timeMetaBody.fileName, MAX_FILE_NAME_SZ)
        COPY_UINT_VAL_FOR_MNDTRY_KEY(body, icd_key_version, out->timeMetaBody.version, uint32_t, UINT32_MAX)
        COPY_UINT_VAL_FOR_MNDTRY_KEY(body, icd_key_length, out->timeMetaBody.length, uint32_t, UINT32_MAX)

        const json_t *const hashes = json_getProperty(body, "hashes");
        ERROR_ON_NULL(hashes)
        if (JSON_ARRAY != json_getType(hashes)) {
            DEBUG_PRINT("ERR: %s should be a JSON array\n", json_getName(hashes));
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
        }
        uint16_t cnt = UINT_ZERO;
        const json_t *hash = json_getChild(hashes);
        while (NULL != hash) {
            if (MAX_HASH_NUM > cnt) {
                if (JSON_OBJ != json_getType(hash)) {
                    DEBUG_PRINT("ERR: %s should be a JSON object\n", json_getName(hash));
                    rval = UTIL_ERR_INVALID_PARAMETER;
                    goto error;
                }

                GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(hash, icd_key_digest)
                /* always evaluates to true if 'icd_mandatory' is used */
                if (NULL != LAST_SET_STR_VAL_PTR) {
                    if ((strnlength(LAST_SET_STR_VAL_PTR, ((SHA256_LENGTH * UINT_TWO) + UINT_ONE))) != (SHA256_LENGTH * UINT_TWO)) {
                        DEBUG_PRINT("ERR: Incorrect digest input length\n");
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    }
                    const int32_t ret = abq_hex_decode(LAST_SET_STR_VAL_PTR,
                                                 (int32_t)(SHA256_LENGTH * UINT_TWO),
                                                 abq_ptr2raw(out->timeMetaBody.hashes[cnt].digest),
                                                 (int32_t) SHA256_LENGTH);
                    if (ret < SINT_ZERO) {
                        DEBUG_PRINT("ERR: Hex decode to binary failed\n");
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    }
                }

                GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(hash, icd_key_function)
                /* always evaluates to true if 'icd_mandatory' is used */
                if (LAST_SET_STR_VAL_PTR != NULL) {
                    if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_SHA256),
                                     strlen(getDBstr(icd_key_SHA256)))) {
                        out->timeMetaBody.hashes[cnt].funcType = FT_SHA256;
                    } else {
                        out->timeMetaBody.hashes[cnt].funcType = FT_UNKNOWN;
                        DEBUG_PRINT("ERR: Value mismatches expectation for: %s\n", getDBstr(icd_key_function));
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    }
                }

            } else {
                DEBUG_PRINT("No resource for hash info.\n");
                rval = UTIL_ERR_NO_RESOURCE;
                goto error;
                break;
            }
            hash = json_getSibling(hash);
            cnt++;
        }
        if (rval == UTIL_NO_ERROR) {
            out->timeMetaBody.hashesNum = cnt;
        }
    }

    error:
    if (rval != UTIL_NO_ERROR) {
        CLEAR_OUTPUT_STRUCTURE(out, sizeof(timeMeta_t));
    }
    return rval;
}

/* parasoft-begin-suppress MISRAC2012-RULE_8_7-a-4 "External API." */
utilErr_t util_ParseUptRootMetadata(str_t const signedStr, const size_t length, util_json_t mem[], const uint16_t qty,
                                    rootMeta_t *const out) {
/* parasoft-end-suppress MISRAC2012-RULE_8_7-a-4 "External API." */
    utilErr_t rval = UTIL_NO_ERROR;
    jsonErr_t json_ret = JSON_NO_ERROR;
    json_t const *root = NULL;
    json_t const *obj = NULL;
    cstr_t out_str_val_ptr = NULL;


    // Check populated parameters
    if (NULL == signedStr) {
        DEBUG_PRINT("ERR: Input string is null.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (UINT_TWO > length) {
        DEBUG_PRINT("ERR: Length of target string is 0 or is less than 0.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (NULL == mem) {
        DEBUG_PRINT("ERR: Array of json properties to allocate is null\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (UINT_ZERO >= qty) {
        DEBUG_PRINT("ERR: Number of elements on array of json properties to allocate is 0 or is less than 0\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (NULL == out) {
        DEBUG_PRINT("ERR: Structure for storing parsed data is null\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    json_ret = json_create_ex(&root, signedStr, length, mem, qty);
    switch (json_ret) {
        case JSON_ERR_INVALID_PARAMETER:
            DEBUG_PRINT("ERR: Target string is not valid.\n");
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
            break;
        case JSON_ERR_NO_RESOURCE:
            DEBUG_PRINT("ERR: Array of json properties to allocate is too short for this JSON string.\n");
            rval = UTIL_ERR_NO_RESOURCE;
            goto error;
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }
    CLEAR_OUTPUT_STRUCTURE(out, sizeof(rootMeta_t));
    if (NULL != root) {

        COPY_UINT_VAL_FOR_MNDTRY_KEY(root, icd_key_version, out->version, uint32_t, UINT32_MAX)
        COPY_UINT_VAL_FOR_MNDTRY_KEY(root, icd_key_expires, out->expires, uint64_t, UINT64_MAX)
        GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(root, icd_key_type)
/* always evaluates to true if 'icd_mandatory' is used */
        if (LAST_SET_STR_VAL_PTR != NULL) {
            if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_root),
                             strlen(getDBstr(icd_key_root)))) {
                out->type = RT_ROOT;
            } else {
                out->type = RT_UNKNOWN;
                DEBUG_PRINT("ERR: Metadata Type mismatches expectation: %s \n", getDBstr(icd_key_root));
                rval = UTIL_ERR_INVALID_PARAMETER;
                goto error;
            }
        }

        const json_t *const body = json_getProperty(root, getDBstr(icd_key_body));
        ERROR_ON_NULL(body)
        if (JSON_OBJ != json_getType(body)) {
            DEBUG_PRINT("ERR: %s should be a JSON object\n", json_getName(body));
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
        }
        const json_t *const keys = json_getProperty(body, getDBstr(icd_key_keys));
        ERROR_ON_NULL(keys)
        if (JSON_ARRAY != json_getType(keys)) {
            DEBUG_PRINT("ERR: %s should be a JSON array\n", json_getName(keys));
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
        }

        uint8_t key_cnt = UINT_ZERO;
        const json_t *key = json_getChild(keys);
        while (NULL != key) {
            if ((MAX_TOTAL_KEY_NUM) > key_cnt) {
                if (JSON_OBJ != json_getType(key)) {
                    DEBUG_PRINT("ERR: %s should be a JSON object\n", json_getName(key));
                    rval = UTIL_ERR_INVALID_PARAMETER;
                    goto error;
                }

                GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(key, icd_key_publicKeyId)
                /* always evaluates to true if 'icd_mandatory' is used */
                if (NULL != LAST_SET_STR_VAL_PTR) {
                    if ((strnlength(LAST_SET_STR_VAL_PTR, ((SHA256_LENGTH * UINT_TWO) + UINT_ONE))) != (SHA256_LENGTH * UINT_TWO)) {
                        DEBUG_PRINT("ERR: Incorrect key_id input length\n");
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    }
                    const int32_t ret = abq_hex_decode(LAST_SET_STR_VAL_PTR,
                                                 (int32_t) (SHA256_LENGTH * UINT_TWO),
                                                 abq_ptr2raw(out->rootMetaBody.keys[key_cnt].id),
                                                 (int32_t) SHA256_LENGTH);
                    if (ret < SINT_ZERO) {
                        DEBUG_PRINT("ERR: Hex decode to binary failed\n");
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    }
                }
                GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(key, icd_key_publicKeyValue)
                /* always evaluates to true if 'icd_mandatory' is used */
                if (NULL != LAST_SET_STR_VAL_PTR) {
                    // get actual input length and allow +1 byte than target storage size
                    const size_t sz = strnlength(LAST_SET_STR_VAL_PTR, ((MAX_KEY_LENGTH * UINT_TWO) + UINT_ONE));
                    // will it fit into target storage area?
                    if (sz > (MAX_KEY_LENGTH * UINT_TWO)) {
                        DEBUG_PRINT("ERR: public key value input too large for target storage\n");
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    }
                    // input must be even number of characters
                    if ((sz % UINT_TWO) != UINT_ZERO) {
                        DEBUG_PRINT("ERR: public key value size/length is not multiple of 2\n");
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    }
                    out->rootMetaBody.keys[key_cnt].value_len = (sz / UINT_TWO);
                    const int32_t ret = abq_hex_decode(LAST_SET_STR_VAL_PTR,
                                                 (int32_t) sz,
                                                 abq_ptr2raw(out->rootMetaBody.keys[key_cnt].value),
                                                 (int32_t) MAX_KEY_LENGTH);
                    if (ret < SINT_ZERO) {
                        DEBUG_PRINT("ERR: Hex decode to binary failed\n");
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    }
                }

                GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(key, icd_key_publicKeyType)
                /* always evaluates to true if 'icd_mandatory' is used */
                if (LAST_SET_STR_VAL_PTR != NULL) {
                    if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_RSA),
                                     strlen(getDBstr(icd_key_RSA)))) {
                        out->rootMetaBody.keys[key_cnt].key_type = KT_RSA;
                    } else if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_ECDSA),
                                            strlen(getDBstr(icd_key_ECDSA)))) {
                        out->rootMetaBody.keys[key_cnt].key_type = KT_ECDSA;
                    } else {
                        out->rootMetaBody.keys[key_cnt].key_type = KT_UNKNOWN;
                        DEBUG_PRINT("ERR: Value mismatches expectation for: %s\n", getDBstr(icd_key_publicKeyType));
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    }
                }
            } else {
                DEBUG_PRINT("No resource for Key info.\n");
                rval = UTIL_ERR_NO_RESOURCE;
                goto error;
                break;
            }
            key = json_getSibling(key);
            key_cnt++;
        }
        out->rootMetaBody.keysNum = key_cnt;

        const json_t *const roles = json_getProperty(body, getDBstr(icd_key_roles));
        ERROR_ON_NULL(roles)
        if (JSON_ARRAY != json_getType(roles)) {
            DEBUG_PRINT("ERR: %s should be a JSON array\n", json_getName(roles));
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
        }

        uint8_t roles_cnt = UINT_ZERO;
        const json_t *role = json_getChild(roles);
        while (NULL != role) {
            if ((uint8_t) NUM_ROLES > roles_cnt) {
                if (JSON_OBJ != json_getType(role)) {
                    DEBUG_PRINT("ERR: %s should be a JSON object\n", json_getName(role));
                    rval = UTIL_ERR_INVALID_PARAMETER;
                    goto error;
                }
                COPY_UINT_VAL_FOR_MNDTRY_KEY(role, icd_key_threshold,
                                             out->rootMetaBody.roles[roles_cnt].threshold, uint32_t, UINT32_MAX)

                GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(role, icd_key_role)
                /* always evaluates to true if 'icd_mandatory' is used */
                if (LAST_SET_STR_VAL_PTR != NULL) {
                    if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_root),
                                     strlen(getDBstr(icd_key_root)))) {
                        out->rootMetaBody.roles[roles_cnt].role = RT_ROOT;
                    } else if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_snapshot),
                                            strlen(getDBstr(icd_key_snapshot)))) {
                        out->rootMetaBody.roles[roles_cnt].role = RT_SNAPSHOT;
                    } else if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_timestamp),
                                            strlen(getDBstr(icd_key_timestamp)))) {
                        out->rootMetaBody.roles[roles_cnt].role = RT_TIMESTAMP;
                    } else if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_targets),
                                            strlen(getDBstr(icd_key_targets)))) {
                        out->rootMetaBody.roles[roles_cnt].role = RT_TARGETS;
                    } else {
                        out->rootMetaBody.roles[roles_cnt].role = RT_UNKNOWN;
                        DEBUG_PRINT("ERR: Value mismatches expectation for: %s\n", getDBstr(icd_key_role));
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    }
                }

                const json_t *const ids = json_getProperty(role, getDBstr(icd_key_keyids));
                ERROR_ON_NULL(ids)
                if (JSON_ARRAY != json_getType(ids)) {
                    DEBUG_PRINT("ERR: %s should be a JSON array\n", json_getName(ids));
                    rval = UTIL_ERR_INVALID_PARAMETER;
                    goto error;
                }

                uint16_t ids_cnt = UINT_ZERO;
                const json_t *id = json_getChild(ids);
                while (NULL != id) {
                    if (MAX_KEY_NUM_PER_ROLE > ids_cnt) {
                        GET_TEXT_VALUE_PTR_FOR_OBJ(id)
                        if (NULL != LAST_SET_STR_VAL_PTR) {
                            if ((strnlength(LAST_SET_STR_VAL_PTR, ((SHA256_LENGTH * UINT_TWO) + UINT_ONE))) != (SHA256_LENGTH * UINT_TWO)) {
                                DEBUG_PRINT("ERR: Incorrect role id input length\n");
                                rval = UTIL_ERR_INVALID_PARAMETER;
                                goto error;
                            }
                            int32_t len = (int32_t)strnlength(LAST_SET_STR_VAL_PTR, SHA256_LENGTH * UINT_TWO);
                            int32_t ret = SINT_MINUS_ONE;
                            if (len <= (int32_t)(SHA256_LENGTH * UINT_TWO)) {
                                ret = abq_hex_decode(LAST_SET_STR_VAL_PTR, len, abq_ptr2raw(&out->rootMetaBody.roles[roles_cnt].ids[ids_cnt][SINT_ZERO]),
                                                     (int32_t) SHA256_LENGTH);
                            }
                            if (ret < SINT_ZERO) {
                                DEBUG_PRINT("ERR: Hex decode to binary failed\n");
                                rval = UTIL_ERR_INVALID_PARAMETER;
                                goto error;
                            }
                        }
                    } else {
                        DEBUG_PRINT("No resource for role info.\n");
                        rval = UTIL_ERR_NO_RESOURCE;
                        goto error;
                        break;
                    }
                    id = json_getSibling(id);
                    ids_cnt++;
                }
                out->rootMetaBody.roles[roles_cnt].idsNum = ids_cnt;
            } else {
                DEBUG_PRINT("No resource for roles info.\n");
                rval = UTIL_ERR_NO_RESOURCE;

                break;
            }
            role = json_getSibling(role);
            roles_cnt++;
        }
        out->rootMetaBody.rolesNum = roles_cnt;
    }

    error:
    if (rval != UTIL_NO_ERROR) {
        CLEAR_OUTPUT_STRUCTURE(out, sizeof(rootMeta_t));
    }
    return rval;
}


utilErr_t util_ParseUptSnapshotMetadata(str_t const signedStr,
                                        const size_t length,
                                        util_json_t mem[],
                                        const uint16_t qty,
                                        snapshotMeta_t *const out) {

    utilErr_t rval = UTIL_NO_ERROR;
    jsonErr_t json_ret = JSON_NO_ERROR;
    json_t const *root = NULL;
    json_t const *obj = NULL;
    cstr_t out_str_val_ptr = NULL;


    // Check populated parameters
    if (NULL == signedStr) {
        DEBUG_PRINT("ERR: Input string is null.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (UINT_TWO > length) {
        DEBUG_PRINT("ERR: Length of target string is 0 or is less than 0.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (NULL == mem) {
        DEBUG_PRINT("ERR: Array of json properties to allocate is null\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (UINT_ZERO >= qty) {
        DEBUG_PRINT("ERR: Number of elements on array of json properties to allocate is 0 or is less than 0\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (NULL == out) {
        DEBUG_PRINT("ERR: Structure for storing parsed data is null\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    json_ret = json_create_ex(&root, signedStr, length, mem, qty);
    switch (json_ret) {
        case JSON_ERR_INVALID_PARAMETER:
            DEBUG_PRINT("ERR: Target string is not valid.\n");
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
            break;
        case JSON_ERR_NO_RESOURCE:
            DEBUG_PRINT("ERR: Array of json properties to allocate is too short for this JSON string.\n");
            rval = UTIL_ERR_NO_RESOURCE;
            goto error;
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }
    CLEAR_OUTPUT_STRUCTURE(out, sizeof(snapshotMeta_t));
    if (NULL != root) {

        COPY_UINT_VAL_FOR_MNDTRY_KEY(root, icd_key_version, out->version, uint32_t, UINT32_MAX)
        COPY_UINT_VAL_FOR_MNDTRY_KEY(root, icd_key_expires, out->expires, uint64_t, UINT64_MAX)
        GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(root, icd_key_type)
/* always evaluates to true if 'icd_mandatory' is used */
        if (LAST_SET_STR_VAL_PTR != NULL) {
            if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_snapshot),
                             strlen(getDBstr(icd_key_snapshot)))) {
                out->type = RT_SNAPSHOT;
            } else {
                out->type = RT_UNKNOWN;
                DEBUG_PRINT("ERR: Metadata Type mismatches expectation: %s \n", getDBstr(icd_key_snapshot));
                rval = UTIL_ERR_INVALID_PARAMETER;
                goto error;
            }
        }

        const json_t *const body = json_getProperty(root, getDBstr(icd_key_body));
        ERROR_ON_NULL(body)
        if (JSON_OBJ != json_getType(body)) {
            DEBUG_PRINT("ERR: %s should be a JSON object\n", json_getName(body));
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
        }

        const json_t *const snapMetaFiles = json_getProperty(body, getDBstr(icd_key_snapshotMetadataFiles));
        ERROR_ON_NULL(snapMetaFiles)
        if (JSON_ARRAY != json_getType(snapMetaFiles)) {
            DEBUG_PRINT("ERR: %s should be a JSON array\n", json_getName(snapMetaFiles));
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
        }
        uint16_t cnt = UINT_ZERO;
        const json_t *snapMeta = json_getChild(snapMetaFiles);
        while (NULL != snapMeta) {
            if (MAX_SNAPSHOT_META_FILES_NUM > cnt) {
                if (JSON_OBJ != json_getType(snapMeta)) {
                    DEBUG_PRINT("ERR: %s not a JSON object\n", json_getName(snapMeta));
                    rval = UTIL_ERR_INVALID_PARAMETER;
                    goto error;
                }

                COPY_TEXT_VAL_FOR_MNDTRY_KEY(snapMeta, icd_key_filename,
                                             out->snapshotMetaBody.snapshotMetaFiles->fileName,
                                             MAX_FILE_NAME_SZ)

                COPY_UINT_VAL_FOR_MNDTRY_KEY(snapMeta, icd_key_version,
                                             out->snapshotMetaBody.snapshotMetaFiles->version, uint32_t, UINT32_MAX)

            } else {
                DEBUG_PRINT("No resource for hash info.\n");
                rval = UTIL_ERR_NO_RESOURCE;
                goto error;
                break;
            }
            snapMeta = json_getSibling(snapMeta);
            cnt++;
        }
        if (rval == UTIL_NO_ERROR) {
            out->snapshotMetaBody.snapshotNum = cnt;
        }
    }

    error:
    if (rval != UTIL_NO_ERROR) {
        CLEAR_OUTPUT_STRUCTURE(out, sizeof(snapshotMeta_t));
    }
    return rval;
}

utilErr_t util_ParseUptTargetsMetadata(str_t const signedStr,
                                       const size_t length,
                                       util_json_t mem[],
                                       const uint16_t qty,
                                       targetsMeta_t *const out) {

    utilErr_t rval = UTIL_NO_ERROR;
    jsonErr_t json_ret = JSON_NO_ERROR;
    json_t const *root = NULL;
    json_t const *obj = NULL;
    cstr_t out_str_val_ptr = NULL;


    // Check populated parameters
    if (NULL == signedStr) {
        DEBUG_PRINT("ERR: Input string is null.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (UINT_TWO > length) {
        DEBUG_PRINT("ERR: Length of target string is 0 or is less than 0.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (NULL == mem) {
        DEBUG_PRINT("ERR: Array of json properties to allocate is null\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (UINT_ZERO >= qty) {
        DEBUG_PRINT("ERR: Number of elements on array of json properties to allocate is 0 or is less than 0\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (NULL == out) {
        DEBUG_PRINT("ERR: Structure for storing parsed data is null\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    json_ret = json_create_ex(&root, signedStr, length, mem, qty);
    switch (json_ret) {
        case JSON_ERR_INVALID_PARAMETER:
            DEBUG_PRINT("ERR: Target string is not valid.\n");
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
            break;
        case JSON_ERR_NO_RESOURCE:
            printf("JSON_ERR_NO_RESOURCE: ctx->qty overflow (%s:%d)\n", __func__, __LINE__);
            printf("ERR: Array of json properties to allocate is too short for this JSON string.\n");
            rval = UTIL_ERR_NO_RESOURCE;
            goto error;
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }
    CLEAR_OUTPUT_STRUCTURE(out, sizeof(targetsMeta_t));
    if (NULL != root) {
        COPY_UINT_VAL_FOR_MNDTRY_KEY(root, icd_key_version, out->version, uint32_t, UINT32_MAX)
        COPY_UINT_VAL_FOR_MNDTRY_KEY(root, icd_key_expires, out->expires, uint64_t, UINT64_MAX)
        GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(root, icd_key_type)
        /* always evaluates to true if 'icd_mandatory' is used */
        if (LAST_SET_STR_VAL_PTR != NULL) {
            if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_targets),
                             strlen(getDBstr(icd_key_targets)))) {
                out->type = RT_TARGETS;
            } else {
                out->type = RT_UNKNOWN;
                DEBUG_PRINT("ERR: Metadata Type mismatches expectation: %s \n", getDBstr(icd_key_targets));
                rval = UTIL_ERR_INVALID_PARAMETER;
                goto error;
            }
        }

        const json_t *const body = json_getProperty(root, getDBstr(icd_key_body));
        ERROR_ON_NULL(body)
        if (JSON_OBJ != json_getType(body)) {
            DEBUG_PRINT("ERR: %s should be a JSON object\n", json_getName(body));
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
        }

        const json_t *const targetCustom = json_getProperty(body, getDBstr(icd_key_targets));
        ERROR_ON_NULL(targetCustom)
        if (JSON_ARRAY != json_getType(targetCustom)) {
            DEBUG_PRINT("ERR: %s should be a JSON array\n", json_getName(targetCustom));
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
        }
        uint16_t cnt = UINT_ZERO;
        const json_t *tarCus = json_getChild(targetCustom);
        while (NULL != tarCus) {
            if (MAX_TARGET_AND_CUSTOM_NUM > cnt) {
                if (JSON_OBJ != json_getType(tarCus)) {
                    DEBUG_PRINT("ERR: %s not a JSON object\n", json_getName(tarCus));
                    rval = UTIL_ERR_INVALID_PARAMETER;
                    goto error;
                }

                const json_t *const target = json_getProperty(tarCus, getDBstr(icd_key_target));
                ERROR_ON_NULL(target)
                if (JSON_OBJ != json_getType(target)) {
                    DEBUG_PRINT("ERR: %s should be a JSON object\n", json_getName(target));
                    rval = UTIL_ERR_INVALID_PARAMETER;
                    goto error;
                }
                COPY_TEXT_VAL_FOR_MNDTRY_KEY(target, icd_key_filename, out->targetsMetaBody.targets[cnt].target.fileName, MAX_FILE_NAME_SZ)
                COPY_UINT_VAL_FOR_MNDTRY_KEY(target, icd_key_length, out->targetsMetaBody.targets[cnt].target.length, uint32_t, UINT32_MAX)
                COPY_TEXT_VAL_FOR_OPTNL_KEY(target, icd_key_fileDownloadUrl, out->targetsMetaBody.targets[cnt].target.fileDownloadUrl, MAX_DWNLD_URL_SZ)

                const json_t *hashes = json_getProperty(target, "hashes");
                ERROR_ON_NULL(hashes)
                if (JSON_ARRAY != json_getType(hashes)) {
                    DEBUG_PRINT("ERR: %s should be a JSON array\n", json_getName(hashes));
                    rval = UTIL_ERR_INVALID_PARAMETER;
                    goto error;
                }
                uint16_t hash_cnt = UINT_ZERO;
                const json_t *hash = json_getChild(hashes);
                while (NULL != hash) {
                    if (MAX_HASH_NUM > hash_cnt) {
                        if (JSON_OBJ != json_getType(hash)) {
                            DEBUG_PRINT("ERR: %s should be a JSON object\n", json_getName(hash));
                            rval = UTIL_ERR_INVALID_PARAMETER;
                            goto error;
                        }

                        GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(hash, icd_key_digest)
                        /* always evaluates to true if 'icd_mandatory' is used */
                        if (NULL != LAST_SET_STR_VAL_PTR) {
                            if ((strnlength(LAST_SET_STR_VAL_PTR, ((SHA256_LENGTH * UINT_TWO) + UINT_ONE))) != (SHA256_LENGTH * UINT_TWO)) {
                                DEBUG_PRINT("ERR: Incorrect digest input length\n");
                                rval = UTIL_ERR_INVALID_PARAMETER;
                                goto error;
                            }
                            const int32_t ret = abq_hex_decode(LAST_SET_STR_VAL_PTR,
                                                         (int32_t)(SHA256_LENGTH * UINT_TWO),
                                                         abq_ptr2raw(
                                                                 out->targetsMetaBody.targets[cnt].target.hashes[hash_cnt].digest),
                                                         (int32_t) SHA256_LENGTH);
                            if (ret < SINT_ZERO) {
                                DEBUG_PRINT("ERR: Hex decode to binary failed\n");
                                rval = UTIL_ERR_INVALID_PARAMETER;
                                goto error;
                            }
                        }

                        GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(hash, icd_key_function)
                        /* always evaluates to true if 'icd_mandatory' is used */
                        if (LAST_SET_STR_VAL_PTR != NULL) {
                            if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_SHA256),
                                             strlen(getDBstr(icd_key_SHA256)))) {
                                out->targetsMetaBody.targets[cnt].target.hashes[hash_cnt].funcType = FT_SHA256;
                            } else {
                                out->targetsMetaBody.targets[cnt].target.hashes[hash_cnt].funcType = FT_UNKNOWN;
                                DEBUG_PRINT("ERR: Value mismatches expectation for: %s\n", getDBstr(icd_key_function));
                                rval = UTIL_ERR_INVALID_PARAMETER;
                                goto error;
                            }
                        }

                    } else {
                        DEBUG_PRINT("No resource for hash info.\n");
                        rval = UTIL_ERR_NO_RESOURCE;
                        goto error;
                        break;
                    }
                    hash = json_getSibling(hash);
                    hash_cnt++;
                }
                out->targetsMetaBody.targets[cnt].target.hashesNum = hash_cnt;

                const json_t *const custom = json_getProperty(tarCus, getDBstr(icd_key_custom));
                if (NULL != custom) {
                    if (JSON_OBJ != json_getType(custom)) {
                        DEBUG_PRINT("ERR: %s should be a JSON object\n", json_getName(custom));
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    }

                    /* The reportId is only available in director repository during runtime when relating
                     * a target to a change_event is necessary.
                     * The reportId is not included in the image repository since it is a static database
                     * only concerning which image exists
                     */
                    COPY_TEXT_VAL_FOR_OPTNL_KEY(custom, icd_key_reportId,
                                                out->targetsMetaBody.targets[cnt].custom.reportId,
                                                MAX_REPORT_ID_SZ)

                    COPY_UINT_VAL_FOR_OPTNL_KEY(custom, icd_key_releaseCounter,
                                                out->targetsMetaBody.targets[cnt].custom.releaseCounter, uint32_t, UINT32_MAX)

                    COPY_TEXT_VAL_FOR_OPTNL_KEY(custom, icd_key_hardwareIdentifier,
                                                out->targetsMetaBody.targets[cnt].custom.hardwareIdentifier,
                                                MAX_PART_NUM_SZ)

                    COPY_TEXT_VAL_FOR_OPTNL_KEY(custom, icd_key_ecuIdentifier,
                                                out->targetsMetaBody.targets[cnt].custom.ecuIdentifier,
                                                MAX_SERIAL_NUM_SZ)

                    const json_t *const encryptedTarget = json_getProperty(custom, getDBstr(icd_key_encryptedTarget));
                    if (NULL != encryptedTarget) {
                        if (JSON_OBJ != json_getType(encryptedTarget)) {
                            DEBUG_PRINT("ERR: %s should be a JSON object\n", json_getName(encryptedTarget));
                            rval = UTIL_ERR_INVALID_PARAMETER;
                            goto error;
                        }
                        COPY_TEXT_VAL_FOR_MNDTRY_KEY(encryptedTarget, icd_key_filename, out->targetsMetaBody.targets[cnt].custom.encryptedTarget.fileName, MAX_FILE_NAME_SZ)
                        COPY_UINT_VAL_FOR_MNDTRY_KEY(encryptedTarget, icd_key_length, out->targetsMetaBody.targets[cnt].custom.encryptedTarget.length, uint32_t, UINT32_MAX)
                        COPY_TEXT_VAL_FOR_OPTNL_KEY(encryptedTarget, icd_key_fileDownloadUrl, out->targetsMetaBody.targets[cnt].custom.encryptedTarget.fileDownloadUrl, MAX_DWNLD_URL_SZ)

                        hashes = json_getProperty(encryptedTarget, "hashes");
                        ERROR_ON_NULL(hashes)
                        if (JSON_ARRAY != json_getType(hashes)) {
                            DEBUG_PRINT("ERR: %s should be a JSON array\n", json_getName(hashes));
                            rval = UTIL_ERR_INVALID_PARAMETER;
                            goto error;
                        }
                        hash_cnt = UINT_ZERO;
                        hash = json_getChild(hashes);
                        while (NULL != hash) {
                            if (MAX_HASH_NUM > hash_cnt) {
                                if (JSON_OBJ != json_getType(hash)) {
                                    DEBUG_PRINT("ERR: %s should be a JSON object\n", json_getName(hash));
                                    rval = UTIL_ERR_INVALID_PARAMETER;
                                    goto error;
                                }

                                GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(hash, icd_key_digest)
                                /* always evaluates to true if 'icd_mandatory' is used */
                                if (NULL != LAST_SET_STR_VAL_PTR) {
                                    if ((strnlength(LAST_SET_STR_VAL_PTR, ((SHA256_LENGTH * UINT_TWO) + UINT_ONE))) != (SHA256_LENGTH * UINT_TWO)) {
                                        DEBUG_PRINT("ERR: Incorrect digest input length\n");
                                        rval = UTIL_ERR_INVALID_PARAMETER;
                                        goto error;
                                    }
                                    const int32_t ret = abq_hex_decode(LAST_SET_STR_VAL_PTR,
                                                                       (int32_t) (SHA256_LENGTH * UINT_TWO),
                                                                       abq_ptr2raw(
                                                                               out->targetsMetaBody.targets[cnt].custom.encryptedTarget.hashes[hash_cnt].digest),
                                                                       (int32_t) SHA256_LENGTH);
                                    if (ret < SINT_ZERO) {
                                        DEBUG_PRINT("ERR: Hex decode to binary failed\n");
                                        rval = UTIL_ERR_INVALID_PARAMETER;
                                        goto error;
                                    }
                                }

                                GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(hash, icd_key_function)
                                /* always evaluates to true if 'icd_mandatory' is used */
                                if (LAST_SET_STR_VAL_PTR != NULL) {
                                    if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_SHA256),
                                                             strlen(getDBstr(icd_key_SHA256)))) {
                                        out->targetsMetaBody.targets[cnt].custom.encryptedTarget.hashes[hash_cnt].funcType = FT_SHA256;
                                    } else {
                                        out->targetsMetaBody.targets[cnt].custom.encryptedTarget.hashes[hash_cnt].funcType = FT_UNKNOWN;
                                        DEBUG_PRINT("ERR: Value mismatches expectation for: %s\n", getDBstr(icd_key_function));
                                        rval = UTIL_ERR_INVALID_PARAMETER;
                                        goto error;
                                    }
                                }

                            } else {
                                DEBUG_PRINT("No resource for hash info.\n");
                                rval = UTIL_ERR_NO_RESOURCE;
                                goto error;
                                break;
                            }
                            hash = json_getSibling(hash);
                            hash_cnt++;
                        }
                        out->targetsMetaBody.targets[cnt].custom.encryptedTarget.hashesNum = hash_cnt;
                    }

                    const json_t *const deltaTarget = json_getProperty(custom, getDBstr(icd_key_deltaTarget));
                    if (NULL != deltaTarget) {
                        if (JSON_OBJ != json_getType(deltaTarget)) {
                            DEBUG_PRINT("ERR: %s should be a JSON object\n", json_getName(deltaTarget));
                            rval = UTIL_ERR_INVALID_PARAMETER;
                            goto error;
                        }
                        COPY_TEXT_VAL_FOR_MNDTRY_KEY(deltaTarget, icd_key_filename, out->targetsMetaBody.targets[cnt].custom.deltaTarget.fileName, MAX_FILE_NAME_SZ)
                        COPY_UINT_VAL_FOR_MNDTRY_KEY(deltaTarget, icd_key_length, out->targetsMetaBody.targets[cnt].custom.deltaTarget.length, uint32_t, UINT32_MAX)
                        COPY_TEXT_VAL_FOR_OPTNL_KEY(deltaTarget, icd_key_fileDownloadUrl, out->targetsMetaBody.targets[cnt].custom.deltaTarget.fileDownloadUrl, MAX_DWNLD_URL_SZ)

                        hashes = json_getProperty(deltaTarget, "hashes");
                        ERROR_ON_NULL(hashes)
                        if (JSON_ARRAY != json_getType(hashes)) {
                            DEBUG_PRINT("ERR: %s should be a JSON array\n", json_getName(hashes));
                            rval = UTIL_ERR_INVALID_PARAMETER;
                            goto error;
                        }
                        hash_cnt = UINT_ZERO;
                        hash = json_getChild(hashes);
                        while (NULL != hash) {
                            if (MAX_HASH_NUM > hash_cnt) {
                                if (JSON_OBJ != json_getType(hash)) {
                                    DEBUG_PRINT("ERR: %s should be a JSON object\n", json_getName(hash));
                                    rval = UTIL_ERR_INVALID_PARAMETER;
                                    goto error;
                                }

                                GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(hash, icd_key_digest)
                                /* always evaluates to true if 'icd_mandatory' is used */
                                if (NULL != LAST_SET_STR_VAL_PTR) {
                                    if ((strnlength(LAST_SET_STR_VAL_PTR, ((SHA256_LENGTH * UINT_TWO) + UINT_ONE))) != (SHA256_LENGTH * UINT_TWO)) {
                                        DEBUG_PRINT("ERR: Incorrect digest input length\n");
                                        rval = UTIL_ERR_INVALID_PARAMETER;
                                        goto error;
                                    }
                                    const int32_t ret = abq_hex_decode(LAST_SET_STR_VAL_PTR,
                                                                       (int32_t) (SHA256_LENGTH * UINT_TWO),
                                                                       abq_ptr2raw(
                                                                               out->targetsMetaBody.targets[cnt].custom.deltaTarget.hashes[hash_cnt].digest),
                                                                       (int32_t) SHA256_LENGTH);
                                    if (ret < SINT_ZERO) {
                                        DEBUG_PRINT("ERR: Hex decode to binary failed\n");
                                        rval = UTIL_ERR_INVALID_PARAMETER;
                                        goto error;
                                    }
                                }

                                GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(hash, icd_key_function)
                                /* always evaluates to true if 'icd_mandatory' is used */
                                if (LAST_SET_STR_VAL_PTR != NULL) {
                                    if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_SHA256),
                                                             strlen(getDBstr(icd_key_SHA256)))) {
                                        out->targetsMetaBody.targets[cnt].custom.deltaTarget.hashes[hash_cnt].funcType = FT_SHA256;
                                    } else {
                                        out->targetsMetaBody.targets[cnt].custom.deltaTarget.hashes[hash_cnt].funcType = FT_UNKNOWN;
                                        DEBUG_PRINT("ERR: Value mismatches expectation for: %s\n", getDBstr(icd_key_function));
                                        rval = UTIL_ERR_INVALID_PARAMETER;
                                        goto error;
                                    }
                                }

                            } else {
                                DEBUG_PRINT("No resource for hash info.\n");
                                rval = UTIL_ERR_NO_RESOURCE;
                                goto error;
                                break;
                            }
                            hash = json_getSibling(hash);
                            hash_cnt++;
                        }
                        if (rval == UTIL_NO_ERROR) {
                            out->targetsMetaBody.targets[cnt].custom.deltaTarget.hashesNum = hash_cnt;
                        }
                    }

                    const json_t *const ecryptdSymKey = json_getProperty(custom, getDBstr(icd_key_encryptedSymmetricKey));
                    /* encryptedSymmetricKey is optional in the ICD */
                    if (NULL != ecryptdSymKey) {
                        if (JSON_OBJ != json_getType(ecryptdSymKey)) {
                            DEBUG_PRINT("ERR: %s should be a JSON object\n", json_getName(ecryptdSymKey));
                            rval = UTIL_ERR_INVALID_PARAMETER;
                            goto error;
                        }

                        GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(ecryptdSymKey, icd_key_encryptedSymmetricKeyType)
                        /* always evaluates to true if 'icd_mandatory' is used */
                        if (LAST_SET_STR_VAL_PTR != NULL) {
                            if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) "AES", strlen("AES"))) {
                                out->targetsMetaBody.targets[cnt].custom.ecryptdSymKey.ecryptdSymKeyType = ECRYPTD_SYMKEY_TYPE_AES;
                            } else if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) "DES", strlen("DES"))) {
                                out->targetsMetaBody.targets[cnt].custom.ecryptdSymKey.ecryptdSymKeyType = ECRYPTD_SYMKEY_TYPE_DES;
                            } else {
                                out->targetsMetaBody.targets[cnt].custom.ecryptdSymKey.ecryptdSymKeyType = ECRYPTD_SYMKEY_TYPE_UNKNOWN;
                                DEBUG_PRINT("ERR: Value mismatches expectation for: %s \n",
                                            getDBstr(icd_key_encryptedSymmetricKeyType));
                                rval = UTIL_ERR_INVALID_PARAMETER;
                                goto error;
                            }
                        }
                        COPY_TEXT_VAL_FOR_MNDTRY_KEY(ecryptdSymKey, icd_key_encryptedSymmetricKeyValue,
                                                     out->targetsMetaBody.targets[cnt].custom.ecryptdSymKey.ecryptdSymKeyValue,
                                                     MAX_ECRYPTD_SYMKEY_VALUE_SZ)

                        GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(ecryptdSymKey, icd_key_encryptedSymmetricKeyAlgorithmMode)
                        /* always evaluates to true if 'icd_mandatory' is used */
                        if (LAST_SET_STR_VAL_PTR != NULL) {
                            if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) "CBC", strlen("CBC"))) {
                                out->targetsMetaBody.targets[cnt].custom.ecryptdSymKey.ecryptdSymKeyAlgMode = ECRYPTD_SYMKEY_ALG_MODE_CBC;
                            } else {
                                out->targetsMetaBody.targets[cnt].custom.ecryptdSymKey.ecryptdSymKeyAlgMode = ECRYPTD_SYMKEY_ALG_MODE_UNKNOWN;
                                DEBUG_PRINT("ERR: Value mismatches expectation for: %s \n",
                                            getDBstr(icd_key_encryptedSymmetricKeyValue));
                                rval = UTIL_ERR_INVALID_PARAMETER;
                                goto error;
                            }
                        }

                        GET_TEXT_VAL_PTR_FOR_OPTNL_KEY(ecryptdSymKey, icd_key_paddingScheme)
                        /* always evaluates to true if 'icd_mandatory' is used */
                        if (LAST_SET_STR_VAL_PTR != NULL) {
                            if (SINT_ZERO ==
                                memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) "PKCS5Padding", strlen("PKCS5Padding"))) {
                                out->targetsMetaBody.targets[cnt].custom.ecryptdSymKey.paddingScheme = PADDING_SCHEME_PKCS5;
                            } else {
                                out->targetsMetaBody.targets[cnt].custom.ecryptdSymKey.paddingScheme = PADDING_SCHEME_UNKNOWN;
                            }
                        }
                        COPY_TEXT_VAL_FOR_OPTNL_KEY(ecryptdSymKey, icd_key_iv,
                                                    out->targetsMetaBody.targets[cnt].custom.ecryptdSymKey.iv,
                                                    MAX_BASE64_IV_SZ)
                    }
                }
            } else {
                printf("Number of elements has exceeded MAX_TARGET_AND_CUSTOM_NUM (%s:%d)\n", __func__, __LINE__);
                printf("No resource for targets and custom\n");
                rval = UTIL_ERR_NO_RESOURCE;
                goto error;
                break;
            }
            tarCus = json_getSibling(tarCus);
            cnt++;
        }
        if (rval == UTIL_NO_ERROR) {
            out->targetsMetaBody.targetsNum = cnt;
        }
    }

    error:
    if (rval != UTIL_NO_ERROR) {
        CLEAR_OUTPUT_STRUCTURE(out, sizeof(targetsMeta_t));
    }
    return rval;
}
/* parasoft-begin-suppress MISRAC2012-DIR_4_9-a-4 CERT_C-PRE00-a-3 "Function like macro is used in favor of function in oder to maintain code readability" */
#define POLICY_MAPPING(out) \
if (SINT_ZERO == memcomp((cstr_t)LAST_SET_STR_VAL_PTR, (cstr_t)getDBstr(icd_key_active), strlen(getDBstr(icd_key_active)))) {  \
(out) = APPROVAL_MODE_ACTIVE; \
} else if (SINT_ZERO == memcomp((cstr_t)LAST_SET_STR_VAL_PTR, (cstr_t)getDBstr(icd_key_passive), strlen(getDBstr(icd_key_passive)))) { \
(out) = APPROVAL_MODE_PASSIVE; \
} else if (SINT_ZERO == memcomp((cstr_t)LAST_SET_STR_VAL_PTR, (cstr_t)getDBstr(icd_key_user_preference), strlen(getDBstr(icd_key_user_preference)))) { \
(out) = APPROVAL_MODE_USER_PREF; }  \
else { (out) = APPROVAL_MODE_UNKNOWN; }

/* parasoft-end-suppress MISRAC2012-DIR_4_9-a-4 CERT_C-PRE00-a-3*/

utilErr_t util_ParseUptAugmentedMetadata(str_t const signedStr,
                                         const size_t length,
                                         util_json_t mem[],
                                         const uint16_t qty,
                                         augMeta_t *const out) {

    utilErr_t rval = UTIL_NO_ERROR;
    jsonErr_t json_ret = JSON_NO_ERROR;
    json_t const *root = NULL;
    json_t const *obj = NULL;
    cstr_t out_str_val_ptr = NULL;

    // Check populated parameters
    if (NULL == signedStr) {
        DEBUG_PRINT("ERR: Input string is null.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (UINT_TWO > length) {
        DEBUG_PRINT("ERR: Length of target string is 0 or is less than 0.\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (NULL == mem) {
        DEBUG_PRINT("ERR: Array of json properties to allocate is null\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (UINT_ZERO >= qty) {
        DEBUG_PRINT("ERR: Number of elements on array of json properties to allocate is 0 or is less than 0\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }
    if (NULL == out) {
        DEBUG_PRINT("ERR: Structure for storing parsed data is null\n");
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    }

    json_ret = json_create_ex(&root, signedStr, length, mem, qty);
    switch (json_ret) {
        case JSON_ERR_INVALID_PARAMETER:
            DEBUG_PRINT("ERR: Target string is not valid.\n");
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
            break;
        case JSON_ERR_NO_RESOURCE:
            DEBUG_PRINT("ERR: Array of json properties to allocate is too short for this JSON string.\n");
            rval = UTIL_ERR_NO_RESOURCE;
            goto error;
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }
    CLEAR_OUTPUT_STRUCTURE(out, sizeof(augMeta_t));
    if (NULL != root) {

        COPY_UINT_VAL_FOR_MNDTRY_KEY(root, icd_key_version, out->version, uint32_t, UINT32_MAX)
        COPY_UINT_VAL_FOR_MNDTRY_KEY(root, icd_key_expires, out->expires, uint64_t, UINT64_MAX)
        GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(root, icd_key_type)
        /* always evaluates to true if 'icd_mandatory' is used */
        if (LAST_SET_STR_VAL_PTR != NULL) {
            if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_targets),
                             strlen(getDBstr(icd_key_targets)))) {
                out->type = RT_TARGETS;
            } else {
                out->type = RT_UNKNOWN;
                DEBUG_PRINT("ERR: Metadata Type mismatches expectation: %s", getDBstr(icd_key_targets));
                rval = UTIL_ERR_INVALID_PARAMETER;
                goto error;
            }
        }

        const json_t *const body = json_getProperty(root, getDBstr(icd_key_body));
        ERROR_ON_NULL(body)
        if (JSON_OBJ != json_getType(body)) {
            DEBUG_PRINT("ERR: %s should be a JSON object\n", json_getName(body));
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
        }

        GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(body, icd_key_status)
        /* always evaluates to true if 'icd_mandatory' is used */
        if (LAST_SET_STR_VAL_PTR != NULL) {
            if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_OTA_DISABLED),
                             strlen(getDBstr(icd_key_OTA_DISABLED)))) {
                out->augMetaBody.status = CONFIG_CHECK_STATUS_OTA_DISABLED;
            } else if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_RE_SYNC),
                                    strlen(getDBstr(icd_key_RE_SYNC)))) {
                out->augMetaBody.status = CONFIG_CHECK_STATUS_RE_SYNC;
            } else if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_STALE_REQUEST),
                                    strlen(getDBstr(icd_key_STALE_REQUEST)))) {
                out->augMetaBody.status = CONFIG_CHECK_STATUS_STALE_REQUEST;
            } else if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_NON_STD_CONFIG),
                                    strlen(getDBstr(icd_key_NON_STD_CONFIG)))) {
                out->augMetaBody.status = CONFIG_CHECK_STATUS_NON_STD_CONFIG;
            } else if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_NO_UPDATE),
                                    strlen(getDBstr(icd_key_NO_UPDATE)))) {
                out->augMetaBody.status = CONFIG_CHECK_STATUS_NO_UPDATE;
            } else if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_UPDATE),
                                    strlen(getDBstr(icd_key_UPDATE)))) {
                out->augMetaBody.status = CONFIG_CHECK_STATUS_UPDATE;
            } else {
                out->augMetaBody.status = CONFIG_CHECK_STATUS_UNKNOWN;
                DEBUG_PRINT("ERR: Unexpected Uptane status cannot be mapped: %s", LAST_SET_STR_VAL_PTR);
                rval = UTIL_ERR_INVALID_PARAMETER;
                goto error;
            }
        }

        const json_t *const subs = json_getProperty(body, getDBstr(icd_key_subscriptionInfo));
        ERROR_ON_NULL(subs)
        if (JSON_OBJ != json_getType(subs)) {
            DEBUG_PRINT("ERR: %s not a JSON object\n", json_getName(subs));
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
        }
        COPY_UINT_VAL_FOR_MNDTRY_KEY(subs, icd_key_statusCheckInterval,
                                     out->augMetaBody.subscInfo.statusCheckInterval, uint32_t, UINT32_MAX)

        COPY_BOOL_VAL_FOR_MNDTRY_KEY(subs, icd_key_otaEnabled,
                                     out->augMetaBody.subscInfo.otaEnabled)

        const json_t *const campaigns = json_getProperty(body, "campaigns");
        ERROR_ON_NULL(campaigns)
        if (JSON_ARRAY != json_getType(campaigns)) {
            DEBUG_PRINT("ERR: %s not a JSON array\n", json_getName(campaigns));
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
        }

        uint16_t cnt = UINT_ZERO;
        const json_t *campaign = json_getChild(campaigns);
        while (NULL != campaign) {
            if (MAX_CAMPAIGNS > cnt) {
                if (JSON_OBJ != json_getType(campaign)) {
                    DEBUG_PRINT("ERR: %s not a JSON object\n", json_getName(campaign));
                    rval = UTIL_ERR_INVALID_PARAMETER;
                    goto error;
                }
                COPY_TEXT_VAL_FOR_MNDTRY_KEY(campaign, icd_key_campaignId, out->augMetaBody.campaigns[cnt].campaignId,
                                             MAX_CAMPAIGN_ID_SZ)

                GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(campaign, icd_key_campaignType)
                /* always evaluates to true if 'icd_mandatory' is used */
                if (LAST_SET_STR_VAL_PTR != NULL) {
                    if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_OTA),
                                     strlen(getDBstr(icd_key_OTA)))) {
                        out->augMetaBody.campaigns[cnt].campaignType = CMP_TYPE_OTA;
                    } else if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_INFORMATIONAL),
                                            strlen(getDBstr(icd_key_INFORMATIONAL)))) {
                        out->augMetaBody.campaigns[cnt].campaignType = CMP_TYPE_INFO;
                    } else {
                        out->augMetaBody.campaigns[cnt].campaignType = CMP_TYPE_UNKNOWN;
                        DEBUG_PRINT("ERR: Value mismatches expectation for: %s\n", getDBstr(icd_key_campaignType));
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    }
                }

                const json_t *const policy = json_getProperty(campaign, getDBstr(icd_key_policy));
                ERROR_ON_NULL(policy)
                if (JSON_OBJ != json_getType(policy)) {
                    DEBUG_PRINT("ERR: %s not a JSON object\n", json_getName(policy));
                    rval = UTIL_ERR_INVALID_PARAMETER;
                    goto error;
                }

                /* policy.campaignApproval */
                GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(policy, icd_key_campaignApproval)
                /* always evaluates to true if 'icd_mandatory' is used */
                if (LAST_SET_STR_VAL_PTR != NULL) {
                    POLICY_MAPPING(out->augMetaBody.campaigns[cnt].policy.campaignApproval)
                    // Failed to map to known value?
                    if(out->augMetaBody.campaigns[cnt].policy.campaignApproval == APPROVAL_MODE_UNKNOWN){
                        DEBUG_PRINT("ERR: Value mismatches expectation for: %s\n", getDBstr(icd_key_campaignApproval));
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    }
                } else {
                    DEBUG_PRINT("ERR: Missing key: %s\n", getDBstr((icd_key_campaignApproval)));\
                    rval = UTIL_ERR_INVALID_PARAMETER;
                    goto error;
                }

                /* policy.downloadApproval */
                GET_TEXT_VAL_PTR_FOR_OPTNL_KEY(policy, icd_key_downloadApproval)
                /* always evaluates to true if 'icd_mandatory' is used */
                if (LAST_SET_STR_VAL_PTR != NULL) {
                    POLICY_MAPPING(out->augMetaBody.campaigns[cnt].policy.downloadApproval)
                    // Failed to map to known value?
                    if((out->augMetaBody.campaigns[cnt].campaignType == CMP_TYPE_OTA) &&
                        (out->augMetaBody.campaigns[cnt].policy.downloadApproval == APPROVAL_MODE_UNKNOWN)){
                        DEBUG_PRINT("ERR: Value mismatches expectation for: %s\n", getDBstr(icd_key_downloadApproval));
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    }
                } else {
                    /* Error out due to mandatory policy on policy settings for OTA campaignType */
                    if (out->augMetaBody.campaigns[cnt].campaignType == CMP_TYPE_OTA) {
                        DEBUG_PRINT("ERR: Missing key: %s\n", getDBstr((icd_key_downloadApproval)));\
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    } else {
                        out->augMetaBody.campaigns[cnt].policy.downloadApproval = APPROVAL_MODE_UNKNOWN;
                    }
                }
                /* policy.installApproval */
                GET_TEXT_VAL_PTR_FOR_OPTNL_KEY(policy, icd_key_installApproval)
                /* always evaluates to true if 'icd_mandatory' is used */
                if (LAST_SET_STR_VAL_PTR != NULL) {
                    POLICY_MAPPING(out->augMetaBody.campaigns[cnt].policy.installApproval)
                    // Failed to map to known value?
                    if((out->augMetaBody.campaigns[cnt].campaignType == CMP_TYPE_OTA) &&
                        (out->augMetaBody.campaigns[cnt].policy.installApproval == APPROVAL_MODE_UNKNOWN)){
                        DEBUG_PRINT("ERR: Value mismatches expectation for: %s\n", getDBstr(icd_key_installApproval));
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    }
                } else {
                    /* Error out due to mandatory policy on policy settings for OTA campaignType */
                    if (out->augMetaBody.campaigns[cnt].campaignType == CMP_TYPE_OTA) {
                        DEBUG_PRINT("ERR: Missing key: %s\n", getDBstr((icd_key_installApproval)));\
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    } else {
                        out->augMetaBody.campaigns[cnt].policy.installApproval = APPROVAL_MODE_UNKNOWN;
                    }
                }
                /* policy.activateApproval */
                GET_TEXT_VAL_PTR_FOR_OPTNL_KEY(policy, icd_key_activateApproval)
                /* always evaluates to true if 'icd_mandatory' is used */
                if (LAST_SET_STR_VAL_PTR != NULL) {
                    POLICY_MAPPING(out->augMetaBody.campaigns[cnt].policy.activateApproval)
                    // Failed to map to known value?
                    if((out->augMetaBody.campaigns[cnt].campaignType == CMP_TYPE_OTA) &&
                        (out->augMetaBody.campaigns[cnt].policy.activateApproval == APPROVAL_MODE_UNKNOWN)){
                        DEBUG_PRINT("ERR: Value mismatches expectation for: %s\n", getDBstr(icd_key_activateApproval));
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    }
                } else {
                    /* Error out due to mandatory policy on policy settings for OTA campaignType */
                    if (out->augMetaBody.campaigns[cnt].campaignType == CMP_TYPE_OTA) {
                        DEBUG_PRINT("ERR: Missing key: %s\n", getDBstr((icd_key_activateApproval)));\
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    } else {
                        out->augMetaBody.campaigns[cnt].policy.activateApproval = APPROVAL_MODE_UNKNOWN;
                    }
                }
                /* policy.completionAck */
                GET_TEXT_VAL_PTR_FOR_OPTNL_KEY(policy, icd_key_completionAck)
                /* always evaluates to true if 'icd_mandatory' is used */
                if (LAST_SET_STR_VAL_PTR != NULL) {
                    POLICY_MAPPING(out->augMetaBody.campaigns[cnt].policy.completionAck)
                    // Failed to map to known value?
                    if((out->augMetaBody.campaigns[cnt].campaignType == CMP_TYPE_OTA) &&
                        (out->augMetaBody.campaigns[cnt].policy.completionAck == APPROVAL_MODE_UNKNOWN)){
                        DEBUG_PRINT("ERR: Value mismatches expectation for: %s\n", getDBstr(icd_key_completionAck));
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    }
                } else {
                    /* Error out due to mandatory policy on policy settings for OTA campaignType */
                    if (out->augMetaBody.campaigns[cnt].campaignType == CMP_TYPE_OTA) {
                        DEBUG_PRINT("ERR: Missing key: %s\n", getDBstr((icd_key_completionAck)));\
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    } else {
                        out->augMetaBody.campaigns[cnt].policy.completionAck = APPROVAL_MODE_UNKNOWN;
                    }
                }

#if 0
                const json_t *const hmiMessages = json_getProperty(campaign, "hmiMessages");
                ERROR_ON_NULL(hmiMessages)
                if (JSON_OBJ != json_getType(hmiMessages)) {
                    DEBUG_PRINT("ERR: %s not a JSON object\n", json_getName(hmiMessages));
                    rval = UTIL_ERR_INVALID_PARAMETER;
                    goto error;
                }
                COPY_TEXT_VAL_FOR_MNDTRY_KEY(hmiMessages, icd_key_reportId,
                                             out->augMetaBody.campaigns[cnt].hmiMessages.reportId,
                                             MAX_REPORT_ID_SZ)

                COPY_TEXT_VAL_FOR_MNDTRY_KEY(hmiMessages, icd_key_url,
                                             out->augMetaBody.campaigns[cnt].hmiMessages.url,
                                             MAX_DWNLD_URL_SZ)

                COPY_UINT_VAL_FOR_MNDTRY_KEY(hmiMessages, icd_key_length,
                                             out->augMetaBody.campaigns[cnt].hmiMessages.length, uint32_t, UINT32_MAX)

                GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(hmiMessages, icd_key_digest)
                /* always evaluates to true if 'icd_mandatory' is used */
                if (NULL != LAST_SET_STR_VAL_PTR) {
                    if ((strnlength(LAST_SET_STR_VAL_PTR, ((SHA256_LENGTH * UINT_TWO) + UINT_ONE))) != (SHA256_LENGTH * UINT_TWO)) {
                        DEBUG_PRINT("ERR: Incorrect digest input length\n");
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    }
                    const int32_t ret = abq_hex_decode(LAST_SET_STR_VAL_PTR,
                                                 (int32_t)(SHA256_LENGTH * UINT_TWO),
                                                 abq_ptr2raw(out->augMetaBody.campaigns[cnt].hmiMessages.digest),
                                                 (int32_t) SHA256_LENGTH);
                    if (ret < SINT_ZERO) {
                        DEBUG_PRINT("ERR: Hex decode to binary failed\n");
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    }
                }
#endif

                const json_t *const changeEvents = json_getProperty(campaign, "changeEvents");
                ERROR_ON_NULL(changeEvents)
                if (JSON_ARRAY != json_getType(changeEvents)) {
                    DEBUG_PRINT("ERR: %s not a JSON array\n", json_getName(changeEvents));
                    rval = UTIL_ERR_INVALID_PARAMETER;
                    goto error;
                }

                uint16_t ce_cnt = UINT_ZERO;
                const json_t *ce = json_getChild(changeEvents);
                while (NULL != ce) {
                    if (MAX_CHANGE_EVENTS > ce_cnt) {
                        if (JSON_OBJ != json_getType(ce)) {
                            DEBUG_PRINT("ERR: %s not a JSON object\n", json_getName(ce));
                            rval = UTIL_ERR_INVALID_PARAMETER;
                            goto error;
                        }
                        COPY_TEXT_VAL_FOR_MNDTRY_KEY(ce, icd_key_reportId,
                                                     out->augMetaBody.campaigns[cnt].changeEvents[ce_cnt].reportId,
                                                     MAX_REPORT_ID_SZ)
                        COPY_TEXT_VAL_FOR_MNDTRY_KEY(ce, icd_key_rootUrl,
                                                     out->augMetaBody.campaigns[cnt].changeEvents[ce_cnt].rootUrl,
                                                     MAX_DWNLD_URL_SZ)
                        COPY_TEXT_VAL_FOR_MNDTRY_KEY(ce, icd_key_queryString,
                                                     out->augMetaBody.campaigns[cnt].changeEvents[ce_cnt].queryString,
                                                     MAX_QUERY_STRING_SZ)
                        COPY_TEXT_VAL_FOR_MNDTRY_KEY(ce, icd_key_elementId,
                                                     out->augMetaBody.campaigns[cnt].changeEvents[ce_cnt].elementId,
                                                     MAX_SERIAL_NUM_SZ)
                    } else {
                        DEBUG_PRINT("No resource for change events\n");
                        rval = UTIL_ERR_NO_RESOURCE;
                        goto error;
                        break;
                    }
                    ce = json_getSibling(ce);
                    ce_cnt++;
                }
                if (rval == UTIL_NO_ERROR) {
                    out->augMetaBody.campaigns[cnt].ceNum = ce_cnt;
                }


                const json_t *const preConditions = json_getProperty(campaign, "preConditions");
                /* precondition policy is "optional" */
                if (NULL != preConditions) {
                    if (JSON_ARRAY != json_getType(preConditions)) {
                        DEBUG_PRINT("ERR: %s not a JSON array\n", json_getName(preConditions));
                        rval = UTIL_ERR_INVALID_PARAMETER;
                        goto error;
                    }

                    uint16_t pc_cnt = UINT_ZERO;
                    const json_t *pc = json_getChild(preConditions);
                    while (NULL != pc) {
                        if (MAX_PRECONDITION_NUM > pc_cnt) {
                            if (JSON_OBJ != json_getType(pc)) {
                                DEBUG_PRINT("ERR: %s not a JSON object\n", json_getName(pc));
                                rval = UTIL_ERR_INVALID_PARAMETER;
                                goto error;
                            }
                            COPY_TEXT_VAL_FOR_MNDTRY_KEY(pc, icd_key_preCondition,
                                                         out->augMetaBody.campaigns[cnt].preConditions[pc_cnt].preCondition,
                                                         MAX_PRECONDITION_STR_SZ)
                            COPY_SINT_VAL_FOR_OPTNL_KEY(pc, icd_key_value,
                                                       out->augMetaBody.campaigns[cnt].preConditions[pc_cnt].value,
                                                       int32_t, INT32_MAX, INT32_MIN)

                            GET_TEXT_VAL_PTR_FOR_MNDTRY_KEY(pc, icd_key_displayMode)
                            /* always evaluates to true if 'icd_mandatory' is used */
                            if (NULL != LAST_SET_STR_VAL_PTR) {
                                if (SINT_ZERO == memcomp((cstr_t) LAST_SET_STR_VAL_PTR, (cstr_t) getDBstr(icd_key_MANDATORY),
                                                 strlen(getDBstr(icd_key_MANDATORY)))) {
                                    out->augMetaBody.campaigns[cnt].preConditions[pc_cnt].displayMode = DISPLAY_MODE_MANDATORY;
                                } else if (SINT_ZERO ==
                                           memcomp((cstr_t) LAST_SET_STR_VAL_PTR,
                                                   (cstr_t) getDBstr(icd_key_CONDITIONAL),
                                                   strlen(getDBstr(icd_key_CONDITIONAL)))) {
                                    out->augMetaBody.campaigns[cnt].preConditions[pc_cnt].displayMode = DISPLAY_MODE_CONDITIONAL;
                                } else {
                                    out->augMetaBody.campaigns[cnt].preConditions[pc_cnt].displayMode = DISPLAY_MODE_UNKNOWN;
                                    DEBUG_PRINT("ERR: Unexpected value for preconditions displayMode: %s", LAST_SET_STR_VAL_PTR);
                                    rval = UTIL_ERR_INVALID_PARAMETER;
                                    goto error;
                                }
                            }

                        } else {
                            DEBUG_PRINT("No resource for preconditions\n");
                            rval = UTIL_ERR_NO_RESOURCE;
                            goto error;
                            break;
                        }
                        pc = json_getSibling(pc);
                        pc_cnt++;
                    }
                    if (rval == UTIL_NO_ERROR) {
                        out->augMetaBody.campaigns[cnt].preCondNum = pc_cnt;
                    }
                }

            } else {
                DEBUG_PRINT("No resource for campaigns\n");
                rval = UTIL_ERR_NO_RESOURCE;
                goto error;
                break;
            }
            campaign = json_getSibling(campaign);
            cnt++;
        }
        if (rval == UTIL_NO_ERROR) {
            out->augMetaBody.campaignsNum = cnt;
        }
    }

    error:
    if (rval != UTIL_NO_ERROR) {
        CLEAR_OUTPUT_STRUCTURE(out, sizeof(augMeta_t));
    }
    return rval;
}

utilErr_t util_CreateSyncCheckBody(const syncCheck_t *const syncCheckInfo, str_t const buf, const size_t bufLen) {
    utilErr_t rval = UTIL_NO_ERROR;

    if ((NULL != syncCheckInfo) && (NULL != buf) && (UINT_ZERO < bufLen)) {
        ABQ_ENCODER(encoder, getUTF8Codec, buf, bufLen);
        ABQ_CLEAR_ENCODER(encoder);
        ADD_JSON_RAW(&encoder, '{')
        ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_primaryIdentifier), syncCheckInfo->primarySerialNum,
                          (int32_t) MAX_SERIAL_NUM_SZ)
        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_vehicleIdentifier), syncCheckInfo->vin, (int32_t) MAX_VIN_SZ)
        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_HEX(&encoder, getDBstr(icd_key_clientDigest), syncCheckInfo->cfgDgst, SHA256_LENGTH)
        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_UINT(&encoder, getDBstr(icd_key_timestamp), syncCheckInfo->timestamp)
        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_uploadReason), syncCheckInfo->upd, (int32_t) MAX_UPLOAD_REASON_SZ)
        ADD_JSON_RAW(&encoder, ',')

        ADD_JSON_KEY(&encoder, getDBstr(icd_key_packageStorage))
        ADD_JSON_RAW(&encoder, ':')
        ADD_JSON_RAW(&encoder, '{')
        ADD_JSON_PAIR_UINT(&encoder, getDBstr(icd_key_used), syncCheckInfo->packageStorage.used)
        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_UINT(&encoder, getDBstr(icd_key_available), syncCheckInfo->packageStorage.available)
        ADD_JSON_RAW(&encoder, '}')
        ADD_JSON_RAW(&encoder, '}')

        if (rval != UTIL_NO_ERROR) {
            ABQ_CLEAR_ENCODER(encoder);
        }
    } else {
        rval = UTIL_ERR_INVALID_PARAMETER;
    }
    return rval;
}


static utilErr_t addTargetBank(abq_encoder_t *const encoder, cstr_t const key, const targetBank_t *const tb) {
    utilErr_t rval = UTIL_NO_ERROR;
    ADD_JSON_KEY(encoder, key)
    ADD_JSON_RAW(encoder, ':')
    ADD_JSON_RAW(encoder, '{')
    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_hardwareId), tb->hardware_id, (int32_t) MAX_PART_NUM_SZ)

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_rewriteCount), tb->rewrite_count)

    switch (tb->bank) {
        case BANK_A:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_bank), "A", SINT_MINUS_ONE)
            break;
        case BANK_B:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_bank), "B", SINT_MINUS_ONE)
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }

    if (tb->swDetailNum > MAX_SW_DETAILS_NUM) {
        rval = UTIL_ERR_INVALID_PARAMETER;
    } else if (tb->swDetailNum > UINT_ZERO) {
        ADD_JSON_RAW(encoder, ',')
        ADD_JSON_KEY(encoder, getDBstr(icd_key_softwareDetails))
        ADD_JSON_RAW(encoder, ':')
        ADD_JSON_RAW(encoder, '[')
        for (uint64_t i = UINT_ZERO; i < MIN(MAX_SW_DETAILS_NUM, (uint64_t) tb->swDetailNum); ++i) {
            if (i > UINT_ZERO) {
                ADD_JSON_RAW(encoder, ',')
            }
            ADD_JSON_RAW(encoder, '{')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_subTargetId), tb->software_details[i].subTargetId,
                              (int32_t) MAX_SUB_TARGET_ID_SZ)

            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_softwareId), tb->software_details[i].softwareId,
                              (int32_t) MAX_ECU_SW_ID_SZ)
            ADD_JSON_RAW(encoder, '}')
        }
        ADD_JSON_RAW(encoder, ']')
    } else {
        /* nothing */
    }
    ADD_JSON_RAW(encoder, '}')

    return rval;
}

static utilErr_t addAugmentedManifest(abq_encoder_t *const encoder, cstr_t const key, const augmentedManifest_t *const am) {
    utilErr_t rval = UTIL_NO_ERROR;
    ADD_JSON_KEY(encoder, key)
    ADD_JSON_RAW(encoder, ':')
    ADD_JSON_RAW(encoder, '{')

    ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_timestamp), am->timestamp)

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_campaignId), am->lastCmpId, (int32_t) MAX_CAMPAIGN_ID_SZ)

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_rxswins), am->rxswinInfo, (int32_t) MAX_RXSWIN_SZ)

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_uploadReason), am->upd, (int32_t) MAX_UPLOAD_REASON_SZ)

    if (am->licenseInfoNum > MAX_TOTAL_LICENSE_INFO) {
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    } else if (am->licenseInfoNum > UINT_ZERO) {
        ADD_JSON_RAW(encoder, ',')
        ADD_JSON_KEY(encoder, getDBstr(icd_key_licenses))
        ADD_JSON_RAW(encoder, ':')
        ADD_JSON_RAW(encoder, '[')
        for (size_t i = UINT_ZERO; i < MIN(MAX_TOTAL_LICENSE_INFO, (uint64_t) am->licenseInfoNum); ++i) {
            if (i > UINT_ZERO) {
                ADD_JSON_RAW(encoder, ',')
            }
            ADD_JSON_RAW(encoder, '{')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_id), am->licenseInfo[i].id, (int32_t) MAX_LICENSE_ID_SZ)
            ADD_JSON_RAW(encoder, '}')
        }
        ADD_JSON_RAW(encoder, ']')
    } else {
        /* nothing */
    }

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_KEY(encoder, getDBstr(icd_key_dcmInfo))
    ADD_JSON_RAW(encoder, ':')
    ADD_JSON_RAW(encoder, '{')
    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_mobileNumber), am->dcmInfo.mobileNumber, (int32_t) MAX_MOBILE_NUMBER_SZ)
    ADD_JSON_RAW(encoder, '}')

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_KEY(encoder, getDBstr(icd_key_packageStorage))
    ADD_JSON_RAW(encoder, ':')
    ADD_JSON_RAW(encoder, '{')
    ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_used), am->packageStorage.used)
    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_available), am->packageStorage.available)
    ADD_JSON_RAW(encoder, '}')

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_KEY(encoder, getDBstr(icd_key_clientInfo))
    ADD_JSON_RAW(encoder, ':')
    ADD_JSON_RAW(encoder, '{')
    ADD_JSON_PAIR_HEX(encoder, getDBstr(icd_key_codeHash), am->clientInfo.codehash, SHA256_LENGTH)
    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_version), am->clientInfo.version, (int32_t) MAX_VERSION_SZ)
    ADD_JSON_RAW(encoder, '}')
    ADD_JSON_RAW(encoder, '}')

    error:
    return rval;
}

utilErr_t util_CreateConfigMatchBody(const cfgMatch_t *const cfgMatchInfo, str_t const buf, const size_t bufLen) {

    utilErr_t rval = UTIL_NO_ERROR;

    if ((NULL != cfgMatchInfo) && (NULL != buf) && (UINT_ZERO < bufLen)) {
        ABQ_ENCODER(encoder, getUTF8Codec, buf, bufLen);
        ABQ_CLEAR_ENCODER(encoder);
        ADD_JSON_RAW(&encoder, '{')
        ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_primaryIdentifier), cfgMatchInfo->primarySerialNum,
                          (int32_t) MAX_SERIAL_NUM_SZ)

        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_vehicleIdentifier), cfgMatchInfo->vin, (int32_t) MAX_VIN_SZ)

        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_securityAttack), cfgMatchInfo->securityAttack,
                          (int32_t) MAX_ATTACK_SZ)

        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_HEX(&encoder, getDBstr(icd_key_clientDigest), cfgMatchInfo->cfgDgst, SHA256_LENGTH)

        if (cfgMatchInfo->ecuInfoNum > MAX_TOTAL_ECUS) {
            rval = UTIL_ERR_INVALID_PARAMETER;
            goto error;
        } else if (cfgMatchInfo->ecuInfoNum > UINT_ZERO) {
            ADD_JSON_RAW(&encoder, ',')
            ADD_JSON_KEY(&encoder, getDBstr(icd_key_ecuVersionManifests))
            ADD_JSON_RAW(&encoder, ':')
            ADD_JSON_RAW(&encoder, '[')
            for (size_t i = UINT_ZERO; i < MIN((uint64_t) cfgMatchInfo->ecuInfoNum, MAX_TOTAL_ECUS); ++i) {
                if (i > UINT_ZERO) {
                    ADD_JSON_RAW(&encoder, ',')
                }
                ADD_JSON_RAW(&encoder, '{')
                ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_ecuIdentifier), cfgMatchInfo->ecusInfo[i].serialNum,
                                  (int32_t) MAX_SERIAL_NUM_SZ)

                ADD_JSON_RAW(&encoder, ',')
                ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_targetId), cfgMatchInfo->ecusInfo[i].targetId,
                                  (int32_t) MAX_TARGET_ID_SZ)

                if (UTIL_NO_ERROR == rval) {
                    ADD_JSON_RAW(&encoder, ',')
                    rval = addTargetBank(&encoder, getDBstr(icd_key_activeTarget),
                                         &cfgMatchInfo->ecusInfo[i].active_bank);
                }

                if (UTIL_NO_ERROR == rval) {
                    ADD_JSON_RAW(&encoder, ',')
                    rval = addTargetBank(&encoder, getDBstr(icd_key_inactiveTarget),
                                         &cfgMatchInfo->ecusInfo[i].inactive_bank);
                }
                ADD_JSON_RAW(&encoder, '}')
                if (UTIL_NO_ERROR != rval) {
                    goto error;
                }
            }
            ADD_JSON_RAW(&encoder, ']')
        } else {
            /* nothing */
        }

        if (UTIL_NO_ERROR == rval) {
            ADD_JSON_RAW(&encoder, ',')
            rval = addAugmentedManifest(&encoder, getDBstr(icd_key_augmentedManifest),
                                        &cfgMatchInfo->augmentedManifest);
        }
        ADD_JSON_RAW(&encoder, '}')

        error:
        if (rval != UTIL_NO_ERROR) {
            ABQ_CLEAR_ENCODER(encoder);
        }
    } else {
        rval = UTIL_ERR_INVALID_PARAMETER;
    }
    return rval;
}

static utilErr_t addEventStatus(abq_encoder_t *const encoder, const eventStatus_t status) {
    utilErr_t rval = UTIL_NO_ERROR;
    switch (status) {
        case EVENT_STATUS_ACCEPTED:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_status), getDBstr(icd_key_accepted), SINT_MINUS_ONE)
            break;
        case EVENT_STATUS_DECLINED:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_status), getDBstr(icd_key_declined), SINT_MINUS_ONE)
            break;
        case EVENT_STATUS_POSTPONED:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_status), getDBstr(icd_key_postponed), SINT_MINUS_ONE)
            break;
        case EVENT_STATUS_EXTRACTED:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_status), getDBstr(icd_key_extracted), SINT_MINUS_ONE)
            break;
        case EVENT_STATUS_EXTRACT_FAIL:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_status), getDBstr(icd_key_extract_fail), SINT_MINUS_ONE)
            break;
        case EVENT_STATUS_VALIDATED:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_status), getDBstr(icd_key_validated), SINT_MINUS_ONE)
            break;
        case EVENT_STATUS_VALIDATE_FAIL:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_status), getDBstr(icd_key_validate_fail), SINT_MINUS_ONE)
            break;
        case EVENT_STATUS_STARTED:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_status), getDBstr(icd_key_started), SINT_MINUS_ONE)
            break;
        case EVENT_STATUS_INPROGRESS:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_status), getDBstr(icd_key_inProgress), SINT_MINUS_ONE)
            break;
        case EVENT_STATUS_SUSPEND:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_status), getDBstr(icd_key_suspend), SINT_MINUS_ONE)
            break;
        case EVENT_STATUS_RESUME:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_status), getDBstr(icd_key_resume), SINT_MINUS_ONE)
            break;
        case EVENT_STATUS_SUCCESS:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_status), getDBstr(icd_key_success), SINT_MINUS_ONE)
            break;
        case EVENT_STATUS_FAILURE:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_status), getDBstr(icd_key_failure), SINT_MINUS_ONE)
            break;
        case EVENT_STATUS_CANCEL:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_status), getDBstr(icd_key_cancel), SINT_MINUS_ONE)
            break;
        case EVENT_STATUS_SKIPPED:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_status), getDBstr(icd_key_skipped), SINT_MINUS_ONE)
            break;
        case EVENT_STATUS_RETRY:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_status), getDBstr(icd_key_retry), SINT_MINUS_ONE)
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }
    return rval;
}

static utilErr_t addTimeMs(abq_encoder_t *const encoder, cstr_t const key, const timeMs_t *const time_input) {
    utilErr_t rval = UTIL_NO_ERROR;
    ADD_JSON_KEY(encoder, key)
    ADD_JSON_RAW(encoder, ':')
    ADD_JSON_RAW(encoder, '{')
    ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_time), time_input->time)
    switch (time_input->clockSource) {
        case EVENT_CLKSRC_SYNCED:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_clocksource), getDBstr(icd_key_synced), SINT_MINUS_ONE)
            break;
        case EVENT_CLKSRC_DEFAULT:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_clocksource), "default", SINT_MINUS_ONE)
            break;
        case EVENT_CLKSRC_GPS:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_clocksource), getDBstr(icd_key_gps), SINT_MINUS_ONE)
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }
    ADD_JSON_RAW(encoder, '}')
    return rval;
}

static utilErr_t addRmStateExtras(abq_encoder_t *const encoder, const rmStateExtras_t *const rmStateExtras) {
    utilErr_t rval = UTIL_NO_ERROR;

    ADD_JSON_KEY(encoder, getDBstr(icd_key_rmStateExtras))
    ADD_JSON_RAW(encoder, ':')
    ADD_JSON_RAW(encoder, '{')
    switch (rmStateExtras->stateScope) {
        case EVENT_STATE_SCOPE_VEHICLE:
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_stateScope), getDBstr(icd_key_vehicle), SINT_MINUS_ONE)

            switch (rmStateExtras->rmVehState) {
                case EVENT_VEHICLE_STATE_WAIT_CMP_USER_ACCPT:
                    ADD_JSON_RAW(encoder, ',')
                    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_rmVehState),
                                      getDBstr(icd_key_waitCampaignUserAcceptance), SINT_MINUS_ONE)
                    break;
                case EVENT_VEHICLE_STATE_WAIT_PKG_DL_USER_ACCPT:
                    ADD_JSON_RAW(encoder, ',')
                    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_rmVehState),
                                      getDBstr(icd_key_waitPackageDlUserAcceptance), SINT_MINUS_ONE)
                    break;
                case EVENT_VEHICLE_STATE_PKG_DL:
                    ADD_JSON_RAW(encoder, ',')
                    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_rmVehState), getDBstr(icd_key_packageDownload), SINT_MINUS_ONE)
                    break;
                case EVENT_VEHICLE_STATE_WAIT_INSTLL_USER_ACCPT:
                    ADD_JSON_RAW(encoder, ',')
                    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_rmVehState),
                                      getDBstr(icd_key_waitInstallUserAcceptance), SINT_MINUS_ONE)
                    break;
                case EVENT_VEHICLE_STATE_REPROG_READY:
                    ADD_JSON_RAW(encoder, ',')
                    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_rmVehState), getDBstr(icd_key_reprogReady), SINT_MINUS_ONE)
                    break;
                case EVENT_VEHICLE_STATE_REPROG:
                    ADD_JSON_RAW(encoder, ',')
                    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_rmVehState), getDBstr(icd_key_reprog), SINT_MINUS_ONE)
                    break;
                case EVENT_VEHICLE_STATE_WAIT_ACTIV_USER_ACCPT:
                    ADD_JSON_RAW(encoder, ',')
                    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_rmVehState),
                                      getDBstr(icd_key_waitActivateUserAcceptance), SINT_MINUS_ONE)
                    break;
                case EVENT_VEHICLE_STATE_POST_PROG:
                    ADD_JSON_RAW(encoder, ',')
                    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_rmVehState), getDBstr(icd_key_postProgramming), SINT_MINUS_ONE)
                    break;
                case EVENT_VEHICLE_STATE_SYSTEM_SYNC_CHK_COMP:
                    ADD_JSON_RAW(encoder, ',')
                    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_rmVehState), getDBstr(icd_key_systemSyncCheckComplete),
                                      SINT_MINUS_ONE)
                    break;
                case EVENT_VEHICLE_STATE_INFO_CMP_COMP:
                    ADD_JSON_RAW(encoder, ',')
                    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_rmVehState), getDBstr(icd_key_infoCampaignComplete), SINT_MINUS_ONE)
                    break;
                default:
                    /* MISRA __NOP comment */
                    break;
            }
            break;
        case EVENT_STATE_SCOPE_ECU:
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_stateScope), getDBstr(icd_key_ecu), SINT_MINUS_ONE)

            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_rmEcuState), rmStateExtras->rmEcuState)

            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_targetId), rmStateExtras->targetId, (int32_t) MAX_TARGET_ID_SZ)
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }
    ADD_JSON_RAW(encoder, '}')
    return rval;
}

static utilErr_t addTrackingExtras(abq_encoder_t *const encoder, const trackingExtras_t *const trackingExtras) {
    utilErr_t rval = UTIL_NO_ERROR;

    ADD_JSON_KEY(encoder, getDBstr(icd_key_trackingExtras))
    ADD_JSON_RAW(encoder, ':')
    ADD_JSON_RAW(encoder, '{')

    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_fileId), trackingExtras->fileId, (int32_t) MAX_EVENT_FILE_ID_SZ)

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_fileSize), trackingExtras->fileSize)

    ADD_JSON_RAW(encoder, ',')
    if (rval == UTIL_NO_ERROR) {
        rval = addTimeMs(encoder, getDBstr(icd_key_dlTime), &trackingExtras->dlTime);
    }
    ADD_JSON_RAW(encoder, '}')
    return rval;
}

static utilErr_t
addOemErrorList(abq_encoder_t *const encoder, cstr_t const key, const oemError_t oemError[], const uint16_t oemErrorNum) {
    utilErr_t rval = UTIL_NO_ERROR;

    ADD_JSON_KEY(encoder, key)
    ADD_JSON_RAW(encoder, ':')
    ADD_JSON_RAW(encoder, '[')
    for (uint64_t i = UINT_ZERO; i < MIN(MAX_EVENT_OEM_ERR_NUM, (uint64_t) oemErrorNum); ++i) {
        if (i > UINT_ZERO) {
            ADD_JSON_RAW(encoder, ',')
        }
        ADD_JSON_RAW(encoder, '{')
        ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_targetId), oemError[i].targetId, (int32_t) MAX_TARGET_ID_SZ)

        ADD_JSON_RAW(encoder, ',')
        ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_phaseCode), oemError[i].phaseCode)

        ADD_JSON_RAW(encoder, ',')
        ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_siteCode), oemError[i].siteCode)

        ADD_JSON_RAW(encoder, ',')
        ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_oemCode), oemError[i].oemCode)

        ADD_JSON_RAW(encoder, '}')
    }
    ADD_JSON_RAW(encoder, ']')
    return rval;
}

static utilErr_t addEcuInfo(abq_encoder_t *const encoder, const eventEcuInfo_t *const ecuInfo) {
    utilErr_t rval = UTIL_NO_ERROR;

    ADD_JSON_RAW(encoder, '{')

    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_targetId), ecuInfo->targetId, (int32_t) MAX_TARGET_ID_SZ)

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_ecuSoftwareId), ecuInfo->ecuSoftwareId, (int32_t) MAX_ECU_SW_ID_SZ)

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_HEX(encoder, getDBstr(icd_key_integrityInfo), ecuInfo->integrityInfo, SHA256_LENGTH)

    switch (ecuInfo->updateStatus) {
        case EVENT_UPDATE_STATUS_SUCCESS:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_updateStatus), getDBstr(icd_key_success), SINT_MINUS_ONE)
            break;
        case EVENT_UPDATE_STATUS_FAILURE:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_updateStatus), getDBstr(icd_key_failure), SINT_MINUS_ONE)
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }

    if (ecuInfo->dtcCodeNum > MAX_EVENT_DTC_NUM) {
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    } else if (ecuInfo->dtcCodeNum > UINT_ZERO) {
        ADD_JSON_RAW(encoder, ',')
        ADD_JSON_KEY(encoder, getDBstr(icd_key_dtcCodes))
        ADD_JSON_RAW(encoder, ':')
        ADD_JSON_RAW(encoder, '[')
        for (size_t c = UINT_ZERO; c < MIN(MAX_EVENT_DTC_NUM, (uint64_t) ecuInfo->dtcCodeNum); ++c) {
            if (c > UINT_ZERO) {
                ADD_JSON_RAW(encoder, ',')
            }
            ADD_JSON_RAW(encoder, '{')
            ADD_JSON_KEY(encoder, getDBstr(icd_key_code))
            ADD_JSON_RAW(encoder, ':')
            ADD_JSON_INT(encoder, ecuInfo->dtcCodes[c].code)
            ADD_JSON_RAW(encoder, '}')
        }
        ADD_JSON_RAW(encoder, ']')
    } else {
        /* nothing */
    }

    if (ecuInfo->errorCodeNum > MAX_EVENT_ERR_NUM) {
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    } else if (ecuInfo->errorCodeNum > UINT_ZERO) {
        ADD_JSON_RAW(encoder, ',')
        ADD_JSON_KEY(encoder, getDBstr(icd_key_errorCodes))
        ADD_JSON_RAW(encoder, ':')
        ADD_JSON_RAW(encoder, '[')
        for (size_t c = UINT_ZERO; c < MIN(MAX_EVENT_ERR_NUM, (uint64_t) ecuInfo->errorCodeNum); ++c) {
            if (c > UINT_ZERO) {
                ADD_JSON_RAW(encoder, ',')
            }
            ADD_JSON_RAW(encoder, '{')
            ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_code), ecuInfo->errorCodes[c].code)

            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_description), ecuInfo->errorCodes[c].description,
                              (int32_t) MAX_EVENT_ERR_DESC_SZ)
            ADD_JSON_RAW(encoder, '}')
        }
        ADD_JSON_RAW(encoder, ']')
    } else {
        /* nothing */
    }

    switch (ecuInfo->rewriteBank) {
        case BANK_A:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_rewriteBank), "A", SINT_MINUS_ONE)
            break;
        case BANK_B:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_rewriteBank), "B", SINT_MINUS_ONE)
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }

    switch (ecuInfo->updateMethod) {
        case EVENT_UPDATE_METHOD_OTA:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_updateMethod), getDBstr(icd_key_ota), SINT_MINUS_ONE)
            break;
        case EVENT_UPDATE_METHOD_WIRED:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_updateMethod), getDBstr(icd_key_wired), SINT_MINUS_ONE)
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }
    ADD_JSON_RAW(encoder, '}')
    error:
    return rval;
}

static utilErr_t
addEcuInfoList(abq_encoder_t *const encoder, cstr_t const key, const eventEcuInfo_t ecuInfo[], const uint16_t ecuInfoNum) {
    utilErr_t rval = UTIL_NO_ERROR;

    ADD_JSON_KEY(encoder, key)
    ADD_JSON_RAW(encoder, ':')
    ADD_JSON_RAW(encoder, '[')
    for (uint64_t i = UINT_ZERO; i < MIN(MAX_TOTAL_ECUS, (uint64_t) ecuInfoNum); ++i) {
        if (i > UINT_ZERO) {
            ADD_JSON_RAW(encoder, ',')
        }
        if (rval == UTIL_NO_ERROR) {
            rval = addEcuInfo(encoder, &ecuInfo[i]);
        }
        if (rval != UTIL_NO_ERROR) {
            goto error;
        }
    }
    ADD_JSON_RAW(encoder, ']')
    error:
    return rval;
}

static utilErr_t addActivationExtras(abq_encoder_t *const encoder, const actvtExtras_t *const actvtExtras) {
    utilErr_t rval = UTIL_NO_ERROR;

    ADD_JSON_KEY(encoder, getDBstr(icd_key_activationExtras))
    ADD_JSON_RAW(encoder, ':')
    ADD_JSON_RAW(encoder, '{')
    ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_updateTimeUTS), actvtExtras->updateTimeUTS)

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_KEY(encoder, getDBstr(icd_key_location))
    ADD_JSON_RAW(encoder, ':')
    ADD_JSON_RAW(encoder, '{')
    ADD_JSON_PAIR_SINT(encoder, getDBstr(icd_key_latitude), (number_t) actvtExtras->location.latitude)

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_SINT(encoder, getDBstr(icd_key_longitude), (number_t) actvtExtras->location.longitude)

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_gpsTime), actvtExtras->location.gpsTime)

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_SINT(encoder, getDBstr(icd_key_altitude), (number_t) actvtExtras->location.altitude)

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_system), actvtExtras->location.system,
                      (int32_t) MAX_EVENT_GPS_SYSTEM_NAME_SZ)

    ADD_JSON_RAW(encoder, '}')

    switch (actvtExtras->updateStatus) {
        case EVENT_UPDATE_STATUS_SUCCESS:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_updateStatus), getDBstr(icd_key_success), SINT_MINUS_ONE)
            break;
        case EVENT_UPDATE_STATUS_FAILURE:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_updateStatus), getDBstr(icd_key_failure), SINT_MINUS_ONE)
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }

    if (actvtExtras->ecuInfoNum > MAX_TOTAL_ECUS) {
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    } else if (actvtExtras->ecuInfoNum > UINT_ZERO) {
        ADD_JSON_RAW(encoder, ',')
        rval = addEcuInfoList(encoder, getDBstr(icd_key_ecuInfo), actvtExtras->eventEcuInfo,
                              actvtExtras->ecuInfoNum);
        if (rval != UTIL_NO_ERROR) {
            goto error;
        }
    } else {
        /* nothing */
    }

    if (actvtExtras->oemErrorNum > MAX_EVENT_OEM_ERR_NUM) {
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    } else if (actvtExtras->oemErrorNum > UINT_ZERO) {
        ADD_JSON_RAW(encoder, ',')
        rval = addOemErrorList(encoder, getDBstr(icd_key_oemErrors), actvtExtras->oemErrors,
                               actvtExtras->oemErrorNum);
        if (rval != UTIL_NO_ERROR) {
            goto error;
        }
    } else {
        /* nothing */
    }
    ADD_JSON_RAW(encoder, '}')
    error:
    return rval;
}

static utilErr_t addNotificationExtras(abq_encoder_t *const encoder, const ntfExtras_t *const ntfExtras) {
    utilErr_t rval = UTIL_NO_ERROR;

    ADD_JSON_KEY(encoder, getDBstr(icd_key_notificationExtras))
    ADD_JSON_RAW(encoder, ':')
    ADD_JSON_RAW(encoder, '{')
    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_campaignId), ntfExtras->cpId, (int32_t) MAX_CAMPAIGN_ID_SZ)

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_campaignName), ntfExtras->cpName, (int32_t) MAX_CAMPAIGN_NAME_SZ)

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_dataGroup), ntfExtras->dataGroup, (int32_t) MAX_DATA_GROUP_SZ)
    ADD_JSON_RAW(encoder, '}')
    return rval;
}

static utilErr_t addDownloadExtras(abq_encoder_t *const encoder, const dwnldExtras_t *const dwnldExtras) {
    utilErr_t rval = UTIL_NO_ERROR;

    ADD_JSON_KEY(encoder, getDBstr(icd_key_downloadExtras))
    ADD_JSON_RAW(encoder, ':')
    ADD_JSON_RAW(encoder, '{')

    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_downloadURL), dwnldExtras->downloadURL, (int32_t) MAX_DWNLD_URL_SZ)

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_downloadSize), dwnldExtras->downloadSize)

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_bytesDownloaded), dwnldExtras->bytesDownloaded)

    switch (dwnldExtras->eventSrc) {
        case EVENT_SRC_PHONE:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_eventSource), getDBstr(icd_key_phone), SINT_MINUS_ONE)
            break;
        case EVENT_SRC_VEHICLE:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_eventSource), getDBstr(icd_key_vehicle), SINT_MINUS_ONE)
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }
    switch (dwnldExtras->eventDst) {
        case EVENT_DST_SDP:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_eventDestination), getDBstr(icd_key_SDP), SINT_MINUS_ONE)
            break;
        case EVENT_DST_PHONE:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_eventDestination), getDBstr(icd_key_phone), SINT_MINUS_ONE)
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }

    switch (dwnldExtras->Trsprt) {
        case TRSPRT_WIFI:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_eventTransport), getDBstr(icd_key_wifi), SINT_MINUS_ONE)
            break;
        case TRSPRT_CELLULAR:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_eventTransport), getDBstr(icd_key_cellular), SINT_MINUS_ONE)
            break;
        case TRSPRT_BLUETOOTH:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_eventTransport), getDBstr(icd_key_bluetooth), SINT_MINUS_ONE)
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }

    if (dwnldExtras->oemErrorNum > MAX_EVENT_OEM_ERR_NUM) {
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    } else if (dwnldExtras->oemErrorNum > UINT_ZERO) {
        ADD_JSON_RAW(encoder, ',')
        rval = addOemErrorList(encoder, getDBstr(icd_key_oemErrors), dwnldExtras->oemErrors, dwnldExtras->oemErrorNum);
        if (rval != UTIL_NO_ERROR) {
            goto error;
        }
    } else {
        /* nothing */
    }

    if (dwnldExtras->ecuInfoNum > MAX_TOTAL_ECUS) {
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    } else if (dwnldExtras->ecuInfoNum > UINT_ZERO) {
        ADD_JSON_RAW(encoder, ',')
        rval = addEcuInfoList(encoder, getDBstr(icd_key_ecuInfo), dwnldExtras->eventEcuInfos, dwnldExtras->ecuInfoNum);
        if (rval != UTIL_NO_ERROR) {
            goto error;
        }
    } else {
        /* nothing */
    }

    ADD_JSON_RAW(encoder, '}')
    error:
    return rval;
}

static utilErr_t addInstallStartedExtras(abq_encoder_t *const encoder, const instllStartedExtras_t *const inst) {
    utilErr_t rval = UTIL_NO_ERROR;
    ADD_JSON_KEY(encoder, getDBstr(icd_key_installStartedExtras))
    ADD_JSON_RAW(encoder, ':')
    ADD_JSON_RAW(encoder, '{')
    ADD_JSON_KEY(encoder, getDBstr(icd_key_ecuInfo))
    ADD_JSON_RAW(encoder, ':')
    if (rval == UTIL_NO_ERROR) {
        rval = addEcuInfo(encoder, &inst->eventEcuInfo);
    }
    ADD_JSON_RAW(encoder, '}')
    return rval;
}

static utilErr_t addInstallProgressExtras(abq_encoder_t *const encoder, const instllProgressExtras_t *const inst) {
    utilErr_t rval = UTIL_NO_ERROR;

    ADD_JSON_KEY(encoder, getDBstr(icd_key_installProgressExtras))
    ADD_JSON_RAW(encoder, ':')
    ADD_JSON_RAW(encoder, '{')
    ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_numEcus), inst->numEcus)

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_completeEcus), inst->completeEcus)

    if (inst->ecuProgressNum > MAX_TOTAL_ECUS) {
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    } else if (inst->ecuProgressNum > UINT_ZERO) {
        ADD_JSON_RAW(encoder, ',')
        ADD_JSON_KEY(encoder, getDBstr(icd_key_ecuProgress))
        ADD_JSON_RAW(encoder, ':')
        ADD_JSON_RAW(encoder, '[')
        for (uint64_t i = UINT_ZERO; i < MIN((uint64_t) inst->ecuProgressNum, MAX_TOTAL_ECUS); ++i) {
            if (i > UINT_ZERO) {
                ADD_JSON_RAW(encoder, ',')
            }
            ADD_JSON_RAW(encoder, '{')
            ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_totalBytes), inst->ecuProgress[i].totalBytes)

            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_bytesProcessed), inst->ecuProgress[i].bytesProcessed)

            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_targetId), inst->ecuProgress[i].targetId,
                              (int32_t) MAX_TARGET_ID_SZ)
            ADD_JSON_RAW(encoder, '}')
            if (rval != UTIL_NO_ERROR) {
                break;
            }
        }
        ADD_JSON_RAW(encoder, ']')
    } else {
        /* nothing */
    }
    ADD_JSON_RAW(encoder, '}')
    error:
    return rval;
}

static utilErr_t addInstallCompleteExtras(abq_encoder_t *const encoder, const instllCompleteExtras_t *const inst) {
    utilErr_t rval = UTIL_NO_ERROR;

    ADD_JSON_KEY(encoder, getDBstr(icd_key_installCompleteExtras))
    ADD_JSON_RAW(encoder, ':')
    ADD_JSON_RAW(encoder, '{')
    ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_numEcus), inst->numEcus)

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_successEcus), inst->successEcus)

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_failEcus), inst->failEcus)

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_updateTimeUTS), inst->updateTimeUTS)

    if (inst->oemErrorNum > MAX_EVENT_OEM_ERR_NUM) {
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    } else if (inst->oemErrorNum > UINT_ZERO) {
        ADD_JSON_RAW(encoder, ',')
        rval = addOemErrorList(encoder, getDBstr(icd_key_oemErrors), inst->oemErrors, inst->oemErrorNum);
        if (rval != UTIL_NO_ERROR) {
            goto error;
        }
    } else {
        /* nothing */
    }

    if (inst->ecuInfoNum > MAX_TOTAL_ECUS) {
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    } else if (inst->ecuInfoNum > UINT_ZERO) {
        ADD_JSON_RAW(encoder, ',')
        rval = addEcuInfoList(encoder, getDBstr(icd_key_ecuInfo), inst->eventEcuInfo, inst->ecuInfoNum);
        if (rval != UTIL_NO_ERROR) {
            goto error;
        }
    } else {
        /* nothing */
    }

    ADD_JSON_RAW(encoder, '}')
    error:
    return rval;
}

static utilErr_t addEvent(abq_encoder_t *const encoder, const event_t *const event) {
    utilErr_t rval = UTIL_NO_ERROR;
    ADD_JSON_RAW(encoder, '{')
    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_eventUuid), event->eventUuid, (int32_t) UUID_LEN)

    switch (event->eventType) {
        case EVENT_RMSTATE:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_eventType), getDBstr(icd_key_rmState), SINT_MINUS_ONE)
            break;
        case EVENT_NOTIFICATION:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_eventType), getDBstr(icd_key_notification), SINT_MINUS_ONE)
            break;
        case EVENT_DOWNLOAD:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_eventType), getDBstr(icd_key_download), SINT_MINUS_ONE)
            break;
        case EVENT_INSTALLATION:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_eventType), getDBstr(icd_key_installation), SINT_MINUS_ONE)
            break;
        case EVENT_ACTIVATE:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_eventType), getDBstr(icd_key_activate), SINT_MINUS_ONE)
            break;
        case EVENT_SYNC_COMPLETE:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_eventType), getDBstr(icd_key_syncComplete), SINT_MINUS_ONE)
            break;
        case EVENT_ACK_INSTALL:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_eventType), getDBstr(icd_key_ackinstall), SINT_MINUS_ONE)
            break;
        case EVENT_OEM_ERROR:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_eventType), getDBstr(icd_key_oem_error), SINT_MINUS_ONE)
            break;
        case EVENT_TRACKING:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_eventType), getDBstr(icd_key_tracking), SINT_MINUS_ONE)
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }

    if (UTIL_NO_ERROR != rval) {
        goto error;
    }

    switch (event->unionFlag) {
        case UNION_FLAG_NTF:
            ADD_JSON_RAW(encoder, ',')
            rval = addNotificationExtras(encoder, &event->eventExtras.ntfExtras);
            break;
        case UNION_FLAG_DWNLD:
            ADD_JSON_RAW(encoder, ',')
            rval = addDownloadExtras(encoder, &event->eventExtras.dwnldExtras);
            break;
        case UNION_FLAG_INSTLLSTARTED:
            ADD_JSON_RAW(encoder, ',')
            rval = addInstallStartedExtras(encoder, &event->eventExtras.instllStartedExtras);
            break;
        case UNION_FLAG_INSTLLPROGRESS:
            ADD_JSON_RAW(encoder, ',')
            rval = addInstallProgressExtras(encoder, &event->eventExtras.instllProgressExtras);
            break;
        case UNION_FLAG_INSTLLCOMPLETE:
            ADD_JSON_RAW(encoder, ',')
            rval = addInstallCompleteExtras(encoder, &event->eventExtras.instllCompleteExtras);
            break;
        case UNION_FLAG_ACTVT:
            ADD_JSON_RAW(encoder, ',')
            rval = addActivationExtras(encoder, &event->eventExtras.actvtExtras);
            break;
        case UNION_FLAG_ERROR:
            if (event->oemErrorNum > MAX_EVENT_OEM_ERR_NUM) {
                rval = UTIL_ERR_INVALID_PARAMETER;
            } else if (event->oemErrorNum > UINT_ZERO) {
                ADD_JSON_RAW(encoder, ',')
                rval = addOemErrorList(encoder, getDBstr(icd_key_errorExtras), event->eventExtras.errorExtras,
                                       event->oemErrorNum);
            } else {
                /* nothing */
            }
            break;
        case UNION_FLAG_RMSTATE:
            ADD_JSON_RAW(encoder, ',')
            rval = addRmStateExtras(encoder, &event->eventExtras.rmStateExtras);
            break;
        case UNION_FLAG_TRACKING:
            ADD_JSON_RAW(encoder, ',')
            rval = addTrackingExtras(encoder, &event->eventExtras.trackingExtras);
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }
    if (UTIL_NO_ERROR != rval) {
        goto error;
    }
    if (event->status == EVENT_STATUS_CANCEL) {
        ADD_JSON_RAW(encoder, ',')
        ADD_JSON_KEY(encoder, getDBstr(icd_key_cancelExtras))
        ADD_JSON_RAW(encoder, ':')
        ADD_JSON_RAW(encoder, '{')
        switch (event->cancelExtras.cancelType) {
            case EVENT_CANCEL_TYPE_OTA:
                ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_cancelType), getDBstr(icd_key_ota), SINT_MINUS_ONE)
                break;
            case EVENT_CANCEL_TYPE_WIRED:
                ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_cancelType), getDBstr(icd_key_wired), SINT_MINUS_ONE)
                break;
            default:
                /* MISRA __NOP comment */
                break;
        }
        ADD_JSON_RAW(encoder, ',')
        ADD_JSON_PAIR_BOOL(encoder, getDBstr(icd_key_cancelExecuted), event->cancelExtras.cancelExecuted)

        ADD_JSON_RAW(encoder, '}')
    }

    switch (event->eventMode) {
        case EVENT_MODE_ACTIVE:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_eventMode), getDBstr(icd_key_active), SINT_MINUS_ONE)
            break;
        case EVENT_MODE_PASSIVE:
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_eventMode), getDBstr(icd_key_passive), SINT_MINUS_ONE)
            break;
        default:
            /* MISRA __NOP comment */
            break;
    }
    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_totalBytes), event->totalBytes)

    if (UTIL_NO_ERROR == rval) {
        ADD_JSON_RAW(encoder, ',')
        rval = addTimeMs(encoder, getDBstr(icd_key_timeStarted), &event->timeStarted);
    } else {
        goto error;
    }
    if (UTIL_NO_ERROR == rval) {
        ADD_JSON_RAW(encoder, ',')
        rval = addTimeMs(encoder, getDBstr(icd_key_currentTime), &event->currentTime);
    } else {
        goto error;
    }
    if (UTIL_NO_ERROR == rval) {
        ADD_JSON_RAW(encoder, ',')
        rval = addTimeMs(encoder, getDBstr(icd_key_postponeTime), &event->postponeTime);
    } else {
        goto error;
    }
    if (UTIL_NO_ERROR == rval) {
        rval = addEventStatus(encoder, event->status);
    } else {
        goto error;
    }

    if (event->errorCodeNum > MAX_EVENT_ERR_NUM) {
        rval = UTIL_ERR_INVALID_PARAMETER;
        goto error;
    } else if (event->errorCodeNum > UINT_ZERO) {
        ADD_JSON_RAW(encoder, ',')
        ADD_JSON_KEY(encoder, getDBstr(icd_key_errorCodes))
        ADD_JSON_RAW(encoder, ':')
        ADD_JSON_RAW(encoder, '[')
        for (size_t i = UINT_ZERO; i < MIN(MAX_EVENT_ERR_NUM, (uint64_t) event->errorCodeNum); ++i) {
            if (i > UINT_ZERO) {
                ADD_JSON_RAW(encoder, ',')
            }
            ADD_JSON_RAW(encoder, '{')
            ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_code), event->errorCodes[i].code)
            ADD_JSON_RAW(encoder, ',')
            ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_description), event->errorCodes[i].description,
                              (int32_t) MAX_EVENT_ERR_DESC_SZ)
            ADD_JSON_RAW(encoder, '}')
        }
        ADD_JSON_RAW(encoder, ']')
    } else {
        /* nothing */
    }

    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_UINT(encoder, getDBstr(icd_key_numRetries), event->numRetries)
    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_attemptId), event->attemptId, (int32_t) MAX_ATTEMPT_ID_SZ)
    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_reportId), event->reportId, (int32_t) MAX_REPORT_ID_SZ)
    ADD_JSON_RAW(encoder, ',')
    ADD_JSON_PAIR_STR(encoder, getDBstr(icd_key_campaignId), event->campaignId, (int32_t) MAX_CAMPAIGN_ID_SZ)

    ADD_JSON_RAW(encoder, '}')

    error:
    return rval;
}


utilErr_t util_CreateNotificationBody(const ntf_t *const ntf, str_t const buf,
                                      const size_t bufLen) {
    utilErr_t rval = UTIL_NO_ERROR;

    if ((NULL != ntf) && (NULL != buf) && (UINT_ZERO < bufLen)) {
        ABQ_ENCODER(encoder, getUTF8Codec, buf, bufLen);
        ABQ_CLEAR_ENCODER(encoder);
        ADD_JSON_RAW(&encoder, '{')
        ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_vin), ntf->vin, (int32_t) MAX_VIN_SZ)

        ADD_JSON_RAW(&encoder, ',')
        ADD_JSON_PAIR_STR(&encoder, getDBstr(icd_key_primaryIdentifier), ntf->primarySerialNum,
                          (int32_t) MAX_SERIAL_NUM_SZ)

        if (ntf->eventsNum > MAX_NOTIFICATIONS) {
            rval = UTIL_ERR_INVALID_PARAMETER;
        } else if (ntf->eventsNum > UINT_ZERO) {
            ADD_JSON_RAW(&encoder, ',')
            ADD_JSON_KEY(&encoder, getDBstr(icd_key_events))
            ADD_JSON_RAW(&encoder, ':')
            ADD_JSON_RAW(&encoder, '[')
            for (size_t i = UINT_ZERO; i < MIN((uint64_t) ntf->eventsNum, MAX_NOTIFICATIONS); ++i) {
                if (UTIL_NO_ERROR == rval) {
                    if (i > UINT_ZERO) {
                        ADD_JSON_RAW(&encoder, ',')
                    }
                    rval = addEvent(&encoder, &ntf->events[i]);
                } else {
                    break;
                }
            }
            ADD_JSON_RAW(&encoder, ']')
        } else {
            /* nothing */
        }

        ADD_JSON_RAW(&encoder, '}')
        if (rval != UTIL_NO_ERROR) {
            ABQ_CLEAR_ENCODER(encoder);
        }
    } else {
        rval = UTIL_ERR_INVALID_PARAMETER;
    }
    return rval;
}

/* parasoft-end-suppress MISRAC2012-RULE_15_1-a-4 MISRAC2012-RULE_15_4-a-4 */
/* parasoft-end-suppress CERT_C-MSC41-a-1 "This string does not contain sensitive information."  */
/* parasoft-end-suppress CERT_C-DCL06-a-3 */
/* parasoft-end-suppress CERT_C-DCL02-a-3 */
/* parasoft-end-suppress CERT_C-DCL19-a-3 */
