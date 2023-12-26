/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

/* system header */
#include <curl/curl.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <queue.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* user header */
#include "logger.h"
#include "tup_compress.h"
#include "tup_crypt.h"
#include "tup_parser.h"

/* object macro */
#define TUP_VERSION_V0LE (0xa060e7fbU)
#define TUP_VERSION_V0BE (0xfbe760a0U)

#define TUP_FIXED_HEADER_MIN_SIZE (512)

#define TUP_MASK_48BIT_LENGTH (0xffffffffffffUL)

#define ICV_THREAD (0)
#define DECOMPRESS_THREAD (1)
#define MAX_WORKER_THREAD (2)

/* function macro */
#define TUP_HOWMANY(x, y) (((x) + ((y)-1)) / (y))

/* typedef */

/* enum */

/* struct/union */
typedef struct tup_tlv {
    uint64_t type : 16;
    uint64_t length : 48;
    uint8_t value[];
} tup_tlv;

typedef struct tup_tlpv {
    uint64_t type : 16;
    uint64_t length : 48;
    uint32_t padding;
    uint8_t value[];
} tup_tlpv;

typedef struct tup_comp_fixed {
    uint64_t algorithm;
    uint64_t offset;
    uint64_t decompress_size;
    uint32_t block_size;
    uint32_t padding_size;
    uint64_t compress_offset;
    uint64_t compress_size;
    uint64_t table_offset;
    uint8_t ext_attr[];
} tup_comp_fixed;

typedef struct tup_tlv_icv_tree {
    uint64_t type : 16;
    uint64_t length : 48;
    uint32_t algorithm;
    uint32_t blocksize;
    uint64_t keyinfo;
    uint64_t offset;
    uint64_t size;
    uint64_t icv_offset;
    uint8_t top_icv[];
} tup_tlv_icv_tree;

typedef struct tup_tlv_icv_block {
    uint64_t type : 16;
    uint64_t length : 48;
    uint64_t algorithm;
    uint64_t keyinfo;
    uint64_t offset;
    uint64_t size;
    uint8_t icv[];
} tup_tlv_icv_block;

typedef struct tup_tlv_icv_after_data {
    uint64_t type : 16;
    uint64_t length : 48;
    uint8_t icv[];
} tup_tlv_icv_after_data;

typedef struct tup_icvinfo {
    uint64_t offset;
    uint64_t size;
    uint8_t icv[];
} tup_icvinfo;

typedef struct tup_tlv_icv_array {
    uint64_t type : 16;
    uint64_t length : 48;
    uint32_t algorithm;
    uint32_t blocksize;
    uint64_t keyinfo;
    tup_icvinfo icv[];
} tup_tlv_icv_array;

typedef struct tup_fixed_header {
    uint64_t type;
    uint32_t version;
    uint32_t ipkgnum;
    uint64_t vh_type;
    uint64_t vh_offset;
    uint64_t vh_size;
    uint64_t vf_type;
    uint64_t vf_offset;
    uint64_t vf_size;
    uint64_t ff_type;
    uint64_t ff_offset;
    uint64_t ff_size;
} tup_fixed_header;

typedef struct tup_fixed_footer {
    uint64_t icv_array_offset;
    uint64_t icv_array_size;
    uint32_t rooticv_algorithm;
    uint32_t rooticv_blocksize;
    uint8_t rooticv_keyinfo[64];
    uint8_t rooticv[128];
    uint32_t vf_algorithm;
    uint32_t vf_blocksize;
    uint8_t vf_keyinfo[64];
    uint32_t sign_algorithm;
    uint32_t sign_blocksize;
    uint8_t sign_keyinfo[64];
} tup_fixed_footer;

typedef struct tup_region {
    uint64_t offset;
    uint64_t size;
} tup_region;

typedef struct tup_stream {
    union {
        int fd;
        CURL *curl;
        tup_loop_stream loop;
    } handle;
    struct tup_parser *parser;
    int (*open)(struct tup_stream *, const struct tup_stream_param *);
    int (*pread)(struct tup_stream *, void *, uint64_t, uint64_t);
    int (*close)(struct tup_stream *);
} tup_stream;

struct worker_data {
    SIMPLEQ_ENTRY(worker_data) next;
    uint8_t *buf;
    uint64_t size;
    uint64_t offset;
    uint64_t pkg_offset;
    uint64_t raw_size;
    uint64_t raw_offset;
    tup_tlv *ptlv;
    uint32_t flags;
    int result;
};

typedef struct worker_thread {
    pthread_t th;
    pthread_mutex_t mtx;
    pthread_cond_t cnd;
    SIMPLEQ_HEAD(, worker_data) queue;
} worker_thread;

typedef struct tup_parser {
    tup_stream stream;
    tup_fixed_header *fh;
    tup_region *vh;
    tup_tlv *vf;
    tup_tlv_icv_array *icv_array;
    tup_fixed_footer *ff;
    worker_thread worker[MAX_WORKER_THREAD];
    pthread_mutex_t mtx;
    pthread_cond_t cnd;
    SIMPLEQ_HEAD(, worker_data) queue;
    volatile bool use_worker_thread;
} tup_parser;

struct curl_write_buffer {
    char *ptr;
    size_t offset;
    size_t size;
};

/* static variable */

/* static function */
static int tup_do_verify_icv(tup_parser *parser, uint32_t algorithm,
  uint32_t blocksize, const uint8_t *key, size_t key_len, const uint8_t *data,
  uint64_t data_len, const uint8_t *icv, size_t icv_len, size_t count);
static int tup_verify_icv_tree(tup_parser *parser, tup_tlv *ptlv,
  const uint8_t *buf, uint64_t size, uint64_t offset);
static int tup_verify_icv_array(tup_parser *parser, const uint8_t *data,
  uint64_t data_len, const uint8_t *icv);
static int tup_raw_pread(
  tup_parser *parser, void *buf, uint64_t count, uint64_t offset);

static inline bool
tup_is_icv_tree(const tup_icvinfo *picvinfo)
{
    if (picvinfo->offset & 0x8000000000000000UL)
        return true;
    return false;
}

