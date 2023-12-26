/*-
 * Copyright 2003-2005 Colin Percival
 * Copyright 2012 Matthew Endsley
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"

#include "delta_update.h"
#include "delta_update_internal.h"
#include "delta_update_bsdiff.h"
#include "delta_update_bsdiff_internal.h"

static int64_t
offtin(uint8_t *buf)
{
    int64_t y;

    y = buf[7] & 0x7F;
    y = y << 8;
    y += buf[6];
    y = y << 8;
    y += buf[5];
    y = y << 8;
    y += buf[4];
    y = y << 8;
    y += buf[3];
    y = y << 8;
    y += buf[2];
    y = y << 8;
    y += buf[1];
    y = y << 8;
    y += buf[0];

    if (buf[7] & 0x80)
        y = -y;

    return y;
}

static int
bsdiff_initialize(tus_delta_update_stream *stream_p)
{
    int64_t newsize;
    bsdiff_stream *bsdiff_data;

    if (stream_p->delta.buf.size <
      (int64_t)BSDIFF_MAGIC_SIZE + BSDIFF_NEWSIZE_SIZE) {
        LOG_ERROR("'delta.buf.size' is too small (%" PRId64 ")",
          stream_p->delta.buf.size);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    /* Check the magic at the beginning of difference data */
    if (memcmp(stream_p->buf_delta, BSDIFF_MAGIC_STRING, BSDIFF_MAGIC_SIZE) !=
      0) {
        int i;
        char s[BSDIFF_MAGIC_SIZE * 3];
        for (i = 0; i < (int)BSDIFF_MAGIC_SIZE; i++) {
            sprintf(&(s[i * 3]), "%02x ", stream_p->buf_delta[i]);
        }
        s[BSDIFF_MAGIC_SIZE * 3 - 1] = '\0';
        LOG_ERROR("'buf_delta' is not a BSDIFF data (%s)", s);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    /* Read newsize (8 bytes) */
    newsize = offtin(stream_p->buf_delta + BSDIFF_MAGIC_SIZE);
    if (newsize < 0) {
        LOG_ERROR("invalid newsize in 'buf_delta' (%" PRId64 ")", newsize);
        return TUS_DELTA_UPDATE_PATCH_ERROR;
    }
    stream_p->total_patched_size = newsize;
    stream_p->delta.buf.pos += (BSDIFF_MAGIC_SIZE + BSDIFF_NEWSIZE_SIZE);
    stream_p->delta.total_pos += (BSDIFF_MAGIC_SIZE + BSDIFF_NEWSIZE_SIZE);

    bsdiff_data = (bsdiff_stream *)stream_p->data;
    bsdiff_data->add = bsdiff_data->insert = bsdiff_data->seek = 0;
    bsdiff_data->hold[0] = 0;

    return TUS_DELTA_UPDATE_PATCH_OK;
}

static int
check_bsdiff_struct(tus_delta_update_stream *stream_p)
{
    int r = 0;
    bsdiff_stream *bsdiff_data = (bsdiff_stream *)stream_p->data;

    if (bsdiff_data->add < 0 ||
      stream_p->patched.total_pos + bsdiff_data->add >
        stream_p->total_patched_size) {
        LOG_WARN("'add' is out of range (%" PRId64 ")", bsdiff_data->add);
        r++;
    }
    if (bsdiff_data->insert < 0 ||
      stream_p->patched.total_pos + bsdiff_data->insert >
        stream_p->total_patched_size) {
        LOG_WARN("'insert' is out of range (%" PRId64 ")", bsdiff_data->insert);
        r++;
    }
    if (stream_p->src.total_pos + bsdiff_data->seek < 0) {
        LOG_WARN("'seek' is out of range (%" PRId64 ")", bsdiff_data->seek);
        r++;
    }
    if (bsdiff_data->hold[0] >= BSDIFF_CTRL_SIZE) {
        LOG_WARN("'hold[0]' is out of range (%u)", bsdiff_data->hold[0]);
        r++;
    }
    return r;
}

static int
bsdiff_cancel(tus_delta_update_stream *stream_p)
{
    check_bsdiff_struct(stream_p);
    return TUS_DELTA_UPDATE_PATCH_OK;
}

static int
bsdiff_finalize(tus_delta_update_stream *stream_p)
{
    if (stream_p->state != TUS_DELTA_UPDATE_STATE_CANCELED) {
        bsdiff_stream *bsdiff_data = (bsdiff_stream *)stream_p->data;
        check_bsdiff_struct(stream_p);
        if (bsdiff_data->add != 0) {
            LOG_WARN("'add' is not zero  (%" PRId64 ")", bsdiff_data->add);
        }
        if (bsdiff_data->insert != 0) {
            LOG_WARN("'insert' is not zero  (%" PRId64 ")",
              bsdiff_data->insert);
        }
        if (bsdiff_data->seek != 0) {
            LOG_WARN("'seek' is not zero  (%" PRId64 ")", bsdiff_data->seek);
        }
        if (bsdiff_data->hold[0] != 0) {
            LOG_WARN("'hold[0]' is not zero  (%u)", bsdiff_data->hold[0]);
        }
    }
    return TUS_DELTA_UPDATE_PATCH_OK;
}

