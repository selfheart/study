/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "logger.h"

#include "delta_update.h"
#include "delta_update_internal.h"
#include "delta_update_bsdiff.h"

#define STREAM_SAVE_SIZE                        \
    ((int64_t)offsetof(tus_delta_update_stream, \
      buf_src)) /* except last pointers */

#if 0
static struct {
    uint8_t algorithm_id;
    tus_delta_update_funcs *funcs;
} id_funcs[] = {
    { TUS_DELTA_UPDATE_ALGORITHM_BSDIFF, &tus_delta_update_bsdiff_funcs },
    /* ... */
};
static tus_delta_update_funcs *
get_funcs(uint8_t algorithm_id)
{
    int i;
    for (i = 0; i < sizeof(id_funcs) / sizeof(*id_funcs); i++) {
        if (algorithm_id == id_funcs[i].algorithm_id) {
            break;
        }
    }
    if (i >= sizeof(id_funcs) / sizeof(*id_funcs)) {
        return NULL;
    }
    return id_funcs[i].funcs;
}
#else
static tus_delta_update_funcs *id_funcs[] = {
    &tus_delta_update_bsdiff_funcs, /* TUS_DELTA_UPDATE_ALGORITHM_BSDIFF */
    /* ... */
};
#define get_funcs(algorithm_id)                                    \
    ((uint8_t)(algorithm_id) < TUS_DELTA_UPDATE_ALGORITHM__LIMIT ? \
        id_funcs[algorithm_id] :                                   \
        NULL)
#endif

