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
 *  Copyright (c) 2019 Airbiquity Inc.  All rights reserved.
 *
 * ****************************************************************************
 */
/**
 * @file abq_io.h
 * @date Jan 9, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#ifndef ONTRAC_STREAM_ABQ_IO_H_
#define ONTRAC_STREAM_ABQ_IO_H_

#include <platform/platformconfig.h>

typedef enum {
    /** the offset is measured in bytes since the beginning of the file, (absolute position) */
    ABQ_SEEK_SET = 0,
    /** the offset is measured in bytes from current position within file, (relative position) */
    ABQ_SEEK_CUR = 1,
    /** the offset is measured in bytes relative to the end-of-file */
    ABQ_SEEK_END = 2
} abq_whence_t;

typedef uint32_t abq_io_flags;
/** Can be used clear all flags*/
#define ABQ_IO_NOFLAGS (0x00U)
/** (1U << 0U) Data is available to be read from datatap_t*/
#define ABQ_IO_READ (0x01U)
/** (1U << 1U) No additional input data (beyond what is currently buffered) will be available to this datatap_t */
#define ABQ_IO_STOP (0x02U)
/** (1U << 2U) Able to write more data to datasink_t */
#define ABQ_IO_WRITE (0x04U)
/** (1U << 3U) Data has been flushed from pending data-writers buffers used in datasink_t */
#define ABQ_IO_FLUSH (0x08U)
/** (ABQ_IO_READ | ABQ_IO_STOP | ABQ_IO_WRITE | ABQ_IO_FLUSH)
 * Matches any of the data related IO events */
#define ABQ_IO_DATA (0x0FU)

/** (1U << 4U) Underlying source has timed-out doing an operation */
#define ABQ_IO_TIMEOUT (0x10U)
/** (1U << 5U) Underlying source has received a remote hang-up */
#define ABQ_IO_HANGUP (0x20U)
/** (1U << 6U) Error has occurred in the pipeline */
#define ABQ_IO_ERROR (0x40U)
/** (1U << 7U) Underlying source has been closed by the application */
#define ABQ_IO_CLOSED (0x80U)
/** (ABQ_IO_TIMEOUT | ABQ_IO_HANGUP | ABQ_IO_ERROR | ABQ_IO_CLOSED)
 * Matched any of the pipeline related events */
#define ABQ_IO_PIPELINE (0xF0U)

/** (UINT32_MAX) Will mask to any of the above events  */
#define ABQ_IO_ANY (0xFFFFFFFFU)

static inline bool_t abq_io_readable(abq_io_flags flags) {
    bool_t retval = false;
    if(ABQ_IO_NOFLAGS != (ABQ_IO_READ & flags)) {
        retval = true;
    }
    return retval;
}
static inline bool_t abq_io_has_eof(abq_io_flags flags) {
    bool_t retval = false;
    if(ABQ_IO_NOFLAGS != (ABQ_IO_STOP & flags)) {
        retval = true;
    }
    return retval;
}
static inline bool_t abq_io_writable(abq_io_flags flags) {
    bool_t retval = false;
    if(ABQ_IO_NOFLAGS != (ABQ_IO_WRITE & flags)) {
        retval = true;
    }
    return retval;
}
static inline bool_t abq_io_is_flushed(abq_io_flags flags) {
    bool_t retval = false;
    if(ABQ_IO_NOFLAGS != (ABQ_IO_FLUSH & flags)) {
        retval = true;
    }
    return retval;
}
static inline bool_t abq_io_data_check(abq_io_flags flags) {
    bool_t retval = false;
    if(ABQ_IO_NOFLAGS != (ABQ_IO_DATA & flags)) {
        retval = true;
    }
    return retval;
}

static inline bool_t abq_io_pipe_check(abq_io_flags flags) {
    bool_t retval = false;
    if(ABQ_IO_NOFLAGS != (ABQ_IO_PIPELINE & flags)) {
        retval = true;
    }
    return retval;
}
static inline bool_t abq_io_timeout(abq_io_flags flags) {
    bool_t retval = false;
    if(ABQ_IO_NOFLAGS != (ABQ_IO_TIMEOUT & flags)) {
        retval = true;
    }
    return retval;
}
static inline bool_t abq_io_hangup(abq_io_flags flags) {
    bool_t retval = false;
    if(ABQ_IO_NOFLAGS != (ABQ_IO_HANGUP & flags)) {
        retval = true;
    }
    return retval;
}
static inline bool_t abq_io_has_error(abq_io_flags flags) {
    bool_t retval = false;
    if(ABQ_IO_NOFLAGS != ((ABQ_IO_TIMEOUT | ABQ_IO_HANGUP | ABQ_IO_ERROR) & flags)) {
        retval = true;
    }
    return retval;
}
static inline bool_t abq_io_is_closed(abq_io_flags flags) {
    bool_t retval = false;
    if(ABQ_IO_NOFLAGS != (ABQ_IO_CLOSED & flags)) {
        retval = true;
    }
    return retval;
}