static int
bsdiff_patch(tus_delta_update_stream *stream_p)
{
    int64_t i;
    bsdiff_stream *bsdiff_data;

    bsdiff_data = (bsdiff_stream *)stream_p->data;

    if (stream_p->delta.buf.pos >= stream_p->delta.buf.size) {
        /* no delta */
        return TUS_DELTA_UPDATE_PATCH_OK;
    }

    if (bsdiff_data->hold[0] != 0) {
        uint8_t ctrl_buf[BSDIFF_CTRL_SIZE];
        int64_t rest = BSDIFF_CTRL_SIZE - bsdiff_data->hold[0];

        /* ASSUME: delta.buf.size >= BSDIFF_CTRL_SIZE */
        memcpy(ctrl_buf, &bsdiff_data->hold[1], bsdiff_data->hold[0]);
        memcpy(ctrl_buf + bsdiff_data->hold[0], stream_p->buf_delta, rest);
        bsdiff_data->add = offtin(ctrl_buf);
        if (bsdiff_data->add < 0 ||
          stream_p->patched.total_pos + bsdiff_data->add >
            stream_p->total_patched_size) {
            LOG_ERROR("invalid ADD value (%" PRId64 ")", bsdiff_data->add);
            return TUS_DELTA_UPDATE_PATCH_ERROR;
        }
        bsdiff_data->insert = offtin(ctrl_buf + 8);
        if (bsdiff_data->insert < 0 ||
          stream_p->patched.total_pos + bsdiff_data->add + bsdiff_data->insert >
            stream_p->total_patched_size) {
            LOG_ERROR("invalid INSERT value (%" PRId64 ")",
              bsdiff_data->insert);
            return TUS_DELTA_UPDATE_PATCH_ERROR;
        }
        bsdiff_data->seek = offtin(
          ctrl_buf + 16); /* can't check out of range here */

        stream_p->delta.buf.pos = rest;
        stream_p->delta.total_pos += rest;
        bsdiff_data->hold[0] = 0;
    }
    while (stream_p->patched.total_pos < stream_p->total_patched_size) {
        if (bsdiff_data->add > 0) {
            /* ADD */
            int64_t add = bsdiff_data->add;
            if (stream_p->buf_src == NULL || stream_p->buf_patched == NULL) {
                /* cannot ADD */
                break;
            }
            if (stream_p->delta.buf.pos + add > stream_p->delta.buf.size) {
                add = stream_p->delta.buf.size - stream_p->delta.buf.pos;
            }
            if (stream_p->src.buf.pos + add > stream_p->src.buf.size) {
                add = stream_p->src.buf.size - stream_p->src.buf.pos;
            }
            if (stream_p->patched.buf.pos + add > stream_p->patched.buf.size) {
                add = stream_p->patched.buf.size - stream_p->patched.buf.pos;
            }
            for (i = 0; i < add; i++) {
                uint8_t diff = stream_p->buf_delta[stream_p->delta.buf.pos + i];
                if (diff != 0) {
                    stream_p->changed = 1;
                }
                stream_p->buf_patched[stream_p->patched.buf.pos + i] =
                  stream_p->buf_src[stream_p->src.buf.pos + i] + diff;
            }
            stream_p->patched.buf.pos += add;
            stream_p->src.buf.pos += add;
            stream_p->delta.buf.pos += add;
            stream_p->patched.total_pos += add;
            stream_p->src.total_pos += add;
            stream_p->delta.total_pos += add;
            bsdiff_data->add -= add;
        }
        if (bsdiff_data->add > 0) {
            /* stop at ADD */
            break;
        }
        if (bsdiff_data->insert > 0) {
            /* INSERT */
            int64_t insert = bsdiff_data->insert;
            if (stream_p->buf_patched == NULL) {
                /* cannot INSERT */
                break;
            }
            if (stream_p->delta.buf.pos + insert > stream_p->delta.buf.size) {
                insert = stream_p->delta.buf.size - stream_p->delta.buf.pos;
            }
            if (stream_p->patched.buf.pos + insert >
              stream_p->patched.buf.size) {
                insert = stream_p->patched.buf.size - stream_p->patched.buf.pos;
            }
            for (i = 0; i < insert; i++) {
                stream_p->buf_patched[stream_p->patched.buf.pos + i] =
                  stream_p->buf_delta[stream_p->delta.buf.pos + i];
            }
            stream_p->patched.buf.pos += insert;
            stream_p->delta.buf.pos += insert;
            stream_p->patched.total_pos += insert;
            stream_p->delta.total_pos += insert;
            bsdiff_data->insert -= insert;
            stream_p->changed = 1;
        }
        if (bsdiff_data->insert > 0) {
            /* stop at INSERT */
            break;
        }
        if (bsdiff_data->seek != 0) {
            /* SEEK */
            stream_p->src.buf.pos += bsdiff_data->seek;
            stream_p->src.total_pos += bsdiff_data->seek;
            bsdiff_data->seek = 0;
            stream_p->changed = 1;
            if (stream_p->src.buf.pos < 0 ||
              stream_p->src.buf.pos >= stream_p->src.buf.size) {
                /* out of range */
                break;
            }
        }

        if (stream_p->patched.total_pos >= stream_p->total_patched_size) {
            break;
        }

        /* read ctrl part */
        if (stream_p->delta.buf.size - stream_p->delta.buf.pos <
          (int64_t)BSDIFF_CTRL_SIZE) {
            bsdiff_data->hold[0] = stream_p->delta.buf.size -
              stream_p->delta.buf.pos;
            memcpy(&(bsdiff_data->hold[1]),
              stream_p->buf_delta + stream_p->delta.buf.pos,
              bsdiff_data->hold[0]);
            stream_p->delta.buf.pos += bsdiff_data->hold[0];
            stream_p->delta.total_pos += bsdiff_data->hold[0];
            break;
        } else {
            bsdiff_data->add = offtin(
              stream_p->buf_delta + stream_p->delta.buf.pos);
            if (bsdiff_data->add < 0 ||
              stream_p->patched.total_pos + bsdiff_data->add >
                stream_p->total_patched_size) {
                LOG_ERROR("invalid ADD value (%" PRId64 ")", bsdiff_data->add);
                return TUS_DELTA_UPDATE_PATCH_ERROR;
            }
            bsdiff_data->insert = offtin(
              stream_p->buf_delta + stream_p->delta.buf.pos + 8);
            if (bsdiff_data->insert < 0 ||
              stream_p->patched.total_pos + bsdiff_data->add +
                  bsdiff_data->insert >
                stream_p->total_patched_size) {
                LOG_ERROR("invalid INSERT value (%" PRId64 ")",
                  bsdiff_data->insert);
                return TUS_DELTA_UPDATE_PATCH_ERROR;
            }
            bsdiff_data->seek = offtin(stream_p->buf_delta +
              stream_p->delta.buf.pos + 16); /* can't check out of range here */

            stream_p->delta.buf.pos += BSDIFF_CTRL_SIZE;
            stream_p->delta.total_pos += BSDIFF_CTRL_SIZE;
        }
    }

    return TUS_DELTA_UPDATE_PATCH_OK;
}