int
tus_delta_update_initialize(uint8_t *buf_delta, int64_t delta_size,
  int64_t src_size, int64_t patched_size, uint8_t algorithm_id,
  tus_delta_update_stream *stream_p)
{
    int r;
    tus_delta_update_funcs *funcs;

    if (buf_delta == NULL) {
        LOG_ERROR("invalid argument 'buf_delta' (%p)", buf_delta);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    if (delta_size <= 0) {
        LOG_ERROR("invalid argument 'delta_size' (%" PRId64 ")", delta_size);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    if (src_size <= 0) {
        LOG_ERROR("invalid argument 'src_size' (%" PRId64 ")", src_size);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    if (patched_size <= 0) {
        LOG_ERROR("invalid argument 'patched_size' (%" PRId64 ")",
          patched_size);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    if (stream_p == NULL) {
        LOG_ERROR("invalid argument 'stream_p' (%p)", stream_p);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    stream_p->struct_size = sizeof(tus_delta_update_stream);
    stream_p->save_size = STREAM_SAVE_SIZE + patched_size;

    stream_p->buf_delta = buf_delta;
    stream_p->delta.buf.size = delta_size;
    stream_p->src.buf.size = src_size;
    stream_p->patched.buf.size = patched_size;
    stream_p->patched.total_pos = stream_p->src.total_pos =
      stream_p->delta.total_pos = 0;
    stream_p->skip_patched_pos = 0;
    stream_p->patched.buf.pos = stream_p->src.buf.pos =
      stream_p->delta.buf.pos = 0;
    stream_p->buf_patched = stream_p->buf_src = NULL;
    stream_p->changed = 0;
    stream_p->algorithm_id = algorithm_id;

    funcs = get_funcs(stream_p->algorithm_id);
    if (funcs == NULL || funcs->initialize == NULL) {
        LOG_ERROR("can't get algorithm dependent function (%p)", funcs);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    r = funcs->initialize(stream_p);
    if (r != TUS_DELTA_UPDATE_PATCH_OK) {
        return r;
    }

    stream_p->state = TUS_DELTA_UPDATE_STATE_INITIALIZED;
    return TUS_DELTA_UPDATE_PATCH_OK;
}

static int
check_struct(tus_delta_update_stream *stream_p)
{
    int r = 0;
    if (stream_p->struct_size != sizeof(tus_delta_update_stream)) {
        LOG_WARN("invalid 'struct_size' (%" PRId64 ")", stream_p->struct_size);
        r++;
    }
    if (stream_p->save_size < STREAM_SAVE_SIZE + stream_p->patched.buf.size) {
        LOG_WARN("too small 'save_size' (%" PRId64 ")", stream_p->save_size);
        r++;
    }
    if (stream_p->src.buf.size <= 0) {
        LOG_WARN("'src.buf.size' is out of range (%" PRId64 ")",
          stream_p->src.buf.size);
        r++;
    }
    if (stream_p->patched.buf.size <= 0) {
        LOG_WARN("'patched.buf.size' is out of range (%" PRId64 ")",
          stream_p->patched.buf.size);
        r++;
    }
    /* not check src.buf.pos, because may be seek */
    if (stream_p->patched.buf.pos < 0 ||
      stream_p->patched.buf.pos > stream_p->patched.buf.size) {
        LOG_WARN("'patched.buf.pos' is out of range (%" PRId64 ")",
          stream_p->patched.buf.pos);
        r++;
    }
    if (stream_p->delta.buf.size <= 0) {
        LOG_WARN("'delta.buf.size' is out of range (%" PRId64 ")",
          stream_p->delta.buf.size);
        r++;
    }
    if (stream_p->delta.buf.pos < 0 ||
      stream_p->delta.buf.pos > stream_p->delta.buf.size) {
        LOG_WARN("'delta.buf.pos' is out of range (%" PRId64 ")",
          stream_p->delta.buf.pos);
        r++;
    }
    if (stream_p->total_patched_size < 0) {
        LOG_WARN("'total_patched_size' is out of range (%" PRId64 ")",
          stream_p->total_patched_size);
        r++;
    }
    if (stream_p->patched.total_pos < 0 ||
      stream_p->patched.total_pos > stream_p->total_patched_size) {
        LOG_WARN("'patched.total_pos' is out of range (%" PRId64 ")",
          stream_p->patched.total_pos);
        r++;
    }
    if (stream_p->src.total_pos < 0) {
        LOG_WARN("'src.total_pos' is out of range (%" PRId64 ")",
          stream_p->src.total_pos);
        r++;
    }
    if (stream_p->delta.total_pos < 0) {
        LOG_WARN("'delta.total_pos' is out of range (%" PRId64 ")",
          stream_p->delta.total_pos);
        r++;
    }
    if (stream_p->skip_patched_pos < 0) {
        LOG_WARN("'skip_patched_pos' is out of range (%" PRId64 ")",
          stream_p->skip_patched_pos);
        r++;
    }
    if (stream_p->changed > 1) {
        LOG_WARN("'changed' is out of range (%d)", stream_p->changed);
        r++;
    }
    if (stream_p->algorithm_id >= TUS_DELTA_UPDATE_ALGORITHM__LIMIT) {
        LOG_WARN("'algorithm_id' is out of range (%d)", stream_p->algorithm_id);
        r++;
    }
    if (stream_p->state >= TUS_DELTA_UPDATE_STATE__LIMIT) {
        LOG_WARN("'state' is out of range (%d)", stream_p->state);
        r++;
    }
    return r;
}

int
tus_delta_update_cancel(tus_delta_update_stream *stream_p)
{
    int r;
    tus_delta_update_funcs *funcs;

    if (stream_p == NULL) {
        LOG_ERROR("invalid argument 'stream_p' (%p)", stream_p);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    if (stream_p->state == TUS_DELTA_UPDATE_STATE_SUSPENDED ||
      stream_p->state == TUS_DELTA_UPDATE_STATE_FINISHED) {
        LOG_ERROR("invalid 'state' (%d)", stream_p->state);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }

    check_struct(stream_p);

    funcs = get_funcs(stream_p->algorithm_id);
    if (funcs == NULL || funcs->finalize == NULL) {
        LOG_ERROR("can't get algorithm dependent function (%p)", funcs);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    r = funcs->cancel(stream_p);
    if (r != TUS_DELTA_UPDATE_PATCH_OK) {
        return r;
    }

    stream_p->state = TUS_DELTA_UPDATE_STATE_CANCELED;
    return TUS_DELTA_UPDATE_PATCH_OK;
}

int
tus_delta_update_finalize(tus_delta_update_stream *stream_p)
{
    int r;
    tus_delta_update_funcs *funcs;

    if (stream_p == NULL) {
        LOG_ERROR("invalid argument 'stream_p' (%p)", stream_p);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    if (stream_p->state != TUS_DELTA_UPDATE_STATE_CANCELED) {
        check_struct(stream_p);

        /* check patch done */
        if (stream_p->patched.total_pos != stream_p->total_patched_size) {
            LOG_WARN("patched.total_pos(%" PRId64
                     ") != total_patched_size(%" PRId64 ")",
              stream_p->patched.total_pos, stream_p->total_patched_size);
        }
        if (stream_p->state != TUS_DELTA_UPDATE_STATE_PATCH_DONE) {
            LOG_WARN("not a TUS_DELTA_UPDATE_STATE_PATCH_DONE (state=%d)",
              stream_p->state);
        }
    }

    funcs = get_funcs(stream_p->algorithm_id);
    if (funcs == NULL || funcs->finalize == NULL) {
        LOG_ERROR("can't get algorithm dependent function (%p)", funcs);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    r = funcs->finalize(stream_p);
    if (r != TUS_DELTA_UPDATE_PATCH_OK) {
        return r;
    }

    stream_p->state = TUS_DELTA_UPDATE_STATE_FINISHED;
    return TUS_DELTA_UPDATE_PATCH_OK;
}

int
tus_delta_update_patch(tus_delta_update_stream *stream_p)
{
    int r;
    tus_delta_update_funcs *funcs;

    if (stream_p == NULL) {
        LOG_ERROR("invalid argument 'stream_p' (%p)", stream_p);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    if (stream_p->state != TUS_DELTA_UPDATE_STATE_INITIALIZED &&
      stream_p->state != TUS_DELTA_UPDATE_STATE_PATCH_CONT &&
      stream_p->state != TUS_DELTA_UPDATE_STATE_RESUMED &&
      stream_p->state != TUS_DELTA_UPDATE_STATE_SKIP_DONE) {
        LOG_ERROR("invalid 'state' (%d)", stream_p->state);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }

    funcs = get_funcs(stream_p->algorithm_id);
    if (funcs == NULL || funcs->patch == NULL) {
        LOG_ERROR("can't get algorithm dependent function (%p)", funcs);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    r = funcs->patch(stream_p);
    if (r != TUS_DELTA_UPDATE_PATCH_OK) {
        return r;
    }

    if (stream_p->patched.total_pos == stream_p->total_patched_size) {
        stream_p->state = TUS_DELTA_UPDATE_STATE_PATCH_DONE;
        if (stream_p->patched.buf.pos != 0) {
            return TUS_DELTA_UPDATE_PATCH_DONE_REMAINING;
        }
        return TUS_DELTA_UPDATE_PATCH_DONE;
    } else if (stream_p->patched.total_pos > stream_p->total_patched_size) {
        LOG_ERROR("invalid 'patched.total_pos' (%" PRId64 ")",
          stream_p->patched.total_pos);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    if (stream_p->buf_patched == NULL ||
      stream_p->patched.buf.pos == stream_p->patched.buf.size) {
        stream_p->state = TUS_DELTA_UPDATE_STATE_PATCH_CONT;
        return TUS_DELTA_UPDATE_PATCH_PATCHED_FULL;
    } else if (stream_p->patched.buf.pos > stream_p->patched.buf.size) {
        LOG_ERROR("invalid 'patched.buf.pos' (%" PRId64 ")",
          stream_p->patched.buf.pos);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    if (stream_p->buf_src == NULL || stream_p->src.buf.pos < 0 ||
      stream_p->src.buf.pos >= stream_p->src.buf.size) {
        stream_p->state = TUS_DELTA_UPDATE_STATE_PATCH_CONT;
        return TUS_DELTA_UPDATE_PATCH_SRC_SEEK;
    }
    if (stream_p->delta.buf.pos == stream_p->delta.buf.size) {
        stream_p->state = TUS_DELTA_UPDATE_STATE_PATCH_CONT;
        return TUS_DELTA_UPDATE_PATCH_DELTA_NEXT;
    }
    LOG_ERROR("invalid internal state of stream (%p)", stream_p);
    return TUS_DELTA_UPDATE_PATCH_ERROR;
}

int
tus_delta_update_suspend(tus_delta_update_stream *stream_p, uint8_t *buf_save)
{
    int r;
    tus_delta_update_funcs *funcs;

    if (stream_p == NULL) {
        LOG_ERROR("invalid argument 'stream_p' (%p)", stream_p);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    if (stream_p->state != TUS_DELTA_UPDATE_STATE_INITIALIZED &&
      stream_p->state != TUS_DELTA_UPDATE_STATE_PATCH_CONT &&
      stream_p->state != TUS_DELTA_UPDATE_STATE_RESUMED &&
      stream_p->state != TUS_DELTA_UPDATE_STATE_SKIP_CONT &&
      stream_p->state != TUS_DELTA_UPDATE_STATE_SKIP_DONE) {
        LOG_ERROR("invalid 'state' (%d)", stream_p->state);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }

    if (buf_save == NULL) {
        LOG_ERROR("invalid argument 'buf_save' (%p)", buf_save);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }

    funcs = get_funcs(stream_p->algorithm_id);
    if (funcs == NULL || funcs->suspend == NULL) {
        LOG_ERROR("can't get algorithm dependent function (%p)", funcs);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    r = funcs->suspend(stream_p,
      buf_save + sizeof(tus_delta_update_stream) + stream_p->patched.buf.size);
    if (r != TUS_DELTA_UPDATE_PATCH_OK) {
        return r;
    }
    stream_p->state = TUS_DELTA_UPDATE_STATE_SUSPENDED;
    memcpy(buf_save, stream_p, STREAM_SAVE_SIZE);
    if (stream_p->buf_patched != NULL) {
        memcpy(buf_save + STREAM_SAVE_SIZE, stream_p->buf_patched,
          stream_p->patched.buf.size);
    }
    return TUS_DELTA_UPDATE_PATCH_OK;
}

int
tus_delta_update_resume(tus_delta_update_stream *stream_p, uint8_t *buf_save)
{
    int r;
    tus_delta_update_funcs *funcs;
    tus_delta_update_stream *saved_stream = (tus_delta_update_stream *)buf_save;

    if (stream_p == NULL) {
        LOG_ERROR("invalid argument 'stream_p' (%p)", stream_p);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    if (buf_save == NULL) {
        LOG_ERROR("invalid argument 'buf_save' (%p)", buf_save);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }

    if (saved_stream->state != TUS_DELTA_UPDATE_STATE_SUSPENDED) {
        LOG_ERROR("invalid 'state' (%d)", stream_p->state);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    memcpy(stream_p, saved_stream, STREAM_SAVE_SIZE);
    stream_p->buf_patched = buf_save + STREAM_SAVE_SIZE;
    stream_p->buf_src = stream_p->buf_delta = NULL;

    funcs = get_funcs(stream_p->algorithm_id);
    if (funcs == NULL || funcs->resume == NULL) {
        LOG_ERROR("can't get algorithm dependent function (%p)", funcs);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    r = funcs->resume(stream_p,
      buf_save + sizeof(tus_delta_update_stream) + stream_p->patched.buf.size);
    if (r != TUS_DELTA_UPDATE_PATCH_OK) {
        return r;
    }
    if (stream_p->skip_patched_pos != 0) {
        stream_p->state = TUS_DELTA_UPDATE_STATE_RESUMED_SKIP;
        return TUS_DELTA_UPDATE_PATCH_SKIP;
    }
    stream_p->state = TUS_DELTA_UPDATE_STATE_RESUMED;
    return TUS_DELTA_UPDATE_PATCH_OK;
}

int
tus_delta_update_skip(tus_delta_update_stream *stream_p,
  int64_t skip_patched_pos)
{
    int r;
    tus_delta_update_funcs *funcs;

    if (stream_p == NULL) {
        LOG_ERROR("invalid argument 'stream_p' (%p)", stream_p);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    if (stream_p->state != TUS_DELTA_UPDATE_STATE_INITIALIZED &&
      stream_p->state != TUS_DELTA_UPDATE_STATE_SKIP_CONT &&
      stream_p->state != TUS_DELTA_UPDATE_STATE_RESUMED_SKIP) {
        LOG_ERROR("invalid 'state' (%d)", stream_p->state);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    if (skip_patched_pos != 0) {
        stream_p->skip_patched_pos = skip_patched_pos;
    }

    funcs = get_funcs(stream_p->algorithm_id);
    if (funcs == NULL || funcs->skip == NULL) {
        LOG_ERROR("can't get algorithm dependent function (%p)", funcs);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    r = funcs->skip(stream_p);
    if (r != TUS_DELTA_UPDATE_PATCH_OK) {
        return r;
    }

    if (stream_p->patched.total_pos >= stream_p->total_patched_size) {
        LOG_ERROR("invalid 'patched.total_pos' (%" PRId64 ")",
          stream_p->patched.total_pos);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    if (stream_p->patched.total_pos == stream_p->skip_patched_pos) {
        stream_p->skip_patched_pos = 0;
        stream_p->state = TUS_DELTA_UPDATE_STATE_SKIP_DONE;
        return TUS_DELTA_UPDATE_PATCH_OK;
    } else if (stream_p->patched.total_pos > stream_p->skip_patched_pos) {
        LOG_ERROR("invalid 'patched.total_pos' (%" PRId64 ")",
          stream_p->patched.total_pos);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    if (stream_p->delta.buf.pos == stream_p->delta.buf.size) {
        stream_p->state = TUS_DELTA_UPDATE_STATE_SKIP_CONT;
        return TUS_DELTA_UPDATE_PATCH_DELTA_NEXT;
    }
    LOG_ERROR("invalid internal state of stream (%p)", stream_p);
    return TUS_DELTA_UPDATE_PATCH_ERROR;
}
