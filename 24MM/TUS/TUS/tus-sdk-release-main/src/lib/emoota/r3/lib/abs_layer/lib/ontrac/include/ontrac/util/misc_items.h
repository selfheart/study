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
 * @file util/misc_items.h
 * @date Jul 14, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {Help Me}
 *
 */

#ifndef ONTRAC_ONTRAC_MISC_ITEMS_H_
#define ONTRAC_ONTRAC_MISC_ITEMS_H_


#include <ontrac/ontrac/abq_class.h>

/**
 * A callback function used to pass a response item back to a requester
 *
 * @param status_code: an http_status_code_t, HTTP_OK is expected for success
 * @param ctx: The consumer's context which was registered alongside the callback function
 * @param result: A classified, potentially NULL result of the request, likely an error_structure_t* for failed requests.
 * @return 0 if handled correctly, else an error code
 */
typedef void (*variable_cb_t)(http_status_code_t status_code, cvar_t ctx, cvar_t result);

/* a container used to store the results of some event */
typedef struct {
    /** result item of event or operation */
    cvar_t item;
    /** status code associated with result */
    err_t status;
    /** optional callback method to be invoked once a result is obtained */
    variable_cb_t callback;
    /** original context to be returned to caller */
    cvar_t ctx;
} var_result_t;

var_result_t* var_result_create(cvar_t item, http_status_code_t status, variable_cb_t callback, cvar_t ctx);
/**
 * @brief resolves an instance of var_result_class to the var_result_t*
 *
 * @param pointer to item to be resolved
 * @return resolved var_result_t* or NULL on failure
 */
var_result_t* var_result_resolve(cvar_t item);

/**
 * @brief a on_item_parsed_t for collecting parsing results in a var_result_t
 * @see rest/item_readers.h for more information on on_item_parsed_t
 *
 * @param err_code: status code associated with result
 * @param ctx: reference to a classified var_result_t
 * @param body: the item which was parsed
 * @return ECANCELED to stop paring items after the first item, else another error code
 */
extern bool_t var_result_on_item_parsed(err_t err_code, cvar_t ctx, cvar_t body);

#endif /* ONTRAC_ONTRAC_MISC_ITEMS_H_ */