static int
tup_find_icvinfo(tup_parser *parser, uint64_t offset, uint64_t size,
  tup_icvinfo **found_icvinfo, tup_tlv **pptlv)
{
    if (NULL == parser) {
        LOG_ERROR("Invalid argument: parser=%p", parser);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == found_icvinfo) {
        LOG_ERROR("Invalid argument: found_icvinfo=%p", found_icvinfo);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    size_t icvinfo_size = sizeof(tup_icvinfo) +
      tup_get_icv_size(parser->icv_array->algorithm);
    size_t icvinfo_count = ((uint64_t)parser->icv_array->length -
                             sizeof(tup_tlv_icv_array)) /
      icvinfo_size;
    tup_icvinfo *picvinfo = (tup_icvinfo *)parser->icv_array->icv;

    *found_icvinfo = NULL;
    if (pptlv)
        *pptlv = NULL;

    for (size_t i = 0; i < icvinfo_count; ++i) {
        if (tup_is_icv_tree(picvinfo)) {
            if (NULL != pptlv) {
                uint8_t *tlv_icv = (uint8_t *)malloc(picvinfo->size);
                if (NULL == tlv_icv) {
                    LOG_ERROR(
                      "Memory allocation: request size=%zu", picvinfo->size);
                    return TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY;
                }
                int ret = tup_raw_pread(parser, tlv_icv, picvinfo->size,
                  picvinfo->offset & TUP_MASK_48BIT_LENGTH);
                if (0 != ret) {
                    LOG_ERROR("tup_raw_pread(): 0x%08x", ret);
                    free(tlv_icv);
                    return ret;
                }
                tup_tlv_icv_tree *icv_tree = (tup_tlv_icv_tree *)tlv_icv;
                if (icv_tree->offset <= offset &&
                  offset + size <= icv_tree->offset + icv_tree->size) {
                    *found_icvinfo = picvinfo;
                    *pptlv = (tup_tlv *)tlv_icv;
                    return 0;
                }
                free(tlv_icv);
            }
        } else {
            if (picvinfo->offset <= offset &&
              offset + size <= picvinfo->offset + picvinfo->size) {
                *found_icvinfo = picvinfo;
                return 0;
            }
        }
       picvinfo = (tup_icvinfo *)((uintptr_t)picvinfo + icvinfo_size);
    }
    return TUP_PARSER_ERROR_NOT_FOUND_ICV_INFORMATION;
}

static int
tup_raw_pread(tup_parser *parser, void *buf, uint64_t count, uint64_t offset)
{
    if (NULL == parser) {
        LOG_ERROR("Invalid argument: parser=%p", parser);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == parser->stream.pread) {
        LOG_ERROR(
          "Invalid argument: parser->stream.pread=%p", parser->stream.pread);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }
    int ret = parser->stream.pread(&parser->stream, buf, count, offset);
    if (0 != ret) {
        LOG_ERROR("parser->stream.pread(): 0x%08x", ret);
        return ret;
    }

    return 0;
}

int
tup_pread(tup_parser *parser, void *buf, uint64_t count, uint64_t offset)
{
    if (NULL == parser) {
        LOG_ERROR("Invalid argument: parser=%p", parser);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == buf) {
        LOG_ERROR("Invalid argument: buf=%p", buf);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    int icv_size = tup_get_icv_size(parser->icv_array->algorithm);
    if (0 >= icv_size) {
        LOG_ERROR("tup_get_icv_size(): %d", icv_size);
        return TUP_PARSER_ERROR_INVALID_ALGORITHM;
    }

    int ret;
    tup_icvinfo *found_icvinfo = NULL;
    tup_tlv *ptlv = NULL;
    ret = tup_find_icvinfo(parser, offset, count, &found_icvinfo, &ptlv);
    if (0 != ret) {
        LOG_ERROR("tup_find_icvinfo(): 0x%08x", ret);
        return ret;
    }

    if (ptlv) {
        tup_tlv_icv_tree *icv_tree = (tup_tlv_icv_tree *)ptlv;

        size_t head = (offset - icv_tree->offset) / icv_tree->blocksize;
        size_t tail = TUP_HOWMANY(
          offset - icv_tree->offset + count, icv_tree->blocksize);
        size_t block = TUP_HOWMANY(icv_tree->size, icv_tree->blocksize);
        size_t data_size = (tail - 1 - head) * icv_tree->blocksize;
        data_size += tail == block ? icv_tree->size % icv_tree->blocksize :
                                     icv_tree->blocksize;
        uint8_t *data = (uint8_t *)malloc(data_size);
        if (NULL == data) {
            LOG_ERROR("Memory allocation: request size=%zu", data_size);
            free(ptlv);
            return TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY;
        }

        ret = tup_raw_pread(parser, data, data_size,
          icv_tree->offset + head * icv_tree->blocksize);
        if (0 != ret) {
            LOG_ERROR("tup_raw_pread(): 0x%08x", ret);
            free(data);
            free(ptlv);
            return ret;
        }

        ret = tup_verify_icv_tree(parser, ptlv, data, data_size,
          icv_tree->offset + head * icv_tree->blocksize);
        if (0 != ret) {
            LOG_ERROR("tup_verify_icv_tree(): 0x%08x", ret);
            free(data);
            free(ptlv);
            return ret;
        }

        ret = tup_verify_icv_array(
          parser, (uint8_t *)ptlv, found_icvinfo->size, found_icvinfo->icv);
        if (0 != ret) {
            LOG_ERROR("tup_verify_icv_array(): 0x%08x", ret);
            free(data);
            free(ptlv);
            return ret;
        }

        memcpy(buf,
          &data[offset - icv_tree->offset - head * icv_tree->blocksize], count);
        free(data);
        free(ptlv);
    } else {
        uint8_t *data = (uint8_t *)malloc(found_icvinfo->size);
        if (NULL == data) {
            LOG_ERROR(
              "Memory allocation: request size=%zu", found_icvinfo->size);
            return TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY;
        }
        ret = tup_raw_pread(
          parser, data, found_icvinfo->size, found_icvinfo->offset);
        if (0 != ret) {
            LOG_ERROR("tup_raw_pread(): 0x%08x", ret);
            free(data);
            return ret;
        }

        ret = tup_verify_icv_array(
          parser, data, found_icvinfo->size, found_icvinfo->icv);
        if (0 != ret) {
            LOG_ERROR("tup_verify_icv_array(): 0x%08x", ret);
            free(data);
            return ret;
        }

        memcpy(buf, &data[offset - found_icvinfo->offset], count);
        free(data);
    }

    return 0;
}

static int
tup_do_verify_icv(tup_parser *parser, uint32_t algorithm, uint32_t blocksize,
  const uint8_t *key, size_t key_len, const uint8_t *data, uint64_t data_len,
  const uint8_t *icv, size_t icv_len, size_t count)
{
    int ret;

    if (NULL == parser) {
        LOG_ERROR("Invalid argument: parser=%p", parser);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == data) {
        LOG_ERROR("Invalid argument: data=%p", data);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == icv) {
        LOG_ERROR("Invalid argument: icv=%p", icv);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (blocksize < data_len) {
        LOG_ERROR("Invalid argument: blocksize=%u, data_len=%" PRIu64,
          blocksize, data_len);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (!tup_check_icv_algorithm(algorithm)) {
        LOG_ERROR("tup_check_icv_algorithm(): %s", "false");
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    uint8_t *block = (uint8_t *)calloc(1, blocksize);
    if (NULL == block) {
        LOG_ERROR("Memory allocation: request size=%zu", (size_t)blocksize);
        return TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY;
    }
    memcpy(block, data, data_len);

    unsigned char md_value[TUP_MD_MAX_SIZE];
    size_t md_len;

    ret = tup_generate_icv(
      algorithm, key, key_len, block, blocksize, md_value, &md_len);
    if (0 != ret) {
        LOG_ERROR("tup_generate_icv(): 0x%08x", ret);
        free(block);
        return ret;
    }

    for (size_t i = 0; i < count; ++i) {
        memset(block, 0, blocksize);
        memcpy(block, md_value, icv_len);
        ret = tup_generate_icv(
          algorithm, key, key_len, block, blocksize, md_value, &md_len);
        if (0 != ret) {
            LOG_ERROR("tup_generate_icv(): 0x%08x", ret);
            free(block);
            return ret;
        }
    }

    ret = tup_constant_time_memcmp(icv, md_value, icv_len);
    free(block);
    if (0 != ret) {
        LOG_ERROR("tup_constant_time_memcmp(): %d", ret);
        return TUP_PARSER_ERROR_INTEGRITY_CHECK;
    }

    return 0;
}

static size_t
tup_get_level(size_t n, size_t icv_per_block)
{
    size_t level = 0;
    while (n > icv_per_block) {
        n = TUP_HOWMANY(n, icv_per_block);
        level++;
    }
    return level;
}

static size_t
tup_get_icv_count(size_t n, size_t icv_per_block)
{
    size_t count = n;
    while (n > icv_per_block) {
        n = TUP_HOWMANY(n, icv_per_block);
        count += n;
    }
    return count;
}

static int
tup_verify_icv_tree(tup_parser *parser, tup_tlv *ptlv, const uint8_t *buf,
  uint64_t size, uint64_t offset)
{
    if (NULL == parser) {
        LOG_ERROR("Invalid argument: parser=%p", parser);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == ptlv) {
        LOG_ERROR("Invalid argument: ptlv=%p", ptlv);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == buf) {
        LOG_ERROR("Invalid argument: buf=%p", buf);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    int ret;
    uint16_t type;
    ret = tup_get_tlv_type(ptlv, &type);
    if (0 != ret) {
        LOG_ERROR("tup_get_tlv_type(): 0x%08x", ret);
        return ret;
    }

    if ((type & TUP_ID_MASK_T) != TUP_ID_ICV_TREE) {
        LOG_ERROR("Invalid TLV ID: 0x%08x", type & TUP_ID_MASK_T);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    tup_tlv_icv_tree *icv_tree = (tup_tlv_icv_tree *)ptlv;
    if (offset < icv_tree->offset ||
      (offset - icv_tree->offset) % icv_tree->blocksize ||
      icv_tree->offset + icv_tree->size < offset + size) {
        LOG_ERROR("Invalid argument: [%" PRIu64 ",%" PRIu64 ")", offset, size);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    int icv_size = tup_get_icv_size(icv_tree->algorithm);
    if (0 >= icv_size) {
        LOG_ERROR("tup_get_icv_size(): %d", icv_size);
        return TUP_PARSER_ERROR_INVALID_ALGORITHM;
    }

    if (!tup_check_icv_key(icv_tree->keyinfo)) {
        LOG_ERROR("tup_check_icv_key(): %s", "false");
        return TUP_PARSER_ERROR_INVALID_KEY_INFORMATION;
    }

    size_t key_len;
    ret = tup_get_icv_key_size(icv_tree->keyinfo, &key_len);
    if (0 != ret) {
        LOG_ERROR("tup_get_icv_key_size(): 0x%08x", ret);
        return ret;
    }
    uint8_t *key = (uint8_t *)malloc(key_len);
    if (NULL == key) {
        LOG_ERROR("Memory allocation: request size=%zu", key_len);
        return TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY;
    }
    ret = tup_get_icv_key(icv_tree->keyinfo, key);
    if (0 != ret) {
        LOG_ERROR("tup_get_icv_key(): 0x%08x", ret);
        free(key);
        return ret;
    }

    uint64_t blocksize = icv_tree->blocksize;
    size_t icv_per_block = blocksize / icv_size;
    size_t level0 = TUP_HOWMANY(icv_tree->size, blocksize);
    size_t icv_count = tup_get_icv_count(level0, icv_per_block);

    uint8_t *icv_table = (uint8_t *)malloc(icv_count * icv_size);
    if (NULL == icv_table) {
        LOG_ERROR("Memory allocation: request size=%zu", icv_count * icv_size);
        free(key);
        return TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY;
    }

    ret = tup_raw_pread(
      parser, icv_table, icv_count * icv_size, icv_tree->icv_offset);
    if (0 != ret) {
        LOG_ERROR("tup_raw_pread(): 0x%08x", ret);
        free(icv_table);
        free(key);
        return ret;
    }

    size_t head = (offset - icv_tree->offset) / blocksize;
    size_t tail = TUP_HOWMANY((offset - icv_tree->offset) + size, blocksize);
    uint64_t tail_size = ((offset - icv_tree->offset) + size) % blocksize ?
      ((offset - icv_tree->offset) + size) % blocksize :
      blocksize;

    // data block verify
    for (size_t i = head; i < tail; ++i) {
        ret = tup_do_verify_icv(parser, icv_tree->algorithm, blocksize, key,
          key_len, &buf[blocksize * (i - head)],
          tail - 1 == i ? tail_size : blocksize, &icv_table[icv_size * i],
          icv_size, 0);
        if (0 != ret) {
            LOG_ERROR("tup_do_verify_icv(): 0x%08x", ret);
            free(icv_table);
            free(key);
            return ret;
        }
    }

    // icv tree verify
    size_t level = tup_get_level(level0, icv_per_block);
    size_t table_offset = 0;
    size_t first = level0;
    size_t last = first;
    for (size_t n = 0; n < level; ++n) {
        head = head / icv_per_block;
        tail = TUP_HOWMANY(tail, icv_per_block);
        last = TUP_HOWMANY(first, icv_per_block);
        tail_size = (icv_size * first) % blocksize ?
          (icv_size * first) % blocksize :
          blocksize;
        for (size_t i = head; i < tail; ++i) {
            ret = tup_do_verify_icv(parser, icv_tree->algorithm, blocksize, key,
              key_len, &icv_table[table_offset + blocksize * i],
              last - 1 == i ? tail_size : blocksize,
              &icv_table[table_offset + icv_size * (first + i)], icv_size, 0);
            if (0 != ret) {
                LOG_ERROR("tup_do_verify_icv(): 0x%08x", ret);
                free(icv_table);
                free(key);
                return ret;
            }
        }
        table_offset += first * icv_size;
        first = last;
    }
    ret = tup_do_verify_icv(parser, icv_tree->algorithm, blocksize, key,
      key_len, &icv_table[table_offset], last * icv_size, icv_tree->top_icv,
      icv_size, 0);
    free(icv_table);
    free(key);
    if (0 != ret) {
        LOG_ERROR("tup_do_verify_icv(): 0x%08x", ret);
        return ret;
    }

    return 0;
}

static int
tup_verify_icv_block(tup_parser *parser, tup_tlv *ptlv, const uint8_t *buf,
  uint64_t size, uint64_t offset)
{
    int ret;
    uint16_t type;
    ret = tup_get_tlv_type(ptlv, &type);
    if (0 != ret) {
        LOG_ERROR("tup_get_tlv_type(): 0x%08x", ret);
        return ret;
    }

    if ((type & TUP_ID_MASK_T) != TUP_ID_ICV_BLOCK) {
        LOG_ERROR("Invalid TLV ID: 0x%08x", type & TUP_ID_MASK_T);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    tup_tlv_icv_block *icv_block = (tup_tlv_icv_block *)ptlv;
    if (offset != icv_block->offset || size != icv_block->size) {
        LOG_ERROR("Invalid argument: [%" PRIu64 ",%" PRIu64 ")", offset, size);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    int icv_size = tup_get_icv_size(icv_block->algorithm);
    if (0 >= icv_size) {
        LOG_ERROR("tup_get_icv_size(): %d", icv_size);
        return TUP_PARSER_ERROR_INVALID_ALGORITHM;
    }

    if (!tup_check_icv_key(icv_block->keyinfo)) {
        LOG_ERROR("tup_check_icv_key(): %s", "false");
        return TUP_PARSER_ERROR_INVALID_KEY_INFORMATION;
    }

    size_t key_len;
    ret = tup_get_icv_key_size(icv_block->keyinfo, &key_len);
    if (0 != ret) {
        LOG_ERROR("tup_get_icv_key_size(): 0x%08x", ret);
        return ret;
    }
    uint8_t *key = (uint8_t *)malloc(key_len);
    if (NULL == key) {
        LOG_ERROR("Memeory allocation: request size=%zu", key_len);
        return TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY;
    }
    ret = tup_get_icv_key(icv_block->keyinfo, key);
    if (0 != ret) {
        LOG_ERROR("tup_get_icv_key(): 0x%08x", ret);
        free(key);
        return ret;
    }
    ret = tup_do_verify_icv(parser, icv_block->algorithm, size, key, key_len,
      buf, size, icv_block->icv, icv_size, 0);
    free(key);
    if (0 != ret) {
        LOG_ERROR("tup_do_verify_icv(): 0x%08x", ret);
        return ret;
    }

    return 0;
}

static int
tup_verify_icv_array(tup_parser *parser, const uint8_t *data, uint64_t data_len,
  const uint8_t *icv)
{
    int ret;

    if (NULL == parser) {
        LOG_ERROR("Invalid argument: parser=%p", parser);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == parser->icv_array) {
        LOG_ERROR("Invalid argument: parser->icv_array=%p", parser->icv_array);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == data) {
        LOG_ERROR("Invalid argument: data=%p", data);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == icv) {
        LOG_ERROR("Invalid argument: icv=%p", icv);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    uint32_t algorithm = parser->icv_array->algorithm;
    uint32_t blocksize = parser->icv_array->blocksize;
    uint64_t keyinfo = parser->icv_array->keyinfo;

    if (!tup_check_icv_algorithm(algorithm)) {
        LOG_ERROR("tup_check_icv_algorithm(): %s", "false");
        return TUP_PARSER_ERROR_INVALID_ALGORITHM;
    }

    int icv_size = tup_get_icv_size(algorithm);
    if (0 >= icv_size) {
        LOG_ERROR("tup_get_icv_size(): %d", icv_size);
        return TUP_PARSER_ERROR_INVALID_ALGORITHM;
    }

    if (!tup_check_icv_key(keyinfo)) {
        LOG_ERROR("tup_check_icv_key(): %s", "false");
        return TUP_PARSER_ERROR_INVALID_KEY_INFORMATION;
    }

    size_t key_len;
    ret = tup_get_icv_key_size(keyinfo, &key_len);
    if (0 != ret) {
        LOG_ERROR("tup_get_icv_key_size(): 0x%08x", ret);
        return ret;
    }
    uint8_t *key = (uint8_t *)malloc(key_len);
    if (NULL == key) {
        LOG_ERROR("Memory allocation: request size=%zu", key_len);
        return TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY;
    }
    ret = tup_get_icv_key(keyinfo, key);
    if (0 != ret) {
        LOG_ERROR("tup_get_icv_key(): 0x%08x", ret);
        free(key);
        return ret;
    }

    ret = tup_do_verify_icv(parser, algorithm, blocksize, key, key_len, data,
      data_len, icv, icv_size, 1);
    free(key);
    if (0 != ret) {
        LOG_ERROR("tup_do_verify_icv(): 0x%08x", ret);
        return ret;
    }

    return 0;
}

static int
tup_check_rooticv(tup_parser *parser, uint32_t algorithm, uint32_t blocksize,
  const uint8_t keyinfo[64], const uint8_t *data, uint64_t data_size,
  const uint8_t rooticv[128])
{
    if (NULL == parser) {
        LOG_ERROR("Invalid argument: parser=%p", parser);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (!tup_check_icv_algorithm(algorithm)) {
        LOG_ERROR("tup_check_icv_algorithm(): %s", "false");
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == keyinfo) {
        LOG_ERROR("Invalid argument: keyinfo=%p", keyinfo);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == data) {
        LOG_ERROR("Invalid argument: data=%p", data);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == rooticv) {
        LOG_ERROR("Invalid argument: rooticv=%p", rooticv);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    int icv_size = tup_get_icv_size(algorithm);
    if (0 >= icv_size) {
        LOG_ERROR("tup_get_icv_size(): %d", icv_size);
        return TUP_PARSER_ERROR_INVALID_ALGORITHM;
    }

    if (!tup_check_rooticv_key(keyinfo)) {
        LOG_ERROR("tup_check_rooticv_key(): %s", "false");
        return TUP_PARSER_ERROR_INVALID_KEY_INFORMATION;
    }

    size_t key_len;
    int ret = tup_get_rooticv_key_size(keyinfo, &key_len);
    if (0 != ret) {
        LOG_ERROR("tup_get_rooticv_key_size(): 0x%08x", ret);
        return ret;
    }
    uint8_t *key = (uint8_t *)malloc(key_len);
    if (NULL == key) {
        LOG_ERROR("Memory allocation: request size=%zu", key_len);
        return TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY;
    }
    ret = tup_get_rooticv_key(keyinfo, key);
    if (0 != ret) {
        LOG_ERROR("tup_get_rooticv_key(): 0x%08x", ret);
        free(key);
        return ret;
    }

    ret = tup_do_verify_icv(parser, algorithm, blocksize, key, key_len, data,
      data_size, rooticv, icv_size, 1);
    free(key);
    if (0 != ret) {
        LOG_ERROR("tup_do_verify_icv(): 0x%08x", ret);
        return ret;
    }

    return 0;
}

static int
tup_decompress_fixed(tup_parser *parser, tup_tlv *ptlv,
  uint8_t *decompress_buffer, uint64_t decompress_size,
  uint64_t decompress_offset, const uint8_t *compress_buffer,
  uint64_t compress_size, uint64_t compress_offset)
{
    if (NULL == parser) {
        LOG_ERROR("Invalid argument: parser=%p", parser);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == ptlv) {
        LOG_ERROR("Invalid argument: ptlv=%p", ptlv);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == decompress_buffer) {
        LOG_ERROR("Invalid argument: decompress_buffer=%p", decompress_buffer);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == compress_buffer) {
        LOG_ERROR("Invalid argument: compress_buffer=%p", compress_buffer);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    int ret;
    tup_comp_fixed comp;
    ret = tup_get_tlv_value(ptlv, 0, &comp, sizeof(comp));
    if (0 != ret) {
        LOG_ERROR("tup_get_tlv_value(): 0x%08x", ret);
        return ret;
    }

    if (compress_offset + compress_size <
        (comp.compress_offset & TUP_MASK_48BIT_LENGTH) ||
      (comp.compress_offset & TUP_MASK_48BIT_LENGTH) + comp.compress_size <
        compress_offset) {
        LOG_ERROR("Invalid argument: [%" PRIu64 ",%" PRIu64 ")",
          compress_offset, compress_offset + compress_size);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (decompress_offset + decompress_size <
        (comp.offset & TUP_MASK_48BIT_LENGTH) ||
      (comp.offset & TUP_MASK_48BIT_LENGTH) + comp.decompress_size <
        decompress_offset) {
        LOG_ERROR("Invalid argument: [%" PRIu64 ",%" PRIu64 ")",
          decompress_offset, decompress_offset + decompress_size);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (!tup_check_compress_algorithm(comp.algorithm)) {
        LOG_ERROR("tup_check_compress_algorithm(): %s", "false");
        return TUP_PARSER_ERROR_INVALID_ALGORITHM;
    }

    uint8_t *block = (uint8_t *)malloc(comp.block_size);
    if (NULL == block) {
        LOG_ERROR(
          "Memory alloction: request size=%zu", (size_t)comp.block_size);
        return TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY;
    }

    size_t block_num = TUP_HOWMANY(comp.decompress_size, comp.block_size);
    uint64_t *comp_table;
    uint64_t one_table[1];
    if (1 == block_num) {
        comp_table = one_table;
    } else {
        comp_table = (uint64_t *)malloc(sizeof(uint64_t) * block_num);
        if (NULL == comp_table) {
            LOG_ERROR("Memory allocation: requent size=%zu",
              sizeof(uint64_t) * block_num);
            free(block);
            return TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY;
        }
        ret = tup_pread(parser, comp_table, sizeof(uint64_t) * (block_num - 1),
          comp.table_offset);
        if (0 != ret) {
            LOG_ERROR("tup_pread(): 0x%08x", ret);
            free(comp_table);
            free(block);
            return ret;
        }
    }
    comp_table[block_num - 1] = comp.compress_size;

    uint64_t decompress_base_offset = decompress_offset -
      (comp.offset & TUP_MASK_48BIT_LENGTH);
    size_t head = decompress_base_offset / comp.block_size;
    size_t tail = TUP_HOWMANY(
      decompress_base_offset + decompress_size, comp.block_size);
    uint64_t offset = head == 0 ? 0 :
                                  comp_table[head - 1] & TUP_MASK_48BIT_LENGTH;
    ret = 0;
    for (size_t i = head; i < tail; ++i) {
        uint64_t adjust_offset = i != head ?
          0 :
          decompress_base_offset % comp.block_size;
        uint64_t blocksize = i == tail - 1 &&
            (decompress_base_offset + decompress_size) % comp.block_size ?
          (decompress_base_offset + decompress_size) % comp.block_size :
          comp.block_size;
        size_t size = (comp_table[i] & TUP_MASK_48BIT_LENGTH) - offset;
        if (size == comp.block_size) {
            memcpy(&decompress_buffer[comp.block_size * (i - head)],
              &compress_buffer[offset - compress_offset + adjust_offset],
              blocksize - adjust_offset);
        } else {
            ret = tup_do_decompress(comp.algorithm, block, comp.block_size,
              &compress_buffer[offset - compress_offset], size, NULL, 0);
            if (0 != ret) {
                LOG_ERROR("tup_do_decompress(): 0x%08x", ret);
                break;
            }
            memcpy(&decompress_buffer[comp.block_size * (i - head)],
              &block[adjust_offset], blocksize - adjust_offset);
        }
        offset = comp_table[i] & TUP_MASK_48BIT_LENGTH;
    }

    if (comp_table != one_table)
        free(comp_table);
    free(block);

    return ret;
}

static int
tup_decompress(tup_parser *parser, tup_tlv *ptlv, uint8_t *decompress_buffer,
  uint64_t decompress_size, uint64_t decompress_offset,
  const uint8_t *compress_buffer, uint64_t compress_size,
  uint64_t compress_offset)
{
    if (NULL == parser) {
        LOG_ERROR("Invalid argument: parser=%p", parser);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == ptlv) {
        LOG_ERROR("Invalid argument: ptlv=%p", ptlv);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == decompress_buffer) {
        LOG_ERROR("Invalid argument: decompress_buffer=%p", decompress_buffer);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == compress_buffer) {
        LOG_ERROR("Invalid argument: compress_buffer=%p", compress_buffer);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    int ret;
    uint16_t type;
    ret = tup_get_tlv_type(ptlv, &type);
    if (0 != ret) {
        LOG_ERROR("tup_get_tlv_type(): 0x%08x", ret);
        return ret;
    }

    switch (type & TUP_ID_MASK_T) {
    case TUP_ID_COMP_FIXED:
        ret = tup_decompress_fixed(parser, ptlv, decompress_buffer,
          decompress_size, decompress_offset, compress_buffer, compress_size,
          compress_offset);
        break;
#if 0 // TODO
    case TUP_ID_COMP_VAR:
        ret = tup_decompress_variable(parser, ptlv, decompress_buffer, decompress_size, decompress_offset, compress_buffer, compress_size, compress_offset);
        break;
    case TUP_ID_COMP_ALL:
        ret = tup_decompress_all(parser, ptlv, decompress_buffer, decompress_size, decompress_offset, compress_buffer, compress_size, compress_offset);
        break;
#endif
    default:
        LOG_ERROR("Invalid TLV ID: 0x%08x", type & TUP_ID_MASK_T);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    return ret;
}

static bool
tup_check_id(uint16_t id)
{
    switch (id) {
    case TUP_ID_INDEX:
    case TUP_ID_POS:
    case TUP_ID_ICV_TREE:
    case TUP_ID_ICV_ARRAY:
    case TUP_ID_IPKGINFO:
    case TUP_ID_IPKGNAME:
    case TUP_ID_COMP_ALL:
    case TUP_ID_COMP_FIXED:
    case TUP_ID_COMP_VAR:
    case TUP_ID_CRYPT:
    case TUP_ID_UPDATEFLOW:
    case TUP_ID_VERSION:
    case TUP_ID_DOMAIN:
    case TUP_ID_ORDER:
    case TUP_ID_DELTA_PATCH:
    case TUP_ID_ICV_AFTER_DATA:
    case TUP_ID_ICV_BLOCK:
    case TUP_ID_TLV_PLAIN:
    case TUP_ID_TLV_VF:
    case TUP_ID_TLV_VH:
    case TUP_ID_FIXED_FF:
    case TUP_ID_FIXED_VH:
    case TUP_ID_FIXED_FH:
        return true;
    default:
        break;
    }
    return false;
}

static int
tup_file_open(tup_stream *stream, const tup_stream_param *param)
{
    if (NULL == stream) {
        LOG_ERROR("Invalid argument: stream=%p", stream);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == param) {
        LOG_ERROR("Invalid argument: param=%p", param);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == param->stream.file->path) {
        LOG_ERROR("Invalid argument: path=%p", param->stream.file->path);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    int fd = open(param->stream.file->path, O_RDONLY);
    if (-1 == fd) {
        int err = errno;
        LOG_ERROR("open(): %s", strerror(err));
        return TUP_PARSER_ERROR_CANNOT_OPEN_STREAM;
    }

    stream->handle.fd = fd;

    return 0;
}

static int
tup_file_pread(tup_stream *stream, void *buf, uint64_t count, uint64_t offset)
{
    if (NULL == stream) {
        LOG_ERROR("Invalid argument: stream=%p", stream);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == buf) {
        LOG_ERROR("Invalid argument: buf=%p", buf);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (UINT64_MAX - offset < count) {
        LOG_ERROR("Invalid argument: offset=%" PRIu64 ", count=%" PRIu64,
          offset, count);
        return TUP_PARSER_ERROR_OUT_OF_RANGE;
    }

    if (-1 == stream->handle.fd) {
        LOG_ERROR("Invalid argument: fd=%d", stream->handle.fd);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    size_t read_size = 0;
    do {
        ssize_t ssz = pread(stream->handle.fd, &((char *)buf)[read_size],
          count - read_size, offset + read_size);
        if (-1 == ssz) {
            int err = errno;
            LOG_ERROR("pread(): %s", strerror(err));
            return TUP_PARSER_ERROR_CANNOT_READ_STREAM;
        }
        if (0 == ssz)
            break;
        read_size += (size_t)ssz;
    } while ((uint64_t)read_size < count);

    if ((uint64_t)read_size != count) {
        LOG_ERROR(
          "pread(): read size=%" PRIu64 ", count=%" PRIu64, read_size, count);
        return TUP_PARSER_ERROR_CANNOT_READ_STREAM;
    }

    return 0;
}

static int
tup_file_close(tup_stream *stream)
{
    if (NULL == stream) {
        LOG_ERROR("Invalid argument: stream=%p", stream);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (-1 == stream->handle.fd) {
        LOG_ERROR("Invalid argument: fd=%d", stream->handle.fd);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    int fd = stream->handle.fd;
    stream->handle.fd = -1;
    if (close(fd)) {
        int err = errno;
        LOG_ERROR("close(): %s", strerror(err));
        return TUP_PARSER_ERROR_CANNOT_CLOSE_STREAM;
    }

    return 0;
}

static int
tup_curl_open(tup_stream *stream, const tup_stream_param *param)
{
    if (NULL == stream) {
        LOG_ERROR("Invalid argument: stream=%p", stream);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == param) {
        LOG_ERROR("Invalid argument: param=%p", param);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == param->stream.net->url) {
        LOG_ERROR("Invalid argument: url=%p", param->stream.net->url);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    CURL *curl = curl_easy_init();
    if (NULL == curl) {
        LOG_ERROR("curl_easy_init(): %p", curl);
        return TUP_PARSER_ERROR_CANNOT_OPEN_STREAM;
    }

    curl_easy_setopt(curl, CURLOPT_URL, param->stream.net->url);
    stream->handle.curl = curl;

    return 0;
}

static size_t
tup_curl_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    const size_t realsize = size * nmemb;
    struct curl_write_buffer *buf = (struct curl_write_buffer *)userdata;

    if (buf->size - buf->offset < realsize)
        return CURLE_WRITE_ERROR;

    memcpy(&(buf->ptr[buf->offset]), ptr, realsize);
    buf->offset += realsize;

    return realsize;
}

static int
tup_curl_pread(tup_stream *stream, void *buf, uint64_t count, uint64_t offset)
{
    if (NULL == stream) {
        LOG_ERROR("Invalid argument: stream=%p", stream);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == buf) {
        LOG_ERROR("Invalid argument: buf=%p", buf);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (UINT64_MAX - offset < count) {
        LOG_ERROR("Invalid argument: offset=%" PRIu64 ", count=%" PRIu64,
          offset, count);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == stream->handle.curl) {
        LOG_ERROR("Invalid argument: curl=%p", stream->handle.curl);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    CURL *handle = curl_easy_duphandle(stream->handle.curl);
    if (NULL == handle) {
        LOG_ERROR("curl_easy_duphandle(): %p", handle);
        return TUP_PARSER_ERROR_CANNOT_READ_STREAM;
    }

    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, tup_curl_write_callback);

    struct curl_write_buffer dest;
    dest.ptr = buf;
    dest.offset = 0;
    dest.size = count;
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &dest);

    char range[42]; // ceil(64 * log10(2)) * 2 + '-' + '\0'
    snprintf(
      range, sizeof(range), "%" PRIu64 "-%" PRIu64, offset, offset + count - 1);
    curl_easy_setopt(handle, CURLOPT_RANGE, range);

    CURLcode ret = curl_easy_perform(handle);
    curl_easy_cleanup(handle);
    if (CURLE_OK != ret) {
        LOG_ERROR("curl_easy_perform(): %d", ret);
        return TUP_PARSER_ERROR_CANNOT_READ_STREAM;
    }

    if (count != (uint64_t)dest.offset) {
        LOG_ERROR("curl_easy_perform(): count=%" PRIu64 ", offset=%" PRIu64,
          count, (uint64_t)dest.offset);
        return TUP_PARSER_ERROR_CANNOT_READ_STREAM;
    }

    return 0;
}

static int
tup_curl_close(tup_stream *stream)
{
    if (NULL == stream) {
        LOG_ERROR("Invalid argument: stream=%p", stream);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == stream->handle.curl) {
        LOG_ERROR("Invalid argument: curl=%p", stream->handle.curl);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    CURL *curl = stream->handle.curl;
    stream->handle.curl = NULL;
    curl_easy_cleanup(curl);

    return 0;
}

static int
tup_loop_open(tup_stream *stream, const tup_stream_param *param)
{
    if (NULL == stream) {
        LOG_ERROR("Invalid argument: stream=%p", stream);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == param) {
        LOG_ERROR("Invalid argument: param=%p", param);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == param->stream.loop->parent) {
        LOG_ERROR("Invalid argument: parent=%p", param->stream.loop->parent);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }
    // keep a copy of "*param"
    stream->handle.loop = *(param->stream.loop);
    // ToDo: increment refcount for parent?
    return 0;
}

static int
tup_loop_pread(tup_stream *stream, void *buf, uint64_t count, uint64_t offset)
{
    if (NULL == stream) {
        LOG_ERROR("Invalid argument: stream=%p", stream);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == buf) {
        LOG_ERROR("Invalid argument: buf=%p", buf);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (UINT64_MAX - offset < count) {
        LOG_ERROR("Invalid argument: offset=%" PRIu64 ", count=%" PRIu64,
          offset, count);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == stream->handle.loop.parent) {
        LOG_ERROR("Invalid argument: parent=%p", stream->handle.loop.parent);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    tup_get_ipkg_param param = {
        .offset = offset,
        .buffer_size = count,
        .buffer = buf,
    };
    int err = tup_get_inner_package_data(stream->handle.loop.parent,
      stream->handle.loop.index, &param);
    if (0 != err) {
        LOG_ERROR("tup_get_inner_package_data(): 0x%08x", err);
    }
    return err;
}

static int
tup_loop_close(tup_stream *stream)
{
    if (NULL == stream) {
        LOG_ERROR("Invalid argument: stream=%p", stream);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    // ToDo: decrement refcount for the parent parser?
    return 0;
}

static void *
icv_worker_thread(void *arg)
{
    tup_parser *parser = (tup_parser *)arg;
    worker_thread *cur = &parser->worker[ICV_THREAD];
    struct worker_data *data = NULL;

    while (1) {
        pthread_mutex_lock(&cur->mtx);
        while (1) {
            if (!parser->use_worker_thread) {
                pthread_mutex_unlock(&cur->mtx);
                return NULL;
            }
            if (!SIMPLEQ_EMPTY(&cur->queue)) {
                data = SIMPLEQ_FIRST(&cur->queue);
                SIMPLEQ_REMOVE_HEAD(&cur->queue, next);
                break;
            }
            pthread_cond_wait(&cur->cnd, &cur->mtx);
        }
        pthread_mutex_unlock(&cur->mtx);

        // icv verify
        int ret;
        if (data->flags & TUP_FLAG_ICV_VERIFY)
            ret = tup_pread(parser, data->buf, data->size, data->offset);
        else
            ret = tup_raw_pread(parser, data->buf, data->size, data->offset);
        if (0 != ret) {
            LOG_ERROR("tup_raw_pread(): 0x%08x", ret);
        }

        data->result = ret;
        pthread_mutex_lock(&(parser->worker[DECOMPRESS_THREAD].mtx));
        SIMPLEQ_INSERT_TAIL(
          &(parser->worker[DECOMPRESS_THREAD].queue), data, next);
        pthread_cond_broadcast(&(parser->worker[DECOMPRESS_THREAD].cnd));
        pthread_mutex_unlock(&(parser->worker[DECOMPRESS_THREAD].mtx));
    }

    return NULL;
}

static void *
decompress_worker_thread(void *arg)
{
    tup_parser *parser = (tup_parser *)arg;
    worker_thread *cur = &parser->worker[DECOMPRESS_THREAD];
    struct worker_data *data = NULL;

    while (1) {
        pthread_mutex_lock(&cur->mtx);
        while (1) {
            if (!parser->use_worker_thread) {
                pthread_mutex_unlock(&cur->mtx);
                return NULL;
            }
            if (!SIMPLEQ_EMPTY(&cur->queue)) {
                data = SIMPLEQ_FIRST(&cur->queue);
                SIMPLEQ_REMOVE_HEAD(&cur->queue, next);
                break;
            }
            pthread_cond_wait(&cur->cnd, &cur->mtx);
        }
        pthread_mutex_unlock(&cur->mtx);

        // decompress
        if (0 == data->result) {
            int ret = 0;
            uint8_t *buf = (uint8_t *)malloc(data->raw_size);
            if (NULL == buf) {
                LOG_ERROR("Memroy allocation: reqest size=%zu", data->raw_size);
                ret = TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY;
            }
            if (0 == ret) {
                ret = tup_decompress(parser, data->ptlv, buf, data->raw_size,
                  data->raw_offset, data->buf, data->size, data->pkg_offset);
                if (0 != ret) {
                    LOG_ERROR("tup_decompress(): 0x%08x", ret);
                }
                free(data->buf);
                data->buf = buf;
            }
            data->result = ret;
        }

        pthread_mutex_lock(&(parser->mtx));
        SIMPLEQ_INSERT_TAIL(&(parser->queue), data, next);
        pthread_cond_broadcast(&(parser->cnd));
        pthread_mutex_unlock(&(parser->mtx));
    }

    return NULL;
}

/* variable */

/* function */
int
tup_create_parser(tup_parser **parser)
{
    if (NULL == parser) {
        LOG_ERROR("Invalid argument: parser=%p", parser);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    *parser = NULL;

    tup_parser *ptr = (tup_parser *)calloc(1, sizeof(tup_parser));
    if (NULL == ptr) {
        LOG_ERROR("Memory allocation: reqest size=%zu", sizeof(tup_parser));
        return TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY;
    }

    ptr->use_worker_thread = true;
    typedef void *(*worker_func)(void *);
    const worker_func func[MAX_WORKER_THREAD] = { icv_worker_thread,
        decompress_worker_thread };
    for (size_t i = 0; i < MAX_WORKER_THREAD; ++i) {
        worker_thread *cur = &ptr->worker[i];
        SIMPLEQ_INIT(&cur->queue);
        pthread_mutex_init(&cur->mtx, NULL);
        pthread_cond_init(&cur->cnd, NULL);
        pthread_create(&cur->th, NULL, func[i], ptr);
    }

    SIMPLEQ_INIT(&(ptr->queue));
    pthread_mutex_init(&(ptr->mtx), NULL);
    pthread_cond_init(&(ptr->cnd), NULL);

    *parser = ptr;

    return 0;
}

int
tup_destroy_parser(tup_parser **parser)
{
    if (NULL == parser) {
        LOG_ERROR("Invalid argument: parser=%p", parser);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == *parser) {
        LOG_ERROR("Invalid argument: *parser=%p", *parser);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL != (*parser)->stream.open) {
        LOG_ERROR("Invalid argument: open=%p", (*parser)->stream.open);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    (*parser)->use_worker_thread = false;
    for (size_t i = 0; i < MAX_WORKER_THREAD; ++i) {
        worker_thread *cur = &(*parser)->worker[i];
        pthread_mutex_lock(&cur->mtx);
        pthread_cond_broadcast(&cur->cnd);
        pthread_mutex_unlock(&cur->mtx);
        pthread_join(cur->th, NULL);
        pthread_cond_destroy(&cur->cnd);
        pthread_mutex_destroy(&cur->mtx);
    }

    pthread_cond_destroy(&((*parser)->cnd));
    pthread_mutex_destroy(&((*parser)->mtx));

    free((*parser)->icv_array);
    (*parser)->icv_array = NULL;
    free((*parser)->fh);
    (*parser)->fh = NULL;
    free((*parser)->vh);
    (*parser)->vh = NULL;
    free((*parser)->vf);
    (*parser)->vf = NULL;
    free((*parser)->ff);
    (*parser)->ff = NULL;
    free(*parser);
    *parser = NULL;

    return 0;
}

int
tup_attach_stream(tup_parser *parser, const tup_stream_param *param)
{
    int ret;

    if (NULL == parser) {
        LOG_ERROR("Invalid argument: parser=%p", parser);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == param) {
        LOG_ERROR("Invalid argument: param=%p", param);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    switch (param->type) {
    case TUP_STREAM_FILE:
        parser->stream.parser = parser;
        parser->stream.open = tup_file_open;
        parser->stream.pread = tup_file_pread;
        parser->stream.close = tup_file_close;
        ret = parser->stream.open(&parser->stream, param);
        if (0 != ret) {
            LOG_ERROR("parser->stream.open(): 0x%08x", ret);
            parser->stream.parser = NULL;
            parser->stream.open = NULL;
            parser->stream.pread = NULL;
            parser->stream.close = NULL;
            return ret;
        }
        break;
    case TUP_STREAM_NET:
        parser->stream.parser = parser;
        parser->stream.open = tup_curl_open;
        parser->stream.pread = tup_curl_pread;
        parser->stream.close = tup_curl_close;
        ret = parser->stream.open(&parser->stream, param);
        if (0 != ret) {
            LOG_ERROR("parser->stream.open(): 0x%08x", ret);
            parser->stream.parser = NULL;
            parser->stream.open = NULL;
            parser->stream.pread = NULL;
            parser->stream.close = NULL;
            return ret;
        }
        break;
    case TUP_STREAM_LOOP:
        parser->stream.parser = parser;
        parser->stream.open = tup_loop_open;
        parser->stream.pread = tup_loop_pread;
        parser->stream.close = tup_loop_close;
        ret = parser->stream.open(&parser->stream, param);
        if (0 != ret) {
            LOG_ERROR("parser->stream.open(): 0x%08x", ret);
            parser->stream.parser = NULL;
            parser->stream.open = NULL;
            parser->stream.pread = NULL;
            parser->stream.close = NULL;
            return ret;
        }
        break;
    default:
        LOG_ERROR("Invalid argument: stream type 0x%08x", param->type);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    return 0;
}

int
tup_detach_stream(tup_parser *parser)
{
    if (NULL == parser) {
        LOG_ERROR("Invalid argument: parser=%p", parser);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == parser->stream.close) {
        LOG_ERROR("Invalid argument: close=%p", parser->stream.close);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    int ret = parser->stream.close(&parser->stream);
    if (0 != ret) {
        LOG_ERROR("parser->stream.close(): 0x%08x", ret);
    }
    parser->stream.parser = NULL;
    parser->stream.open = NULL;
    parser->stream.pread = NULL;
    parser->stream.close = NULL;

    return ret;
}

int
tup_get_version(tup_parser *parser, uint32_t *version)
{
    if (NULL == parser) {
        LOG_ERROR("Invalid argument: parser=%p", parser);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == version) {
        LOG_ERROR("Invalid argument: version=%p", version);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    // check FH ICV
    int ret = tup_validate(parser);
    if (0 != ret) {
        LOG_ERROR("tup_validate(): 0x%08x", ret);
        return ret;
    }

    *version = parser->fh->version;

    return 0;
}

bool
tup_check_supported_version(uint32_t version)
{
    switch (version) {
    case TUP_VERSION_V0LE:
    case TUP_VERSION_V0BE:
        return true;
    default:
        break;
    }

    return false;
}

int
tup_get_endian(uint32_t version)
{
    switch (version) {
    case TUP_VERSION_V0LE:
        return TUP_ENDIAN_LITTLE;
    case TUP_VERSION_V0BE:
        return TUP_ENDIAN_BIG;
    default:
        break;
    }
    return TUP_PARSER_ERROR_INVALID_ENDIAN;
}

int
tup_validate(tup_parser *parser)
{
    if (NULL == parser) {
        LOG_ERROR("Invalid argument: parser=%p", parser);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    // check stream
    if (NULL == parser->stream.open) {
        LOG_ERROR("Invalid argument: open=%p", parser->stream.open);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == parser->fh && NULL == parser->ff) {
        uint8_t *fhblock = (uint8_t *)malloc(TUP_FIXED_HEADER_MIN_SIZE);
        if (NULL == fhblock) {
            LOG_ERROR("Memory allocation: request size=%zu",
              (size_t)TUP_FIXED_HEADER_MIN_SIZE);
            return TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY;
        }

        int ret = tup_raw_pread(parser, fhblock, TUP_FIXED_HEADER_MIN_SIZE, 0);
        if (0 != ret) {
            LOG_ERROR("tup_raw_pread(): 0x%08x", ret);
            free(fhblock);
            return ret;
        }

        tup_fixed_header *pfh = (tup_fixed_header *)fhblock;

        uint8_t *ffblock = (uint8_t *)malloc(pfh->ff_size);
        if (NULL == ffblock) {
            LOG_ERROR("Memory allocation: %zu", pfh->ff_size);
            free(fhblock);
            return TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY;
        }

        ret = tup_raw_pread(parser, ffblock, pfh->ff_size, pfh->ff_offset);
        if (0 != ret) {
            LOG_ERROR("tup_raw_pread(): 0x%08x", ret);
            free(ffblock);
            free(fhblock);
            return ret;
        }

        tup_fixed_footer *pff = (tup_fixed_footer *)ffblock;

        // check signatue
        uint32_t algorithm = pff->sign_algorithm;
        size_t sign_len;
        ret = tup_get_sign_len(algorithm, &sign_len);
        if (0 != ret) {
            LOG_ERROR("tup_get_sign_len(): 0x%08x", ret);
            free(ffblock);
            free(fhblock);
            return ret;
        }
        uint32_t blocksize = pff->sign_blocksize;
        uint8_t *keyinfo = pff->sign_keyinfo;
        uint8_t *data = ffblock;
        size_t data_len = sizeof(*pff);
        uint8_t *signature = ffblock + data_len;
        ret = tup_verify_signature(
          algorithm, blocksize, keyinfo, data, data_len, signature, sign_len);
        if (0 != ret) {
            LOG_ERROR("tup_verify_signature(): 0x%08x", ret);
            free(ffblock);
            free(fhblock);
            return ret;
        }

        uint8_t *icv_array = (uint8_t *)malloc(pff->icv_array_size);
        if (NULL == icv_array) {
            LOG_ERROR(
              "Memory allocation: request size=%zu", pff->icv_array_size);
            free(ffblock);
            free(fhblock);
            return TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY;
        }
        ret = tup_raw_pread(
          parser, icv_array, pff->icv_array_size, pff->icv_array_offset);
        if (0 != ret) {
            LOG_ERROR("tup_raw_pread(): 0x%08x", ret);
            free(icv_array);
            free(ffblock);
            free(fhblock);
            return ret;
        }

        ret = tup_check_rooticv(parser, pff->rooticv_algorithm,
          pff->rooticv_blocksize, pff->rooticv_keyinfo, icv_array,
          pff->icv_array_size, pff->rooticv);
        if (0 != ret) {
            LOG_ERROR("tup_check_rooticv(): 0x%08x", ret);
            free(icv_array);
            free(ffblock);
            free(fhblock);
            return ret;
        }

        parser->icv_array = (tup_tlv_icv_array *)icv_array;

        tup_icvinfo *found_icvinfo;
        // check FF ICV
        ret = tup_find_icvinfo(parser, pfh->ff_offset,
          offsetof(tup_fixed_footer, rooticv), &found_icvinfo, NULL);
        if (0 != ret) {
            LOG_ERROR("tup_find_icvinfo(): 0x%08x", ret);
            free(parser->icv_array);
            parser->icv_array = NULL;
            free(ffblock);
            free(fhblock);
            return ret;
        }

        ret = tup_verify_icv_array(parser, ffblock,
          offsetof(tup_fixed_footer, rooticv), found_icvinfo->icv);
        if (0 != ret) {
            LOG_ERROR("tup_verify_icv_array(): 0x%08x", ret);
            free(parser->icv_array);
            parser->icv_array = NULL;
            free(ffblock);
            free(fhblock);
            return ret;
        }

        // check FH ICV
        ret = tup_find_icvinfo(
          parser, 0, TUP_FIXED_HEADER_MIN_SIZE, &found_icvinfo, NULL);
        if (0 != ret) {
            LOG_ERROR("tup_find_icvinfo(): 0x%08x", ret);
            free(parser->icv_array);
            parser->icv_array = NULL;
            free(ffblock);
            free(fhblock);
            return ret;
        }

        ret = tup_verify_icv_array(
          parser, fhblock, TUP_FIXED_HEADER_MIN_SIZE, found_icvinfo->icv);
        if (0 != ret) {
            LOG_ERROR("tup_verify_icv_array(): 0x%08x", ret);
            free(parser->icv_array);
            parser->icv_array = NULL;
            free(ffblock);
            free(fhblock);
            return ret;
        }

        parser->fh = pfh;
        parser->ff = pff;
    }
    return 0;
}

int
tup_get_inner_package_count(tup_parser *parser, uint32_t *count)
{
    if (NULL == parser) {
        LOG_ERROR("Invalid argument: parser=%p", parser);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == count) {
        LOG_ERROR("Invalid argument: count=%p", count);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    // check FH ICV
    int ret = tup_validate(parser);
    if (0 != ret) {
        LOG_ERROR("tup_validate(): 0x%08x", ret);
        return ret;
    }

    *count = parser->fh->ipkgnum;

    return 0;
}

int
tup_get_inner_package_size(
  tup_parser *parser, uint32_t index, tup_get_range *range)
{
    if (NULL == parser) {
        LOG_ERROR("Invalid argument: parser=%p", parser);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == range) {
        LOG_ERROR("Invalid argument: range=%p", range);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    int ret = tup_validate(parser);
    if (0 != ret) {
        LOG_ERROR("tup_validate(): 0x%08x", ret);
        return ret;
    }

    if (parser->fh->ipkgnum <= index) {
        LOG_ERROR("Invalid argument: index=%u", index);
        return TUP_PARSER_ERROR_OUT_OF_RANGE;
    }

    if (NULL == parser->vh) {
        uint8_t *vhblock = (uint8_t *)malloc(parser->fh->vh_size);
        if (NULL == vhblock) {
            LOG_ERROR(
              "Memory allocation: request size=%zu", parser->fh->vh_size);
            return TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY;
        }

        int ret = tup_pread(
          parser, vhblock, parser->fh->vh_size, parser->fh->vh_offset);
        if (0 != ret) {
            LOG_ERROR("tup_pread(): 0x%08x", ret);
            free(vhblock);
            return ret;
        }

        parser->vh = (tup_region *)vhblock;
    }

    uint64_t raw_size = parser->vh[index].size;
    tup_tlv *ptlv = NULL;
    ret = tup_get_inner_package_info(parser, index, &ptlv);
    if (0 != ret) {
        LOG_ERROR("tup_get_inner_package_info(): 0x%08x", ret);
        return ret;
    }
    if (NULL != ptlv) {
        tup_get_tlv_param tlv_param;
        tlv_param.parent_tlv = ptlv;
        tlv_param.tlv = NULL;
        ret = tup_find_tlv(parser, TUP_ID_COMP_FIXED, &tlv_param);
        if (0 != ret) {
            LOG_ERROR("tup_find_tlv(): 0x%08x", ret);
            return ret;
        }
        if (NULL != tlv_param.tlv) {
            tup_comp_fixed comp;
            ret = tup_get_tlv_value(tlv_param.tlv, 0, &comp, sizeof(comp));
            if (0 != ret) {
                LOG_ERROR("tup_get_tlv_value(): 0x%08x", ret);
                return ret;
            }
            if (raw_size < comp.compress_size) {
                LOG_ERROR("Invalid compress_size=%" PRIu64, comp.compress_size);
                return TUP_PARSER_ERROR_OUT_OF_RANGE;
            }
            raw_size -= comp.compress_size;
            raw_size += comp.decompress_size;
            if (raw_size < comp.decompress_size) {
                LOG_ERROR(
                  "Invalid decompress_size=%" PRIu64, comp.decompress_size);
                return TUP_PARSER_ERROR_OUT_OF_RANGE;
            }
        }
    }

    range->offset = parser->vh[index].offset;
    range->size = parser->vh[index].size;
    range->raw_size = raw_size;

    return 0;
}

int
tup_get_inner_package_data(
  tup_parser *parser, uint32_t index, const tup_get_ipkg_param *param)
{
    if (NULL == parser) {
        LOG_ERROR("Invalid argument: parser=%p", parser);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == param) {
        LOG_ERROR("Invalid argument: param=%p", param);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (param->flags & ~TUP_FLAG_ALL) {
        LOG_ERROR("Invalid argument: flags=%x", param->flags);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == param->buffer) {
        LOG_ERROR("Invalid argument: buffer=%p", param->buffer);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    // check ICV
    int ret = tup_validate(parser);
    if (0 != ret) {
        LOG_ERROR("tup_validate(): 0x%08x", ret);
        return ret;
    }

    if (parser->fh->ipkgnum <= index) {
        LOG_ERROR("Invalid argument: index=%u", index);
        return TUP_PARSER_ERROR_OUT_OF_RANGE;
    }

    tup_get_range range;
    ret = tup_get_inner_package_size(parser, index, &range);
    if (0 != ret) {
        LOG_ERROR("tup_get_inner_package_size(): 0x%08x", ret);
        return ret;
    }

    if (range.raw_size <= param->offset) {
        LOG_ERROR("Invalide argument: offset=%" PRIu64, param->offset);
        return TUP_PARSER_ERROR_OUT_OF_RANGE;
    }

    if (0 == param->buffer_size ||
      range.raw_size - param->offset < param->buffer_size) {
        LOG_ERROR("Invalid argument: buff_size=%" PRIu64, param->buffer_size);
        return TUP_PARSER_ERROR_OUT_OF_RANGE;
    }

    if (param->flags & TUP_FLAG_ALL) {
        int err = 0;
        tup_tlv *ptlv;
        ret = tup_get_inner_package_info(parser, index, &ptlv);
        if (0 != ret) {
            LOG_ERROR("tup_get_inner_package_info(): 0x%08x", ret);
            return ret;
        }
        tup_get_tlv_param tlv_param;
        tlv_param.parent_tlv = ptlv;
        if (param->flags & TUP_FLAG_DECOMPRESS) {
            tlv_param.tlv = NULL;
            ret = tup_find_tlv(parser, TUP_ID_COMP_FIXED, &tlv_param);
            if (0 != ret) {
                LOG_ERROR("tup_find_tlv(): 0x%08x", ret);
                return ret;
            }
            if (NULL != tlv_param.tlv) {
                tup_comp_fixed comp;
                ret = tup_get_tlv_value(tlv_param.tlv, 0, &comp, sizeof(comp));
                if (0 != ret) {
                    LOG_ERROR("tup_get_tlv_value(): 0x%08x", ret);
                    return ret;
                }
                size_t block_num = TUP_HOWMANY(
                  comp.decompress_size, comp.block_size);
                uint64_t *comp_table;
                uint64_t one_table[1];
                if (1 == block_num) {
                    comp_table = one_table;
                } else {
                    comp_table = (uint64_t *)malloc(
                      sizeof(uint64_t) * block_num);
                    if (NULL == comp_table) {
                        LOG_ERROR("Memory allocation: request size=%zu",
                          sizeof(uint64_t) * block_num);
                        return TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY;
                    }
                    ret = tup_pread(parser, comp_table,
                      sizeof(uint64_t) * (block_num - 1), comp.table_offset);
                    if (0 != ret) {
                        LOG_ERROR("tup_pread(): 0x%08x", ret);
                        free(comp_table);
                        return ret;
                    }
                }
                comp_table[block_num - 1] = comp.compress_size;

                uint64_t decompress_base_offset = param->offset -
                  (comp.offset & TUP_MASK_48BIT_LENGTH);
                size_t head = decompress_base_offset / comp.block_size;
                size_t tail = TUP_HOWMANY(
                  decompress_base_offset + param->buffer_size, comp.block_size);
                uint64_t offset = head == 0 ?
                  0 :
                  comp_table[head - 1] & TUP_MASK_48BIT_LENGTH;
                err = 0;
                size_t count = 0;
                for (size_t i = head; i < tail; ++i) {
                    size_t size = (comp_table[i] & TUP_MASK_48BIT_LENGTH) -
                      offset;
                    uint8_t *buf = (uint8_t *)malloc(size);
                    if (NULL == buf) {
                        LOG_ERROR("Memory allocation: request size=%zu", size);
                        err = TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY;
                        break;
                    }
                    struct worker_data *work_data = (struct worker_data *)
                      malloc(sizeof(struct worker_data));
                    if (NULL == work_data) {
                        LOG_ERROR("Memory allocation: request size=%zu",
                          sizeof(struct worker_data));
                        free(buf);
                        err = TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY;
                    }
                    memset(work_data, 0, sizeof(struct worker_data));
                    work_data->ptlv = tlv_param.tlv;
                    work_data->buf = buf;
                    work_data->size = size;
                    work_data->offset = range.offset + offset;
                    work_data->pkg_offset = offset;
                    work_data->raw_size = comp.block_size;
                    work_data->raw_offset = i * comp.block_size;
                    work_data->flags = param->flags;

                    count++;
                    pthread_mutex_lock(&(parser->worker[ICV_THREAD].mtx));
                    SIMPLEQ_INSERT_TAIL(
                      &(parser->worker[ICV_THREAD].queue), work_data, next);
                    pthread_cond_broadcast(&(parser->worker[ICV_THREAD].cnd));
                    pthread_mutex_unlock(&(parser->worker[ICV_THREAD].mtx));
                    offset = comp_table[i] & TUP_MASK_48BIT_LENGTH;
                }
                if (one_table != comp_table)
                    free(comp_table);

                while (0 < count--) {
                    struct worker_data *work_data;
                    pthread_mutex_lock(&(parser->mtx));
                    while (1) {
                        if (!SIMPLEQ_EMPTY(&(parser->queue))) {
                            work_data = SIMPLEQ_FIRST(&(parser->queue));
                            SIMPLEQ_REMOVE_HEAD(&(parser->queue), next);
                            break;
                        }
                        pthread_cond_wait(&(parser->cnd), &(parser->mtx));
                    }
                    pthread_mutex_unlock(&(parser->mtx));

                    int result = work_data->result;
                    if (0 == err && 0 != result)
                        err = result;
                    if (0 == err) {
                        size_t dest_offset = 0;
                        size_t src_offset = 0;
                        size_t copy_size;
                        if (param->offset < work_data->raw_offset) {
                            dest_offset = work_data->raw_offset - param->offset;
                            copy_size = param->buffer_size - dest_offset <
                                work_data->raw_size ?
                              param->buffer_size - dest_offset :
                              work_data->raw_size;
                        } else {
                            src_offset = param->offset - work_data->raw_offset;
                            copy_size = work_data->raw_size - src_offset <
                                param->buffer_size ?
                              work_data->raw_size - src_offset :
                              param->buffer_size;
                        }
                        memcpy(param->buffer + dest_offset,
                          work_data->buf + src_offset, copy_size);
                    }
                    free(work_data->buf);
                    free(work_data);
                }

                return err;
            }
        }

        if (param->flags & TUP_FLAG_ICV_VERIFY)
            ret = tup_pread(parser, param->buffer, param->buffer_size,
              range.offset + param->offset);
        else
            ret = tup_raw_pread(parser, param->buffer, param->buffer_size,
              range.offset + param->offset);
        if (0 != ret) {
            LOG_ERROR("tup_pread() or tup_raw_pread(): 0x%08x", ret);
            return ret;
        }
        return 0;
    } else {
        ret = tup_raw_pread(parser, param->buffer, param->buffer_size,
          range.offset + param->offset);
        if (0 != ret) {
            LOG_ERROR("tup_raw_pread(): 0x%08x", ret);
            return ret;
        }
        return 0;
    }
}

int
tup_get_tlv(tup_parser *parser, tup_get_tlv_param *param)
{
    if (NULL == parser) {
        LOG_ERROR("Invalid argument: parser=%p", parser);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == param) {
        LOG_ERROR("Invalid argument: param=%p", param);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == param->parent_tlv) {
        if (NULL == param->tlv) {
            if (NULL == parser->vf) {
                int ret = tup_validate(parser);
                if (0 != ret) {
                    LOG_ERROR("tup_validate(): 0x%08x", ret);
                    return ret;
                }

                uint8_t *vfblock = (uint8_t *)malloc(parser->fh->vf_size);
                if (NULL == vfblock) {
                    LOG_ERROR("Memory allocation: request size=%zu",
                      parser->fh->vf_size);
                    return TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY;
                }

                ret = tup_pread(
                  parser, vfblock, parser->fh->vf_size, parser->fh->vf_offset);
                if (0 != ret) {
                    LOG_ERROR("tup_pread(): 0x%08x", ret);
                    free(vfblock);
                    return ret;
                }

                parser->vf = (tup_tlv *)vfblock;
            }
            param->tlv = parser->vf;
        } else {
            if ((uintptr_t)parser->vf + parser->fh->vf_size <=
              (uintptr_t)param->tlv + param->tlv->length)
                param->tlv = NULL;
            else
                param->tlv = (tup_tlv *)((uintptr_t)param->tlv +
                  param->tlv->length);
        }
    } else {
        if (NULL == param->tlv) {
            if (param->parent_tlv->type & TUP_ID_MASK_P) {
                tup_tlpv *tmp = (tup_tlpv *)param->parent_tlv;
                param->tlv = (tup_tlv *)&tmp->value[tmp->padding];
            } else {
                param->tlv = (tup_tlv *)param->parent_tlv->value;
            }
        } else {
            if ((uintptr_t)param->parent_tlv + param->parent_tlv->length <=
              (uintptr_t)param->tlv + param->tlv->length)
                param->tlv = NULL;
            else
                param->tlv = (tup_tlv *)((uintptr_t)param->tlv +
                  param->tlv->length);
        }
    }

    return 0;
}

int
tup_find_tlv(tup_parser *parser, uint16_t id, tup_get_tlv_param *param)
{
    if (NULL == parser) {
        LOG_ERROR("Invalid argument: parser=%p", parser);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == param) {
        LOG_ERROR("Invalid argument: param=%p", param);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (!tup_check_id(id)) {
        LOG_ERROR("tup_check_id(): %s", "false");
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == param->parent_tlv) {
        if (NULL == param->tlv) {
            if (NULL == parser->vf) {
                int ret = tup_validate(parser);
                if (0 != ret) {
                    LOG_ERROR("tup_validate(): 0x%08x", ret);
                    return ret;
                }

                uint8_t *vfblock = (uint8_t *)malloc(parser->fh->vf_size);
                if (NULL == vfblock) {
                    LOG_ERROR("Memory allocation: request size=%zu",
                      parser->fh->vf_size);
                    return TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY;
                }

                ret = tup_pread(
                  parser, vfblock, parser->fh->vf_size, parser->fh->vf_offset);
                if (0 != ret) {
                    LOG_ERROR("tup_pread(): 0x%08x", ret);
                    free(vfblock);
                    return ret;
                }

                parser->vf = (tup_tlv *)vfblock;
            }
            tup_tlv *tlv;
            for (tlv = parser->vf;
                 (uintptr_t)tlv < (uintptr_t)parser->vf + parser->fh->vf_size;
                 tlv = (tup_tlv *)((uintptr_t)tlv + tlv->length)) {
                if ((tlv->type & TUP_ID_MASK_T) == id) {
                    param->tlv = tlv;
                    break;
                }
            }
        } else {
            if ((uintptr_t)parser->vf + parser->fh->vf_size <=
              (uintptr_t)param->tlv + param->tlv->length)
                param->tlv = NULL;
            else {
                tup_tlv *tlv = (tup_tlv *)((uintptr_t)param->tlv +
                  param->tlv->length);
                param->tlv = NULL;
                for (; (uintptr_t)tlv <
                     (uintptr_t)parser->vf + parser->fh->vf_size;
                     tlv = (tup_tlv *)((uintptr_t)tlv + tlv->length)) {
                    if ((tlv->type & TUP_ID_MASK_T) == id) {
                        param->tlv = tlv;
                        break;
                    }
                }
            }
        }
    } else {
        if (NULL == param->tlv) {
            tup_tlv *tlv;
            if (param->parent_tlv->type & TUP_ID_MASK_P) {
                tup_tlpv *tmp = (tup_tlpv *)param->parent_tlv;
                tlv = (tup_tlv *)&tmp->value[tmp->padding];
            } else {
                tlv = (tup_tlv *)param->parent_tlv->value;
            }
            for (; (uintptr_t)tlv <
                 (uintptr_t)param->parent_tlv + param->parent_tlv->length;
                 tlv = (tup_tlv *)((uintptr_t)tlv + tlv->length)) {
                if ((tlv->type & TUP_ID_MASK_T) == id) {
                    param->tlv = tlv;
                    break;
                }
            }
        } else {
            if ((uintptr_t)param->parent_tlv + param->parent_tlv->length <=
              (uintptr_t)param->tlv + param->tlv->length) {
                param->tlv = NULL;
            } else {
                tup_tlv *tlv = (tup_tlv *)((uintptr_t)param->tlv +
                  param->tlv->length);
                param->tlv = NULL;
                for (; (uintptr_t)tlv <
                     (uintptr_t)param->parent_tlv + param->parent_tlv->length;
                     tlv = (tup_tlv *)((uintptr_t)tlv + tlv->length)) {
                    if ((tlv->type & TUP_ID_MASK_T) == id) {
                        param->tlv = tlv;
                        break;
                    }
                }
            }
        }
    }

    return 0;
}

int
tup_get_inner_package_info(tup_parser *parser, uint32_t index, tup_tlv **pptlv)
{
    if (NULL == parser) {
        LOG_ERROR("Invalid argument: parser=%p", parser);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == pptlv) {
        LOG_ERROR("Invalid argument: pptlv=%p", pptlv);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    int ret = tup_validate(parser);
    if (0 != ret) {
        LOG_ERROR("tup_validate(): 0x%08x", ret);
        return ret;
    }

    if (parser->fh->ipkgnum <= index) {
        LOG_ERROR("Invalid argument: index=%u", index);
        return TUP_PARSER_ERROR_OUT_OF_RANGE;
    }

    if (NULL == parser->vf) {
        uint8_t *vfblock = (uint8_t *)malloc(parser->fh->vf_size);
        if (NULL == vfblock) {
            LOG_ERROR(
              "Memory allocation: request size=%zu", parser->fh->vf_size);
            return TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY;
        }

        ret = tup_pread(
          parser, vfblock, parser->fh->vf_size, parser->fh->vf_offset);
        if (0 != ret) {
            LOG_ERROR("tup_pread(): 0x%08x", ret);
            free(vfblock);
            return ret;
        }

        parser->vf = (tup_tlv *)vfblock;
    }

    *pptlv = NULL;
    tup_tlv *tlv;
    uint32_t count = 0;
    for (tlv = parser->vf;
         tlv < (tup_tlv *)((char *)parser->vf + parser->fh->vf_size);
         tlv = (tup_tlv *)((char *)tlv + tlv->length)) {
        if ((tlv->type & TUP_ID_MASK_T) == TUP_ID_IPKGINFO) {
            if (count++ == index) {
                *pptlv = tlv;
                break;
            }
        }
    }

    return 0;
}

int
tup_verify_patched_data(tup_parser *parser, const uint8_t *data, uint64_t len,
  uint64_t offset, tup_tlv *ptlv)
{
    if (NULL == parser) {
        LOG_ERROR("Invalid argument: parser=%p", parser);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == data) {
        LOG_ERROR("Invalid argument: data=%p", data);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == ptlv) {
        LOG_ERROR("Invalid argument: ptlv=%p", ptlv);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    int ret;
    uint16_t type;
    ret = tup_get_tlv_type(ptlv, &type);
    if (0 != ret) {
        LOG_ERROR("tup_get_tlv_type(): 0x%08x", ret);
        return ret;
    }

    if ((type & TUP_ID_MASK_T) != TUP_ID_ICV_AFTER_DATA) {
        LOG_ERROR("Invalid argument: type=0x%08x", type & TUP_ID_MASK_T);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    tup_tlv *icv = (tup_tlv *)((tup_tlv_icv_after_data *)ptlv)->icv;
    ret = tup_get_tlv_type(icv, &type);
    if (0 != ret) {
        LOG_ERROR("tup_get_tlv_type(): 0x%08x", ret);
        return ret;
    }

    type &= TUP_ID_MASK_T;
    if (TUP_ID_ICV_TREE == type) {
        ret = tup_verify_icv_tree(parser, icv, data, len, offset);
    } else if (TUP_ID_ICV_BLOCK == type) {
        ret = tup_verify_icv_block(parser, icv, data, len, offset);
    } else {
        LOG_ERROR("Invalid argument: type=0x%08x", type);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    return ret;
}

int
tup_get_tlv_type(const tup_tlv *ptlv, uint16_t *type)
{
    if (NULL == ptlv) {
        LOG_ERROR("Invalid argument: ptlv=%p", ptlv);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == type) {
        LOG_ERROR("Invalid argument: type=%p", type);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    *type = ptlv->type;

    return 0;
}

int
tup_get_tlv_length(const tup_tlv *ptlv, uint64_t *length)
{
    if (NULL == ptlv) {
        LOG_ERROR("Invalid argument: ptlv=%p", ptlv);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == length) {
        LOG_ERROR("Invalid argument: length=%p", length);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    *length = ptlv->length;

    return 0;
}

int
tup_get_tlv_value_size(const tup_tlv *ptlv, uint64_t *value_size)
{
    if (NULL == ptlv) {
        LOG_ERROR("Invalid argument: ptlv=%p", ptlv);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == value_size) {
        LOG_ERROR("Invalid argument: value_size=%p", value_size);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (ptlv->type & TUP_ID_MASK_P)
        *value_size = ptlv->length - sizeof(tup_tlv) - sizeof(uint32_t) -
          ((tup_tlpv *)ptlv)->padding;
    else
        *value_size = ptlv->length - sizeof(tup_tlv);

    return 0;
}

int
tup_get_tlv_value(
  const tup_tlv *ptlv, uint64_t offset, void *buffer, uint64_t buffer_size)
{
    if (NULL == ptlv) {
        LOG_ERROR("Invalid argument: ptlv=%p", ptlv);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == buffer) {
        LOG_ERROR("Invalid argument: buffer=%p", buffer);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    uint64_t size;
    if (ptlv->type & TUP_ID_MASK_P)
        size = ptlv->length - sizeof(tup_tlv) - sizeof(uint32_t) -
          ((tup_tlpv *)ptlv)->padding;
    else
        size = ptlv->length - sizeof(tup_tlv);
    if (size < offset) {
        LOG_ERROR(
          "Invalid argument: offset=%" PRIu64 ", size=%" PRIu64, offset, size);
        return TUP_PARSER_ERROR_OUT_OF_RANGE;
    }

    if (size - offset < buffer_size) {
        LOG_ERROR("Invalid argument: buffer_size=%" PRIu64, buffer_size);
        return TUP_PARSER_ERROR_OUT_OF_RANGE;
    }

    if (ptlv->type & TUP_ID_MASK_P)
        memcpy(buffer,
          &((tup_tlpv *)ptlv)->value[((tup_tlpv *)ptlv)->padding + offset],
          buffer_size);
    else
        memcpy(buffer, &ptlv->value[offset], buffer_size);

    return 0;
}

const char *
tup_get_id_name(uint16_t id)
{
    switch (id) {
    case TUP_ID_INVALID:
        return "INVALID";
    case TUP_ID_INDEX:
        return "INDEX";
    case TUP_ID_POS:
        return "POS";
    case TUP_ID_ICV_TREE:
        return "ICV_TREE";
    case TUP_ID_ICV_ARRAY:
        return "ICV_ARRAY";
    case TUP_ID_IPKGINFO:
        return "IPKGINFO";
    case TUP_ID_IPKGNAME:
        return "IPKGNAME";
    case TUP_ID_COMP_ALL:
        return "COMP_ALL";
    case TUP_ID_COMP_FIXED:
        return "COMP_FIXED";
    case TUP_ID_COMP_VAR:
        return "COMP_VAR";
    case TUP_ID_CRYPT:
        return "CRYPT";
    case TUP_ID_UPDATEFLOW:
        return "UPDATEFLOW";
    case TUP_ID_VERSION:
        return "VERSION";
    case TUP_ID_DOMAIN:
        return "DOMAIN";
    case TUP_ID_ORDER:
        return "ORDER";
    case TUP_ID_DELTA_PATCH:
        return "DELTA_PATCH";
    case TUP_ID_ICV_AFTER_DATA:
        return "ICV_AFTER_DATA";
    case TUP_ID_ICV_BLOCK:
        return "ICV_BLOCK";
    case TUP_ID_TLV_PLAIN:
        return "TLV_PLAIN";
    case TUP_ID_TLV_VF:
        return "TLV_VF";
    case TUP_ID_TLV_VH:
        return "TLV_VH";
    case TUP_ID_FIXED_FF:
        return "FIXED_FF";
    case TUP_ID_FIXED_VH:
        return "FIXED_VH";
    case TUP_ID_FIXED_FH:
        return "FIXED_FH";
    default:
        break;
    }
    return NULL;
}
