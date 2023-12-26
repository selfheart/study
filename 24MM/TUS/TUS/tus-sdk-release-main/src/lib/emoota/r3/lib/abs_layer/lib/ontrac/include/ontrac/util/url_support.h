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
 * @file util/url_support.h
 * @date Apr 9, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief A URL parser and formatter
 * https://en.wikipedia.org/wiki/URL
 *
 */

#ifndef SPIL_NIO_HTTP_URL_SUPPORT_H_
#define SPIL_NIO_HTTP_URL_SUPPORT_H_

#include <ontrac/ontrac/abq_str.h>
#include <ontrac/text/url_enc.h>
#include <ontrac/util/byte_buffer.h>

#include <ontrac/ontrac/namevars.h>

/** typedef of url_t */
typedef struct for_url url_t;
/** structure of url_t */
struct for_url {
    /** the url's credentials ({user}:{password}) where :password is optional */
    cstr_t credentials;
    /** the url's scheme */
    cstr_t scheme;
    /** the url's host */
    cstr_t host;
    /** the url's post */
    uint16_t port;
    /** the url's path */
    cstr_t path;
    /** the url's query */
    namevars_t *query;
};

extern const class_t url_class;

extern url_t *url_instance(cstr_t credentials,
        cstr_t scheme, cstr_t host, uint16_t port,
        cstr_t path, namevars_t *query);
/**
 * @brief resolves an instance of url_class to the url_t*
 *
 * @param pointer to item to be resolved
 * @return resolved url_t* or NULL on failure
 */
extern url_t* url_resolve(cvar_t item);
/**
 * @brief decodes and parses a string formatted url, then allocates and initializes a url_t with the parsed data
 *
 * @param spec: a string formatted url to be parsed
 * @param spec_len: length of the string formatted url
 * @return pointer to a newly parsed url_t, or NULL on failure
 */
extern url_t *url_create(cstr_t spec, int32_t spec_len);
/**
 * @brief formats a url_t into a newly allocated, self-referencing cstr_t
 *
 * @param url:  pointer to a url_t to be formatted & encoded into a string
 * @return a new cstr_t pointer, or NULL on failure
 */
extern cstr_t url_format_str(const url_t *url);
/**
 * @brief decode a URL path from the beginning of a given abq_decoder_t
 *
 * @param decoder: byte_t array wrapped with a url_codec
 * @param delim: Optional support for knowing what next character is post path variable
 * @return decoded url path classified as a string_class with a self-reference, or NULL on error
 */
extern cstr_t url_decode_path(abq_decoder_t *decoder, byte_t *delim);
/**
 * @brief parses a singular query parameter (key & value set) from the given decoder and puts them into query
 *
 * @param decoder: byte_t array wrapped with a url_codec
 * @param delim: Optional support for knowing what next character is post query-param
 * @param query: storage for recording the parsed query-param
 * @return EXIT_SUCCESS if successful, else an error code
 */
extern err_t url_decode_query_param(abq_decoder_t *decoder, byte_t* delim, namevars_t *query);

/**
 * @brief used to check if a particular URI is qualified with a scheme
 * - Warning, this does not do a full parse of the URL, so it might return true for malformed URLs with a scheme
 *
 * @param uri : A Universal Resource Indicator which might be a full Universal Resource Locator
 * @return true if the string includes a scheme delimiter, false otherwise
 */
extern bool_t uri_is_url(cstr_t uri);

/**
 * @brief used to convert check for the leading "file://" scheme on a uri and crop it if found
 *
 * @param uri: A URI used to reference a file on the local file-system, potentially just the path or a full URL
 */
extern cstr_t uri_snip_to_path(cstr_t uri);

/**
 * @brief check that a FQDN has a valid value
 * - At least 1 character
 * - Each character must be '.', '-', or alphanumeric
 *
 * @param fqdn: string to verify
 * @return true if valid, false otherwise
 */
extern bool_t fqdn_is_valid(cstr_t fqdn);

#endif /* SPIL_NIO_HTTP_URL_SUPPORT_H_ */
