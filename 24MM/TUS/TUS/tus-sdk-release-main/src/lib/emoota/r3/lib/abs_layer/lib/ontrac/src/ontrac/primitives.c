//#line 2 "ontrac/primitives.c"
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
 * @file ontrac/primitives.c
 * @date Mar 30, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {Help Me}
 *
 */

#include <ontrac/ontrac/primitives.h>

#ifndef MAX_NUMBERS
#define MAX_NUMBERS (1024U)
#endif /* MAX_NUMBERS */

static cvar_t number_coerce_internal(cvar_t source, cvar_t ref);

static int8_t null_compare(cvar_t left, cvar_t right);

static cvar_t bool_coerce(cvar_t source, cvar_t ref);
static int8_t bool_compare(cvar_t left, cvar_t right);

EMPTY_CLASS(null_class, void, NULL, NULL, null_compare,
        NULL, global_property_getter, global_property_setter, invalid_delete, &null_class);
EMPTY_CLASS(bool_class, bool_t, NULL, bool_coerce, bool_compare,
        NULL, global_property_getter, global_property_setter, invalid_delete, &bool_class);
DEFINE_CLASS(number_class, number_t, MAX_NUMBERS,
        NULL, number_coerce_internal, number_compare, NULL, NULL);

DEFINE_LIST_OF_CLASS(list_of_number_class, number_class);

static bool_t false_const = false;
static bool_t true_const = true;

const bool_ptr true_ptr = &true_const;
const bool_ptr false_ptr = &false_const;

#if !defined(NDEBUG)
static obj_t the_true_obj = {.meta=&bool_class, .refs=LLIST_HEAD_INIT(), .member=(cvar_t)&true_const, .next=NULL};
static obj_t the_false_obj = {.meta=&bool_class, .refs=LLIST_HEAD_INIT(), .member=(cvar_t)&false_const, .next=&the_true_obj};
static obj_t the_null_obj = {.meta=&null_class, .refs=LLIST_HEAD_INIT(), .member=NULL, .next=&the_false_obj};
#else /* NDEBUG */
static obj_t the_true_obj = {.meta=&bool_class, .ref_count=0, .member=(cvar_t)&true_const, .next=NULL};
static obj_t the_false_obj = {.meta=&bool_class, .ref_count=0, .member=(cvar_t)&false_const, .next=&the_true_obj};
static obj_t the_null_obj = {.meta=&null_class, .ref_count=0, .member=NULL, .next=&the_false_obj};
#endif /* NDEBUG */

LLIST_HEAD_DEF(obj_t, primative_obj_list) = &the_null_obj;

// NULL's should always be considered greater so that they are placed at end of list
static int8_t null_compare(cvar_t left, cvar_t right) {
    int8_t rvalue = 0;
    if (NULL == left) {
        rvalue = (int8_t) ((NULL == right) ? 0 : 1);
    } else if (NULL == right) {
        rvalue = -1;
    } else { // neither are null, compare as variables
        rvalue = var_compare(left, right);
    }
    return rvalue;
}

cvar_t bool_var(bool_t value) {
    return value ? (cvar_t) &true_const : (cvar_t) &false_const;
}

bool_ptr bool_intern(bool_t value) {
    return value ? true_ptr : false_ptr;
}

bool_t* bool_resolve(cvar_t source) {
    bool_t* retval = NULL;
    obj_t *obj = obj_lookup(source);
    if (NULL == obj) {
        // (void) abq_status_set(EINVAL, false_const)
    } else if (&bool_class == obj->meta) {
        if(((cvar_t)true_ptr) == ((cvar_t)obj->member)){
            retval = &true_const;
        }else{
            ABQ_VITAL(((cvar_t)false_ptr) == ((cvar_t)obj->member));
            retval = &false_const;
        }
    } else if (&string_class == obj->meta) {
        cstr_t string_value = str_resolve(source);
        if (0 == utf8_compare_insensitive(abq_true_str, string_value, -1)) {
            retval = &true_const;
        } else if (0 == utf8_compare_insensitive(abq_false_str, string_value, -1)) {
            retval =  &false_const;
        } else {
            // (void) abq_status_set(EINVAL, false_const)
        }
    } else if (&number_class == obj->meta) {
        number_t number = number_resolve(obj->member);
        if (IS_NAN(number)) {
            retval = NULL;
        } else if(0 == (int32_t) number) {
            retval =  &false_const;
        } else {
            retval = &true_const;
        }
    } else {
        // (void) abq_status_set(ENOSYS, false_const)
    }
    return retval;
}

