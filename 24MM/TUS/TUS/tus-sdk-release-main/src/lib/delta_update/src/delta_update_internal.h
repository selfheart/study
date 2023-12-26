/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */
#pragma once

typedef struct tus_delta_update_funcs {
    int (*initialize)(tus_delta_update_stream *stream_p);
    int (*cancel)(tus_delta_update_stream *stream_p);
    int (*finalize)(tus_delta_update_stream *stream_p);
    int (*patch)(tus_delta_update_stream *stream_p);
    int (*suspend)(tus_delta_update_stream *stream_p, uint8_t *buf_save);
    int (*resume)(tus_delta_update_stream *stream_p, uint8_t *buf_save);
    int (*skip)(tus_delta_update_stream *stream_p);
} tus_delta_update_funcs;