static int
bsdiff_suspend(tus_delta_update_stream *stream_p, uint8_t *buf_save)
{
    (void)stream_p; /* unused parameter */
    (void)buf_save; /* unused parameter */
    return TUS_DELTA_UPDATE_PATCH_OK;
}

static int
bsdiff_resume(tus_delta_update_stream *stream_p, uint8_t *buf_save)
{
    (void)stream_p; /* unused parameter */
    (void)buf_save; /* unused parameter */
    return TUS_DELTA_UPDATE_PATCH_OK;
}
static int
bsdiff_skip(tus_delta_update_stream *stream_p)
{
    bsdiff_stream *bsdiff_data;

    bsdiff_data = (bsdiff_stream *)stream_p->data;

    if (bsdiff_data->hold[0] != 0) {
        uint8_t ctrl_buf[BSDIFF_CTRL_SIZE];
        int64_t rest = BSDIFF_CTRL_SIZE - bsdiff_data->hold[0];

        /* ASSUME: delta.buf.size >= BSDIFF_CTRL_SIZE */
        memcpy(ctrl_buf, &bsdiff_data->hold[1], bsdiff_data->hold[0]);
        memcpy(ctrl_buf + bsdiff_data->hold[0], stream_p->buf_delta, rest);
        bsdiff_data->add = offtin(ctrl_buf);
        if (bsdiff_data->add < 0 ||
          stream_p->patched.total_pos + bsdiff_data->add >
            stream_p->total_patched_size) {
            LOG_ERROR("invalid ADD value (%" PRId64 ")", bsdiff_data->add);
            return TUS_DELTA_UPDATE_PATCH_ERROR;
        }
        bsdiff_data->insert = offtin(ctrl_buf + 8);
        if (bsdiff_data->insert < 0 ||
          stream_p->patched.total_pos + bsdiff_data->add + bsdiff_data->insert >
            stream_p->total_patched_size) {
            LOG_ERROR("invalid INSERT value (%" PRId64 ")",
              bsdiff_data->insert);
            return TUS_DELTA_UPDATE_PATCH_ERROR;
        }
        bsdiff_data->seek = offtin(
          ctrl_buf + 16); /* can't check out of range here */

        stream_p->delta.buf.pos = rest;
        stream_p->delta.total_pos += rest;
        bsdiff_data->hold[0] = 0;
    }
    while (stream_p->patched.total_pos < stream_p->skip_patched_pos) {
        if (bsdiff_data->add > 0) {
            /* ADD */
            int64_t add = bsdiff_data->add;
            if (stream_p->delta.buf.pos + add > stream_p->delta.buf.size) {
                add = stream_p->delta.buf.size - stream_p->delta.buf.pos;
            }
            if (stream_p->patched.total_pos + add >
              stream_p->skip_patched_pos) {
                add = stream_p->skip_patched_pos - stream_p->patched.total_pos;
            }
            stream_p->delta.buf.pos += add;
            stream_p->patched.total_pos += add;
            stream_p->src.total_pos += add;
            stream_p->delta.total_pos += add;
            bsdiff_data->add -= add;
            if (stream_p->changed == 0) {
                int64_t i;
                for (i = 0; i < add; i++) {
                    uint8_t diff =
                      stream_p->buf_delta[stream_p->delta.buf.pos + i];
                    if (diff != 0) {
                        stream_p->changed = 1;
                    }
                }
            }
        }
        if (bsdiff_data->add > 0) {
            /* stop at ADD */
            break;
        }
        if (bsdiff_data->insert > 0) {
            /* INSERT */
            int64_t insert = bsdiff_data->insert;
            if (stream_p->delta.buf.pos + insert > stream_p->delta.buf.size) {
                insert = stream_p->delta.buf.size - stream_p->delta.buf.pos;
            }
            if (stream_p->patched.total_pos + insert >
              stream_p->skip_patched_pos) {
                insert = stream_p->skip_patched_pos -
                  stream_p->patched.total_pos;
            }
            stream_p->delta.buf.pos += insert;
            stream_p->patched.total_pos += insert;
            stream_p->delta.total_pos += insert;
            bsdiff_data->insert -= insert;
            stream_p->changed = 1;
        }
        if (bsdiff_data->insert > 0) {
            /* stop at INSERT */
            break;
        }
        if (bsdiff_data->seek != 0) {
            /* SEEK */
            stream_p->src.total_pos += bsdiff_data->seek;
            bsdiff_data->seek = 0;
            stream_p->changed = 1;
        }

        if (stream_p->patched.total_pos >= stream_p->skip_patched_pos) {
            break;
        }

        /* read ctrl part */
        if (stream_p->delta.buf.size - stream_p->delta.buf.pos <
          (int64_t)BSDIFF_CTRL_SIZE) {
            bsdiff_data->hold[0] = stream_p->delta.buf.size -
              stream_p->delta.buf.pos;
            memcpy(&(bsdiff_data->hold[1]),
              stream_p->buf_delta + stream_p->delta.buf.pos,
              bsdiff_data->hold[0]);
            stream_p->delta.buf.pos += bsdiff_data->hold[0];
            stream_p->delta.total_pos += bsdiff_data->hold[0];
            break;
        } else {
            bsdiff_data->add = offtin(
              stream_p->buf_delta + stream_p->delta.buf.pos);
            if (bsdiff_data->add < 0 ||
              stream_p->patched.total_pos + bsdiff_data->add >
                stream_p->total_patched_size) {
                LOG_ERROR("invalid ADD value (%" PRId64 ")", bsdiff_data->add);
                return TUS_DELTA_UPDATE_PATCH_ERROR;
            }
            bsdiff_data->insert = offtin(
              stream_p->buf_delta + stream_p->delta.buf.pos + 8);
            if (bsdiff_data->insert < 0 ||
              stream_p->patched.total_pos + bsdiff_data->add +
                  bsdiff_data->insert >
                stream_p->total_patched_size) {
                LOG_ERROR("invalid INSERT value (%" PRId64 ")",
                  bsdiff_data->insert);
                return TUS_DELTA_UPDATE_PATCH_ERROR;
            }
            bsdiff_data->seek = offtin(stream_p->buf_delta +
              stream_p->delta.buf.pos + 16); /* can't check out of range here */

            stream_p->delta.buf.pos += BSDIFF_CTRL_SIZE;
            stream_p->delta.total_pos += BSDIFF_CTRL_SIZE;
        }
    }

    return TUS_DELTA_UPDATE_PATCH_OK;
}

tus_delta_update_funcs tus_delta_update_bsdiff_funcs = { bsdiff_initialize,
    bsdiff_cancel, bsdiff_finalize, bsdiff_patch, bsdiff_suspend, bsdiff_resume,
    bsdiff_skip };
