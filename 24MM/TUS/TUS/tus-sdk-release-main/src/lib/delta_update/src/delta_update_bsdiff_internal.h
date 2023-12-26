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

#pragma once

#define BSDIFF_MAGIC_STRING "TUS_BSDIFF_010"
#define BSDIFF_MAGIC_SIZE (sizeof(BSDIFF_MAGIC_STRING) - 1)
#define BSDIFF_NEWSIZE_SIZE (8)
#define BSDIFF_CTRL_SIZE (3 * sizeof(int64_t))

/* BSDIFF: difference algorithm dependent data */
typedef struct bsdiff_stream {
    int64_t add;                    /* (remaining) size of ADD */
    int64_t insert;                 /* (remaining) size of INSERT */
    int64_t seek;                   /* SEEK movement amount */
                                    /* if add!=0 then ADD.
                                     * if add==0 and insert!=0 then INSERT.
                                     * if add==0 and insert==0 and seek!=0 then SEEK.
                                     * if add==0 and insert==0 and seek==0 then read ctrl part.
                                     */
    uint8_t hold[BSDIFF_CTRL_SIZE]; /* hold ctrl part */
                                    /* if hold[0]==0 then Non-holding.
                                     * if 1<=hold[0]<=23 then number of bytes to hold,
                                     * and hold a part of ctrl part after hold[1].
                                     */
} bsdiff_stream;
