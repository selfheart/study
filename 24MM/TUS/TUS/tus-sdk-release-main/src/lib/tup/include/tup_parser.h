/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */
#ifndef TUP_PARSER_H
#define TUP_PARSER_H

/* system header */
#include <stdbool.h>
#include <stdint.h>

/* user header */
#include "tup_parser_error_code.h"

/* object macro */
#define TUP_ID_INVALID (0x0000U)
#define TUP_ID_INDEX (0x0010U)
#define TUP_ID_POS (0x0020U)
#define TUP_ID_ICV_TREE (0x0030U)
#define TUP_ID_ICV_ARRAY (0x0040U)
#define TUP_ID_IPKGINFO (0x0050U)
#define TUP_ID_IPKGNAME (0x0060U)
#define TUP_ID_COMP_ALL (0x0070U)
#define TUP_ID_COMP_FIXED (0x0080U)
#define TUP_ID_COMP_VAR (0x0090U)
#define TUP_ID_CRYPT (0x00A0U)
#define TUP_ID_UPDATEFLOW (0x00B0U)
#define TUP_ID_VERSION (0x00C0U)
#define TUP_ID_DOMAIN (0x00D0U)
#define TUP_ID_ORDER (0x00E0U)
#define TUP_ID_DELTA_PATCH (0x00F0U)
#define TUP_ID_ICV_AFTER_DATA (0x0100U)
#define TUP_ID_ICV_BLOCK (0x0110U)
#define TUP_ID_TLV_PLAIN (0x0FA0U)
#define TUP_ID_TLV_VF (0x0FB0U)
#define TUP_ID_TLV_VH (0x0FC0U)
#define TUP_ID_FIXED_FF (0x0FD0U)
#define TUP_ID_FIXED_VH (0x0FE0U)
#define TUP_ID_FIXED_FH (0x0FF0U)

#define TUP_ID_MASK_P (0x0001U)
#define TUP_ID_MASK_V (0x000EU)
#define TUP_ID_MASK_T (0x0FF0U)
#define TUP_ID_MASK_R (0x7000U)
#define TUP_ID_MASK_N (0x8000U)

#define TUP_FLAG_DECRYPT (0x1)
#define TUP_FLAG_DECOMPRESS (0x2)
#define TUP_FLAG_ICV_VERIFY (0x4)
#define TUP_FLAG_ALL \
    (TUP_FLAG_DECRYPT | TUP_FLAG_DECOMPRESS | TUP_FLAG_ICV_VERIFY)

#define TUP_ENDIAN_BIG (1)
#define TUP_ENDIAN_LITTLE (2)

#define TUP_STREAM_FILE (1)
#define TUP_STREAM_NET (2)
#define TUP_STREAM_LOOP (3)

/* function macro */

/* typedef definition */

/* enum definition */

/* struct/union definition */
typedef struct tup_tlv tup_tlv;
typedef struct tup_parser tup_parser;

typedef struct tup_file_stream {
    const char *path;
} tup_file_stream;

typedef struct tup_net_stream {
    const char *url;
} tup_net_stream;

typedef struct tup_loop_stream {
    tup_parser *parent;
    uint32_t index;
} tup_loop_stream;

typedef struct tup_stream_param {
    union {
        tup_file_stream *file;
        tup_net_stream *net;
        tup_loop_stream *loop;
    } stream;
    uint32_t type;
} tup_stream_param;

typedef struct tup_get_range {
    uint64_t offset;
    uint64_t size;
    uint64_t raw_size;
} tup_get_range;

typedef struct tup_get_ipkg_param {
    uint32_t flags;
    uint32_t reserved;
    uint64_t offset;
    uint64_t buffer_size;
    void *buffer;
} tup_get_ipkg_param;

typedef struct tup_get_tlv_praram {
    const tup_tlv *parent_tlv;
    tup_tlv *tlv;
} tup_get_tlv_param;

/* extern variable */

#ifdef __cplusplus
extern "C" {
#endif

/* function prototype */
int tup_create_parser(tup_parser **parser);
int tup_destroy_parser(tup_parser **parser);
int tup_attach_stream(tup_parser *parser, const tup_stream_param *param);
int tup_detach_stream(tup_parser *parser);
int tup_get_version(tup_parser *parser, uint32_t *version);
bool tup_check_supported_version(uint32_t version);
int tup_get_endian(uint32_t version);
int tup_validate(tup_parser *parser);
int tup_pread(tup_parser *parser, void *buf, uint64_t count, uint64_t offset);
int tup_get_inner_package_count(tup_parser *parser, uint32_t *count);
int tup_get_inner_package_size(
  tup_parser *parser, uint32_t index, tup_get_range *range);
int tup_get_inner_package_data(
  tup_parser *parser, uint32_t index, const tup_get_ipkg_param *param);
int tup_get_tlv(tup_parser *parser, tup_get_tlv_param *param);
int tup_find_tlv(tup_parser *parser, uint16_t id, tup_get_tlv_param *param);
int tup_get_inner_package_info(
  tup_parser *parser, uint32_t index, tup_tlv **ptlv);
int tup_verify_patched_data(tup_parser *parser, const uint8_t *data,
  uint64_t len, uint64_t offset, tup_tlv *ptlv);
int tup_get_tlv_type(const tup_tlv *ptlv, uint16_t *type);
int tup_get_tlv_length(const tup_tlv *ptlv, uint64_t *length);
int tup_get_tlv_value_size(const tup_tlv *ptlv, uint64_t *value_size);
int tup_get_tlv_value(
  const tup_tlv *ptlv, uint64_t offset, void *buffer, uint64_t buffer_size);

const char *tup_get_id_name(uint16_t id);

/* inline function */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !TUP_PARSER_H */
