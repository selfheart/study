//#line 2 "util/url_support.c"
/****************************************************************************
 *
 *         AIRBIQUITY INC. PROPRIETARY AND CONFIDENTIAL INFORMATION
 *
 *  The software and information contained herein is the proprietary and
 *  confidential property of Airbiquity Inc. and shall not be disclosed
 *  in whole or in part. Possession, use, reproduction or transfer of
 *  this software and information is prohibited without the express written
 *  permission of Airbiquity Inc.
 *
 *  Copyright (c) 2017 Airbiquity Inc.  All rights reserved.
 *
 ****************************************************************************/
/**
 * @file util/url_support.c
 * @date Apr 9, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {Help Me}
 *
 */

#include <ontrac/util/url_support.h>

#include <ctype.h>

#ifndef MAX_NO_URLS
#define MAX_NO_URLS (4U)
#endif /* MAX_NO_URLS */

static void url_delete(cvar_t old_url);

DEFINE_CLASS(url_class, url_t, MAX_NO_URLS, NULL, NULL, NULL, NULL, url_delete);

static cstr_t abq_decode_token(abq_decoder_t *decoder,
        cstr_t delimiters, byte_t* match,
        bool_t match_required) {
    cstr_t retval = NULL;
    size_t available = decoder->max - decoder->pos;
    if (0UL == available) {
        // Expect the token to end with one of "delimiters" or a terminator ('\0')
        abq_status_set(ENODATA, false);
    } else {
        cstr_t source = &decoder->source[decoder->pos];
        int32_t endtoken = utf8_end_of_token(source, (int32_t) available, delimiters, -1);
        if (0 > endtoken) {
            // Error occurred, return as is
        } else {
            // mark delimiter as end-of string
            decoder->max = decoder->pos + (size_t) endtoken;
            retval = abq_decode_str(decoder);
            // return max to previous position
            decoder->max = (decoder->pos - (size_t) endtoken) + available;
            byte_t delim = abq_decode_char(decoder);
            if (('\0' == delim) && (match_required)) {
                // Token not found, terminated string, rewind decoder
                ABQ_DECODER_REWIND(decoder, delim);
                decoder->pos -= (size_t) endtoken;
                decoder->max = decoder->pos + available;
                (void) obj_release_self(retval);
                retval = NULL;
            } else {
                if (NULL != match) {
                    *match = delim;
                }
            }
        }
    }
    return retval;
}

err_t url_decode_query_param(abq_decoder_t *decoder,
        byte_t* delim, namevars_t *query) {
    err_t status = EXIT_SUCCESS;
    VITAL_NOT_NULL(decoder);
    ABQ_VITAL(&url_codec == decoder->codec);
    byte_t match = '\0';
    cstr_t token = abq_decode_token(decoder, "#=&;\r\n", &match, false); // parasoft-suppress CERT_C-MSC41-a-1 "c0544. This string does not contain sensitive information."
    if (NULL == token) {
        // Parsing error
        status = abq_status_take(EIO);
        // Token released below
    } else if (NULL == query) {
        status = EFAULT;
    } else if ('=' == match) {
        // decoded a key value, must be post-fixed with a value
        cstr_t value = abq_decode_token(decoder, "#=&;\r\n", &match, false); // parasoft-suppress CERT_C-MSC41-a-1 "c0545. This string does not contain sensitive information."
        if ((NULL == value) || ('=' == match)) {
            status = abq_status_take(EILSEQ);
        } else {
            status = namevars_append(query, token, value);
        }
        (void) obj_release_self(value);
     } else {
        status = namevars_append(query, token, NULL);
    }
    if(NULL != delim) {
        *delim = match;
    }
    (void) obj_release_self(token);
    return status;
}

static namevars_t* abq_decode_query(abq_decoder_t *decoder, byte_t* delim) {
    VITAL_NOT_NULL(delim);
    *delim = (byte_t) 0xFF;
    namevars_t *retval = namevars_create( );
    while ((NULL != retval) && ('#' != *delim) && ('\0' != *delim) && (' ' != *delim) && ('\r' != *delim) && ('\n' != *delim)) {
        err_t status = url_decode_query_param(decoder, delim, retval);
        if (EXIT_SUCCESS != status) {
            // Parsing error
            (void) abq_status_set(status, false);
            (void) obj_release_self(retval);
            retval = NULL;
        }
    }
    return retval;
}

