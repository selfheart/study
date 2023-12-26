//#line 2 "util/misc_items.c"
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
 * @file util/misc_items.c
 * @date Jul 14, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {Help Me}
 *
 */

#include <ontrac/util/misc_items.h>
#ifndef MAX_VAR_RESULTS
#define MAX_VAR_RESULTS (3U)
#endif /* MAX_VAR_RESULTS */

static void var_result_delete(cvar_t old_var_result);
DEFINE_CLASS(var_result_class, var_result_t, MAX_VAR_RESULTS,
        NULL, NULL, NULL, NULL, var_result_delete, static);

var_result_t* var_result_create(cvar_t item, http_status_code_t status,
                                    variable_cb_t callback, cvar_t ctx) {
    var_result_t* rvalue = CREATE_BASE_INSTANCE(var_result_class, var_result_t);
    if(NULL != rvalue) {
        (void)obj_reserve(item, (cvar_t)rvalue);
        (void)obj_reserve(ctx, (cvar_t)rvalue);
        *rvalue = (var_result_t) {
            .item = item,
            .status = status,
            .callback = callback,
            .ctx = ctx
        };
    }
    return rvalue;
}

var_result_t* var_result_resolve(cvar_t item) {
    var_result_t* retval = NULL;
    if (NULL != item) {
        CLASS_RESOLVE(var_result_class, var_result_t, retval, item);
    }
    return retval;
}

static void var_result_delete(cvar_t old_var_result) {
    if (NULL != old_var_result) {
        var_result_t *var_result = var_result_resolve(old_var_result);
        VITAL_NOT_NULL(var_result);
        (void) obj_release(var_result->item, old_var_result);
        (void) obj_release(var_result->ctx, old_var_result);
    }
}

bool_t var_result_on_item_parsed(err_t err_code, cvar_t ctx, cvar_t body) {
    var_result_t *vessel = var_result_resolve(ctx);
    if (NULL == vessel) {
        ABQ_ERROR_MSG("Invalid ctx");
    } else {
    	SET_RESERVED_FIELD(vessel, item, body);
        vessel->status = err_code;
    }
    return true;
}