static cvar_t bool_coerce(cvar_t source, cvar_t ref) {
    //  Parameter 'ref' is not used in function 'bool_coerce'
    if(NULL == ref) { }
    return (cvar_t) bool_resolve(source);
}

static int8_t bool_compare(cvar_t left, cvar_t right) {
    int8_t rvalue = 0;
    if (left == right) {
        rvalue = 0;
    } else if (NULL == left) {
        rvalue = 1;
    } else if(NULL == right) {
        rvalue = -1;
    } else if(left == (cvar_t)true_ptr) {
        ABQ_VITAL(right == (cvar_t)false_ptr);
        rvalue = -1;
    } else {
        ABQ_VITAL(left == (cvar_t)false_ptr);
        ABQ_VITAL(right == (cvar_t)true_ptr);
        rvalue = 1;
    }
    return rvalue;
}

number_ptr number_create(const cnumber_t number) {
    number_t * rvalue = CREATE_BASE_INSTANCE(number_class, number_t);
    if (NULL != rvalue) {
        *rvalue = number;
    }
    return (number_ptr) rvalue;
}

number_t number_resolve(cvar_t number) {
    number_t retval = abq_nan;
    number_ptr ptr = NULL;
    CLASS_RESOLVE(number_class, number_t, ptr, number);
    if(NULL != ptr) {
        retval = *ptr;
    }
    return retval;
}

number_ptr number_coerce(cvar_t source, cvar_t ref) {
    number_ptr retval = NULL;
    obj_t *obj = obj_lookup(source);
    if (NULL == obj) {
        number_ptr input_value = NULL;
        // Assume unclassified number
        (void) bytes_copy(&input_value, &source, sizeof(cvar_t));
        retval = number_create(*input_value);
        if (NULL != retval) {
            ABQ_INFO_MSG_X("coerced number: ", *(number_ptr) retval);
            (void) obj_takeover(retval, ref);
        }
    } else if (&number_class == obj->meta) {
        (void) bytes_copy(&retval, &obj->member, sizeof(cvar_t));
        VITAL_IS_OK(obj_reserve(retval, ref));
    } else if(&null_class == obj->meta) {
    	// Coerce'd from NULL to NULL
    } else if (&string_class == obj->meta) {
        number_t number = 0.0;
        int32_t bytes = utf8_read_number(str_resolve(source), 64, &number);
        if (-1 != bytes) {
            retval =number_create(number);
            VITAL_IS_OK(obj_takeover(retval, ref));
        }
    } else {
        (void) abq_status_set(ENOSYS, false_const);
    }
    return retval;
}

static cvar_t number_coerce_internal(cvar_t source, cvar_t ref) {
    return (cvar_t) number_coerce(source, ref);
}

int8_t number_compare_number(const number_t left, const number_t right) {
    // TODO: account for NaN, Inf, etc
    int8_t rvalue = 0;
    if (left > right) {
        rvalue = 1;
    } else if (left < right) {
        rvalue = -1;
    } else {
        rvalue = 0;
    }
    return rvalue;
}

int8_t number_compare(cvar_t left, cvar_t right) {
    int8_t retval = 0;
    if ((&number_class == class_of(left))
            && (&number_class == class_of(right))) {
        retval = number_compare_number(number_resolve(left),
                number_resolve(right));
    } else {
        retval = var_compare(left, right);
    }
    return retval;
}