cstr_t url_decode_path(abq_decoder_t *decoder, byte_t *delim) {
    cstr_t retval = NULL;
    if (NULL == decoder) {
        (void) abq_status_set(EFAULT, false);
    } else if(decoder->pos >= decoder->max) {
        // Default to empty string on ENODATA
        retval = str_create("", 0, false);
        if (NULL != delim) {
            *delim = '\0';
        }
    } else {
        ABQ_VITAL(&url_codec == decoder->codec);
        retval = abq_decode_token(decoder, "?#\r\n", delim, false); // parasoft-suppress CERT_C-MSC41-a-1 "c0546. This string does not contain sensitive information."
    }
    return retval;
}

static err_t url_populate(url_t *url, abq_decoder_t *decoder) {
    err_t retval = CHECK_NULL(url);
    if (EXIT_SUCCESS != retval) {
        // Return error as is
    } else if(NULL == decoder) {
        retval = EFAULT;
    } else {
        byte_t delim = (byte_t) 0xFF;
        url->scheme = abq_decode_token(decoder, ":", // parasoft-suppress CERT_C-MSC41-a-1 "c0547. This string does not contain sensitive information."
                &delim, true);
        if (NULL == url->scheme) {
            retval = abq_status_take(EILSEQ);
        }else{
            VITAL_IS_OK(obj_takeover((cvar_t)url->scheme, (cvar_t)url));

            if(0 == utf8_compare_insensitive(url->scheme, "https", -1)){ // parasoft-suppress CERT_C-MSC41-a-1 "c0548. This string does not contain sensitive information."
                url->port = 443; // set default port for https, can still be overwritten below
            }else if(0 == utf8_compare_insensitive(url->scheme, "http", -1)){ // parasoft-suppress CERT_C-MSC41-a-1 "c0549. This string does not contain sensitive information."
                url->port = 80; // set default port for http, can
            }else{
                // Unknown scheme, "file" perhaps?
            }
            retval = abq_decode_skip_prefix(decoder, "//", -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0550. This string does not contain sensitive information."
            if (EXIT_SUCCESS == retval) {
                url->credentials = abq_decode_token(decoder, "@", // parasoft-suppress CERT_C-MSC41-a-1 "c0551. This string does not contain sensitive information."
                        &delim, true);
                if (NULL != url->credentials) {
                    (void) obj_takeover(url->credentials, url);
                }
                url->host = abq_decode_token(decoder, ":", // parasoft-suppress CERT_C-MSC41-a-1 "c0552. This string does not contain sensitive information."
                        &delim, true);
                if (NULL != url->host) {
                    int64_t port = 0; // would be a uint8_t but int64_t is supported by utf8_utils
                    // Port delimiter was found, need to parse port
                    retval = abq_decode_int(decoder, &port, DECIMAL_RADIX);
                    url->port = (uint16_t) port;
                } else {
                    // Port delimiter was not found, retry for host
                    url->host = abq_decode_token(decoder, "/?#\r\n", // parasoft-suppress CERT_C-MSC41-a-1 "c0553. This string does not contain sensitive information."
                            &delim, false);
                    ABQ_DECODER_REWIND(decoder, delim);
                }
                if((NULL != url->host) && (fqdn_is_valid(url->host))) {
                    (void) obj_takeover(url->host, url);
                }else{
                    // Failed to parse host, even an empty string would have been OK
                    retval = abq_status_take(EIO);
                }
            } else if (EINVAL == retval) {
                // Not all
                retval = EXIT_SUCCESS;
            } else {
                // Return error as is
            }
            if (EXIT_SUCCESS == retval) {
               // Force URL decoding for query
               decoder->codec = &url_codec;
               // everything up to the query or fragment is considered path
               url->path = url_decode_path(decoder, &delim);
               if (NULL == url->path) {
                   // Failed to parse path, even an empty string would have been OK
                   retval = abq_status_take(EIO);
               } else {
                   (void) obj_takeover(url->path, url);
                    if ('?' == delim) {
                        // Query section exists, read out query section
                        url->query = abq_decode_query(decoder, &delim);
                        if (NULL == url->query) {
                            // Failure to parse the query section
                            retval = abq_status_take(EIO);
                        } else {
                            (void) obj_takeover(url->query, url);
                        }
                    }
                    if ('#' == delim) {
                        ABQ_WARN_MSG("ignoring the fragment section");
                    }
                }
            }
        }
    }
    return retval;
}

url_t *url_create(cstr_t spec, int32_t spec_len) {
    url_t *result = CREATE_BASE_INSTANCE(url_class, url_t);
    if(NULL != result){
        // zero out the data
        bytes_set(result, '\0', sizeof(url_t));
        ABQ_DECODER(decoder, &utf8_codec, spec, spec_len);
        err_t err = url_populate(result, &decoder);
        if(EXIT_SUCCESS != err) {
            (void)abq_status_set(err, false);
            VITAL_IS_OK(obj_release_self((cvar_t)result));
            result = NULL;
        }
    }
    return result;
}

url_t* url_resolve(cvar_t item) {
    url_t *retval = NULL;
    if (NULL != item) {
        CLASS_RESOLVE(url_class, url_t, retval, item);
    }
    return retval;
}

static void url_delete(cvar_t old_url) {
    if (NULL != old_url) {
        url_t *url = url_resolve(old_url);
        VITAL_NOT_NULL(url);
        (void) obj_release((cvar_t) url->credentials, old_url);
        (void) obj_release((cvar_t) url->scheme, old_url);
        (void) obj_release((cvar_t) url->host, old_url);
        (void) obj_release((cvar_t) url->path, old_url);
        (void) obj_release((cvar_t) url->query, old_url);
    }
}

url_t *url_instance(cstr_t credentials,
        cstr_t scheme, cstr_t host, uint16_t port,
        cstr_t path, namevars_t *query) {
    url_t* retval = CREATE_BASE_INSTANCE(url_class, url_t);
    if (NULL != retval) {
        (void) obj_reserve(query, retval);
        *retval = (url_t) {
            .credentials = str_coerce(credentials, retval),
            .scheme = str_coerce(scheme, retval),
            .host = str_coerce(host, retval),
            .port = port,
            .path = str_coerce(path, retval),
            .query = query
        };
        if(((NULL == retval->credentials) && (NULL != credentials))
                || ((NULL == retval->scheme) && (NULL != scheme))
                || ((NULL == retval->host) && (NULL != host))
                || ((NULL == retval->path) && (NULL != path))) {
            // Failed to internalize fields
            (void)obj_release_self(retval);
            retval = NULL;
        }
    }
    return retval;
}

static err_t abq_encode_query(abq_encoder_t *encoder, const namevars_t* query) {
    err_t retval = EXIT_SUCCESS;
    if ((NULL == query) || (NULL == encoder)) {
        retval = EFAULT;
    } else {
        cstr_t key = NULL;
        cstr_t value = NULL;
        ABQ_VITAL(&url_codec == encoder->codec);
        for (uint16_t index = 0U; index < query->count; index++) {
            key = query->array[index].name;
            value = str_resolve(query->array[index].value);
            if ((NULL != query->array[index].value)
                    && (NULL == value)) {
                // Value was not classified as string_class
                retval = abq_status_take(ENOSYS);
            } else if (0U == index) {
                // First name/value pair, mark as q query string w/ '?'
                //  Use the utf8_codec so that the character is not URL escaped
                retval = utf8_codec.encode(encoder, '?');
            } else {
                // Secondary name/value pair, delimit with an ampersand '&'
                // Use the utf8_codec so that the character is not URL escaped
                retval = utf8_codec.encode(encoder, '&');
            }
            if (EXIT_SUCCESS == retval) {
                // Write out the query key
                if (NULL != key) {
                    retval = abq_encode_text(encoder,
                            &utf8_codec, key, -1);
                }
                if ((NULL != value) && (EXIT_SUCCESS == retval)) {
                    // Use the utf8_codec so that the character is not URL escaped
                    retval = utf8_codec.encode(encoder, '=');
                    if (EXIT_SUCCESS == retval) {
                        // Write out the query value
                        retval = abq_encode_text(encoder,
                                &utf8_codec, value, -1);
                    }
                }
            }
            if (EXIT_SUCCESS != retval) {
                break;
            }
        }
    }
    return retval;
}

static err_t url_format(const url_t *url, abq_encoder_t *encoder) {
    err_t retval = EXIT_SUCCESS;
    if(NULL == encoder){
        retval = EFAULT;
    }else if(NULL == url->scheme){
        retval = ENODATA;
    }else{
        retval = abq_encode_ascii(encoder, url->scheme, -1);
        if(EXIT_SUCCESS == retval) {
            // Use the utf8_codec so that the character is not URL escaped
            retval = utf8_codec.encode(encoder, ':');
        }
        if (EXIT_SUCCESS == retval) {
            // A URL is not required to have authority components
            if ((NULL != url->host) || (NULL != url->credentials)) {
                // Add section for authority components
                retval = abq_encode_ascii(encoder, "//", -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0554. This string does not contain sensitive information."
                if((EXIT_SUCCESS == retval) && (NULL != url->credentials)) {
                    retval = abq_encode_text(encoder,
                            &utf8_codec, url->credentials, -1);
                    if(EXIT_SUCCESS == retval){
                        // Use the utf8_codec so that the character is not URL escaped
                        retval = utf8_codec.encode(encoder, '@');
                    }
                }
                if((EXIT_SUCCESS == retval) && (NULL != url->host)) {
                    retval = abq_encode_text(encoder,
                            &utf8_codec, url->host, -1);
                    if((EXIT_SUCCESS == retval) && (0U != url->port)) {
                        if((80U == url->port) && (UTF8_EQUALS(url->scheme, "http"))){ // parasoft-suppress CERT_C-MSC41-a-1 "c0555. This string does not contain sensitive information."
                            // Default port do not append
                        }else if((443U == url->port) && (UTF8_EQUALS(url->scheme, "https"))){ // parasoft-suppress CERT_C-MSC41-a-1 "c0556. This string does not contain sensitive information."
                            // Default port do not append
                        }else{
                            // Use the utf8_codec so that the character is not URL escaped
                            retval = utf8_codec.encode(encoder, ':');
                            if (EXIT_SUCCESS == retval) {
                                retval = abq_encode_int(encoder, (int64_t) url->port, DECIMAL_RADIX);
                            }
                        }
                    }
                }
            }
            // Completed authority components
            if((EXIT_SUCCESS == retval) && (NULL != url->path)) {
                retval = abq_encode_text(encoder,
                        &utf8_codec, url->path, -1);
            }

            if((EXIT_SUCCESS == retval) && (NULL != url->query)) {
                retval = abq_encode_query(encoder, url->query);
            }
        }
    }
    return retval;
}

cstr_t url_format_str(const url_t *url) {
    cstr_t retval = NULL;
    if (NULL != url) {
        byte_t buffer[BUFFER_MEDIUM];
        ABQ_ENCODER(encoder, &url_codec, buffer, sizeof(buffer));
        err_t err = url_format(url, &encoder);
        if (EXIT_SUCCESS != err) {
            abq_status_set(err, false);
        } else {
            retval = str_create(buffer, (int32_t) encoder.pos, true);
        }
    }
    return retval;
}

static const cstr_t scheme_delimiter = "://"; // parasoft-suppress CERT_C-MSC41-a-1 "c0557. This string does not contain sensitive information."
bool_t uri_is_url(cstr_t uri) {
    bool_t retval = false;
    int32_t delimieter_len = utf8_byte_length(scheme_delimiter, -1);
    // Simply check for the presence of the delimiter
    if(0 <= utf8_index_of(uri, -1, scheme_delimiter, delimieter_len)) {
        retval = true;
    }
    return retval;
}

cstr_t uri_snip_to_path(cstr_t uri) {
    cstr_t retval = NULL;
    if (NULL != uri) {
        int32_t lws = utf8_leading_ws(uri, -1);
        if (0 <= lws) {
            retval = &uri[lws];
            int32_t delimieter_len = utf8_byte_length(scheme_delimiter, -1);
            int32_t indexof_delimiter
                    = utf8_index_of(retval, -1, scheme_delimiter, delimieter_len);
            if (0 > indexof_delimiter) {
                // Assume the uri is already a file-system path. return as is
            } else {
                // Skip over scheme and the scheme_delimiter
                retval = &retval[indexof_delimiter + delimieter_len];
                // Skip over credentials, host and port if any searching for path, query or fragment
                indexof_delimiter = utf8_end_of_token(retval, -1,
                                                      "/?# \r\n",  // parasoft-suppress CERT_C-MSC41-a-1 "c0558. This string does not contain sensitive information."
                                                      -1);
                if (0 <= indexof_delimiter) {
                    retval = &retval[indexof_delimiter];
                }
            }
        }
    }
    return retval;
}

bool_t fqdn_is_valid(cstr_t fqdn) {
    bool_t is_valid = true;
    if (NULL == fqdn) {
        is_valid = false;
    } else {
        ABQ_DECODER(decoder, &ascii_codec, fqdn, -1);
        int32_t codepoint = -1;
        for (codepoint = abq_decode_cp(&decoder);
             0 < codepoint;
             codepoint = abq_decode_cp(&decoder)) {
            if ((false == ascii_is_alnum(codepoint))
                && (codepoint != (int32_t) '.')
                && (codepoint != (int32_t) '-')) {
                // Invalid character
                is_valid = false;
                break;
            }
            // Keep iterating
        }
        if (0 > codepoint) {
            // Invalid ASCII character
            is_valid = false;
        } else if (0U == decoder.pos) {
            // Empty FQDN is not valid
            is_valid = false;
        } else {
            // Happy path
        }
    }
    return is_valid;
}
