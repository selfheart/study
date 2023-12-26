/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

enum tus_delta_update_algorithm {
    TUS_DELTA_UPDATE_ALGORITHM_BSDIFF = 0,
    // add TUS_DELTA_UPDATE_ALGORITHM_...,

    TUS_DELTA_UPDATE_ALGORITHM__LIMIT,
};
_Static_assert(TUS_DELTA_UPDATE_ALGORITHM__LIMIT <= 256,
  "enum tus_delta_update_algorithm must be compatible with uint8_t");

enum tus_delta_update_state {
    TUS_DELTA_UPDATE_STATE_INVALID = 0,
    TUS_DELTA_UPDATE_STATE_CANCELED,
    TUS_DELTA_UPDATE_STATE_FINISHED,
    TUS_DELTA_UPDATE_STATE_INITIALIZED,
    TUS_DELTA_UPDATE_STATE_PATCH_CONT,
    TUS_DELTA_UPDATE_STATE_PATCH_DONE,
    TUS_DELTA_UPDATE_STATE_SKIP_CONT,
    TUS_DELTA_UPDATE_STATE_SKIP_DONE,
    TUS_DELTA_UPDATE_STATE_RESUMED,
    TUS_DELTA_UPDATE_STATE_RESUMED_SKIP,
    TUS_DELTA_UPDATE_STATE_SUSPENDED,

    TUS_DELTA_UPDATE_STATE__LIMIT
};
_Static_assert(TUS_DELTA_UPDATE_STATE__LIMIT <= 256,
  "enum tus_delta_update_state must be compatible with uint8_t");

enum tus_delta_update_return_value {
    TUS_DELTA_UPDATE_PATCH_OK = 0,
    TUS_DELTA_UPDATE_PATCH_ERROR,
    TUS_DELTA_UPDATE_PATCH_DONE,
    TUS_DELTA_UPDATE_PATCH_DONE_REMAINING,
    TUS_DELTA_UPDATE_PATCH_PATCHED_FULL,
    TUS_DELTA_UPDATE_PATCH_SRC_SEEK,
    TUS_DELTA_UPDATE_PATCH_DELTA_NEXT,
    TUS_DELTA_UPDATE_PATCH_SKIP,
};

#define TUS_DELTA_UPDATE_STREAM_SIZE ((size_t)256)
typedef struct tus_delta_update_stream {
    int64_t struct_size;
    int64_t save_size;

    uint8_t changed;
    uint8_t algorithm_id; // enum tus_delta_update_algorithm
    uint8_t state;        // enum tus_delta_update_state
    uint8_t padding[5];

    struct {
        struct {
            int64_t size;
            int64_t pos;
        } buf; // states of a corresponding buf_*
        int64_t total_pos;
    } src, patched, delta;

    int64_t total_patched_size; // expected total size of patched data
    int64_t skip_patched_pos;

    uint8_t data[TUS_DELTA_UPDATE_STREAM_SIZE - (14 * sizeof(int64_t)) -
      (3 * sizeof(void *))];

    uint8_t *buf_src;
    uint8_t *buf_patched;
    uint8_t *buf_delta;
} tus_delta_update_stream;
_Static_assert(sizeof(void *) <= 8, "");
_Static_assert(offsetof(struct tus_delta_update_stream, src) == 24, "");
_Static_assert(sizeof(struct tus_delta_update_stream) ==
    TUS_DELTA_UPDATE_STREAM_SIZE,
  "");

extern int tus_delta_update_initialize(uint8_t *buf_delta, int64_t delta_size,
  int64_t src_size, int64_t patched_size, uint8_t algorithm_id,
  tus_delta_update_stream *stream_p);
extern int tus_delta_update_cancel(tus_delta_update_stream *stream_p);
extern int tus_delta_update_finalize(tus_delta_update_stream *stream_p);
extern int tus_delta_update_patch(tus_delta_update_stream *stream_p);
extern int tus_delta_update_suspend(tus_delta_update_stream *stream_p,
  uint8_t *buf_save);
extern int tus_delta_update_resume(tus_delta_update_stream *stream_p,
  uint8_t *buf_save);
extern int tus_delta_update_skip(tus_delta_update_stream *stream_p,
  int64_t skip_patched_pos);