static inline cstr_t abq_io_flags_strval( abq_io_flags flags) {
    cstr_t retval = NULL;
    if (abq_io_pipe_check(flags)) {
        retval = "err";
    } else if(abq_io_writable(flags)) {
        if(abq_io_readable(flags)) {
            retval = "r/w";
        }else{
            retval = "write";
        }
    } else if(abq_io_readable(flags)) {
        retval = "read";
    } else {
        retval = "?";
    }
    return retval;
}

typedef enum {
    /** ready to publish event to listeners */
    ABQ_PUBLISH_INACTIVE = 0,
    /** publication in progress, and state hasn't changed since starting */
    ABQ_PUBLISH_CLEAN,
    /** publication in progress, and state has been modified since starting */
    ABQ_PUBLISH_DIRTY,
} abq_publish_state;

// Note these MUST be lower case to work with rest_request.c mime_type checking
#define MISC_CONTENT_TYPE  "application/octet-stream"
#define JSON_CONTENT_TYPE  "application/json"
#define CSV_CONTENT_TYPE   "text/csv"
#define TEXT_CONTENT_TYPE  "text/plain"
#define GZ_CONTENT_TYPE    "application/x-gunzip"
#define XML_CONTENT_TYPE   "application/xml"
#define JS_CONTENT_TYPE    "application/javascript"
#define TAR_CONTENT_TYPE   "application/x-tar"
#define TGZ_CONTENT_TYPE   "application/x-tar-gz"
#define XHTML_CONTENT_TYPE "application/xhtml+xml"
#define PDF_CONTENT_TYPE   "application/pdf"
#define RTF_CONTENT_TYPE   "application/rtf"
#define PS_CONTENT_TYPE    "application/postscript"
#define SWF_CONTENT_TYPE   "application/x-shockwave-flash"
#define ARG_CONTENT_TYPE   "application/x-arj-compressed"
#define ZIP_CONTENT_TYPE   "application/x-zip-compressed"
#define DOC_CONTENT_TYPE   "application/msword"
#define PPT_CONTENT_TYPE   "application/x-mspowerpoint"
#define XLS_CONTENT_TYPE   "application/x-msexcel"
#define TORNT_CONTENT_TYPE "application/x-bittorrent"
#define SFNT_CONTENT_TYPE  "application/font-sfnt"
#define PRF_CONTENT_TYPE   "application/font-tdpfr"
#define WOFF_CONTENT_TYPE  "application/font-woff"
#define HTML_CONTENT_TYPE  "text/html"
#define CSS_CONTENT_TYPE   "text/css"
#define XML_CONTENT_TYPE2  "text/xml"
#define SGML_CONTENT_TYPE  "text/sgml"
#define GIF_CONTENT_TYPE   "image/gif"
#define IEF_CONTENT_TYPE   "image/ief"
#define JPEG_CONTENT_TYPE  "image/jpeg"
#define JPM_CONTENT_TYPE   "image/jpm"
#define JPX_CONTENT_TYPE   "image/jpx"
#define PNG_CONTENT_TYPE   "image/png"
#define SVG_CONTENT_TYPE   "image/svg+xml"
#define TIFF_CONTENT_TYPE  "image/tiff"
#define BMP_CONTENT_TYPE   "image/bmp"
#define ICO_CONTENT_TYPE   "image/x-icon"
#define PCT_CONTENT_TYPE   "image/x-pct"
#define PICT_CONTENT_TYPE  "image/pict"
#define RGB_CONTENT_TYPE   "image/x-rgb"
#define MP3_CONTENT_TYPE   "audio/mpeg"
#define OGG_CONTENT_TYPE   "audio/ogg"
#define AAC_CONTENT_TYPE   "audio/aac"
#define AIF_CONTENT_TYPE   "audio/x-aif"
#define M3U_CONTENT_TYPE   "audio/x-mpegurl"
#define MIDI_CONTENT_TYPE  "audio/x-midi"
#define RAM_CONTENT_TYPE   "audio/x-pn-realaudio"
#define WAV_CONTENT_TYPE   "audio/x-wav"
#define MOV_CONTENT_TYPE   "video/quicktime"
#define MP4_CONTENT_TYPE   "video/mp4"
#define MPEG_CONTENT_TYPE  "video/mpeg"
#define OGV_CONTENT_TYPE   "video/ogg"
#define WEBM_CONTENT_TYPE  "video/webm"
#define ASF_CONTENT_TYPE   "video/x-ms-asf"
#define AVI_CONTENT_TYPE   "video/x-msvideo"
#define M4V_CONTENT_TYPE   "video/x-m4v"
#define WRL_CONTENT_TYPE   "model/vrml"

/**
 * @brief attempts to lookup the appropriate value of the Content-Type header based on the file extension
 *
 * @param file_name: name of the file on which we are to lookup a mime-type match
 * @param if_none_found: default value to return if no mine-type matched the given file extension, recommend using either MISC_CONTENT_TYPE or NULL
 * @return pointer to an appropriate Content-Type header for the given file, or the 'if_none_found' parameter if no match was found
 */
extern cstr_t http_headers_find_content_type(cstr_t file_name, cstr_t if_none_found);

#endif /* ONTRAC_STREAM_ABQ_IO_H_ */
