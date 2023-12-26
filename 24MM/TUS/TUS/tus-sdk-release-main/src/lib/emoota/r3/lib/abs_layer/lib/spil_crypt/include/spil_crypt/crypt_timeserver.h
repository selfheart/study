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
 *  Copyright (c) 2020 Airbiquity Inc.  All rights reserved.
 *
 * ****************************************************************************
 */

#ifndef CRYPT_TIMESERVER_H_
#define CRYPT_TIMESERVER_H_

/**
 * This file defines functions used to communicate with time server. Generally speaking, these
 * functions depend more on end-to-end architecture, instead of specific target devices.
 */

/**
 * @brief Generate time server request.
 *
 * This function generates necessary request to communicate with a time server.
 * @public
 * @param [in] request buffer for request to be sent to time server
 * @param [in] buf_size size of request buffer
 * @param [in] query name of the query to be sent.
 *        - If query is not empty (query[0] != '\0'), then the request should be sent as query
 *           of HTTP_GET in the format of \<query\>=\<request\>.
 *        - If query is empty (query[0] == '\0'), then the request is sent as data of HTTP_POST
 * @param [in] query_buf_size size of the buffer to hold query name
 * @param [in] url_path URL path to the time server. Time server request would be sent to
 *            http://\<server address\>/\<url_path\>
 * @param [in] url_buf_size size of the buffer to hold url_path
 * @retval EXIT_SUCCESS or error code
 */
err_t crypt_time_server_request(str_t request, size_t buf_size,
                                str_t query, size_t query_buf_size, str_t url_path, size_t url_buf_size);

/**
 * @brief Process time server response and set trusted time
 *
 * Process the response of the time server request from crypt_time_server_request(), verify
 * validity of the response and update trusted time based on the response.
 * This function may optionally update the system time as well.
 *
 * @public
 * @param [in] time_response received from the server
 * @param [out] timestamp_ms pointer to the return buffer of timestamp contained in the response.
 *           This is valid only when the return value is EXIT_SUCCESS
 * @param [in] req original time server request sent to the server, from crypt_time_server_request().
 *           Underline code may or may not use this to verify the response.
 * @retval EXIT_SUCCESS or error code
 */
err_t crypt_on_time_server_response(cstr_t time_response, uint64_t *timestamp_ms,
                                    cstr_t req);
#endif /* CRYPT_TIMESERVER_H_ */
