//#line 2 "ontrac/abq_props.c"
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
 * @file ontrac/abq_props.c
 * @date Apr 17, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {Help Me}
 *
 */

#include <ontrac/ontrac/abq_props.h>
#include <ontrac/ontrac/abq_str.h>

static void ptree_delete(cvar_t old_tree);
static ptree_t *ptree_properties_of(cvar_t item);

#ifndef MAX_DYNAMIC_PROPERTIES
#define MAX_DYNAMIC_PROPERTIES (1024U)
#endif /* MAX_DYNAMIC_PROPERTIES */

#ifndef MAX_PTREE_INSTANCES
#define MAX_PTREE_INSTANCES (1024U)
#endif /* MAX_PTREE_INSTANCES */

static cvar_t local_property_getter(cvar_t item, const property_t *prop);
static err_t local_property_setter(cvar_t item, property_t *prop, cvar_t value);
STATIC_ITEM_POOL(property_t, MAX_DYNAMIC_PROPERTIES, local_properties);
EMPTY_CLASS(property_class, void, NULL, NULL, NULL,
        NULL, local_property_getter, local_property_setter,
        invalid_delete, &property_class, static);

static int8_t ptree_compare(cvar_t left, cvar_t right);
DEFINE_CLASS(ptree_class, ptree_t, MAX_PTREE_INSTANCES, NULL,
                    NULL, ptree_compare, ptree_properties_of, ptree_delete);

static inline bool_t property_is_global(const property_t *prop) {
    return (bool_t)(&property_class != prop->class_of_value);
}

static inline int8_t property_compare_names(const property_t *prop, cstr_t name) {
    int8_t retval = 1;
    if (NULL != prop) {
        retval = utf8_compare_insensitive(prop->name, name, -1);
    }
    return retval;
}

class_ptr property_get_class(const property_t *prop) {
    return (property_is_global(prop))
            ? prop->class_of_value : NULL;
}

cvar_t property_get_value(const property_t *prop, cvar_t item) {
    cvar_t retval = NULL;
    if ((NULL== item) || (NULL == prop)) {
        // Not found
    } else if (NULL == prop->class_of_value) {
        // Global-property with class: unspecified
        retval = global_property_getter(item, prop);
    } else {
        retval = prop->class_of_value->getter(item, prop);
    }
    return retval;
}

err_t property_set_value(property_t *prop, cvar_t item, cvar_t value) {
    err_t retval = EXIT_SUCCESS;
    if ((NULL== item) || (NULL == prop)) {
        // create a leaf and fatal if error
        retval = EFAULT;
    } else if(NULL == prop->class_of_value) {
        // Global-property with class: unspecified
        retval = global_property_setter(item, prop, value);
    } else {
        retval = prop->class_of_value->setter(item, prop, value);
    }
    return retval;
}

static int8_t property_compare(const property_t *left_prop, cvar_t left_item,
        const property_t *right_prop, cvar_t right_item) {
    int8_t retval = 0;
    if (NULL == left_prop) {
        retval = (int8_t) ((NULL == right_prop) ? 0 : 1);
    } else if(NULL == right_prop) {
        retval = -1;
    } else {
        retval = utf8_compare_exact(left_prop->name, right_prop->name, -1);
        if (0 != retval) {
            // Return value as is
        } else {
            retval = var_compare(property_get_value(left_prop, left_item),
                    property_get_value(right_prop, right_item));
        }
    }
    return retval;
}

static cvar_t local_property_getter(cvar_t item, const property_t *prop) {
    VITAL_NOT_NULL(item);
    VITAL_NOT_NULL(prop);
    // Must be a local property
    FATAL_IF(property_is_global(prop));
    return prop->value;

}

cvar_t global_property_getter(cvar_t item, const property_t *prop) {
    VITAL_NOT_NULL(item);
    VITAL_NOT_NULL(prop);
    // Must be a global property
    ABQ_VITAL(property_is_global(prop));
    uintptr_t field_offset = ptr2addr(prop->value);
    // Assume all structures are less them INT16_MAX in size
    //   More than 32767 bytes in an object: MISRAC2012-RULE_1_1-a
    ABQ_VITAL(field_offset <= (uintptr_t)(INT16_MAX));
    cvar_t* field = item_get_field(item, (size_t)field_offset);
    VITAL_NOT_NULL(field);
    return *field;
}

static err_t local_property_setter(cvar_t item, property_t *prop, cvar_t value) {
    VITAL_NOT_NULL(item);
    VITAL_NOT_NULL(prop);
    // Must be a local property
    FATAL_IF(property_is_global(prop));
    err_t retval = (NULL == prop->value) ? EXIT_SUCCESS : EALREADY;
    item_field_set(item, &prop->value, value);
    return retval;
}


err_t global_property_setter(cvar_t item, property_t *prop, cvar_t value) { /* parasoft-suppress MISRAC2012-RULE_8_13-a-4 "m0018. Not using 'const' specifier to adhere to callback type." */
    VITAL_NOT_NULL(item);
    VITAL_NOT_NULL(prop);
    // Must be a global property
    ABQ_VITAL(property_is_global(prop));
    err_t retval = EXIT_SUCCESS;
    uintptr_t field_offset = ptr2addr(prop->value);
    // Assume all structures are less them INT16_MAX in size
    //   More than 32767 bytes in an object: MISRAC2012-RULE_1_1-a
    ABQ_VITAL(field_offset <= (uintptr_t)(INT16_MAX));
    cvar_t* field = item_get_field(item, (size_t)field_offset);
    VITAL_NOT_NULL(field);
    cvar_t old_value = *field;
    *field = classify(value, prop->class_of_value, item);
    if ((NULL == *field) && (NULL != value)) {
        // revert to old_value (if any_
        *field = old_value;
        // and return error as appropriate
        retval = abq_status_take(EINVAL);
    } else {
        // Release old_value as able
        (void) obj_release(old_value, item);
    }
    return retval;
}

static err_t ptree_create_leaf(ptree_t *ptree, const cstr_t name, cvar_t value) {
    // No existing leaf existed with given value, must create one
    err_t rvalue = EXIT_SUCCESS;
    property_t *leaf = NULL;
    ITEM_POOL_ALLOC(&local_properties, property_t, leaf);
    if(NULL == leaf){
        rvalue = abq_status_take(ENOMEM);
    } else {
        if (NULL == ptree->owner) {
            ptree->owner = ptree;
        }
        // allow un"class"ified data to be stored in tree
        cstr_t name_str = str_coerce(name, ptree->owner);
        if(NULL == name_str) {
            rvalue = abq_status_take(EINVAL);
            ITEM_POOL_FREE(&local_properties, property_t, leaf);
        }else{
            *leaf = (property_t) {
                    .name = name_str,
                    .value = value,
                    .next = NULL,
                    .class_of_value = &property_class
            };
            // Always push new props prior to globals
            LLIST_PUSH(ptree->props, leaf, next);
            (void) obj_reserve(value, ptree->owner);
            ptree->count += 1UL;
        }
    }
    return rvalue;
}

static void ptree_remove_leaf(ptree_t *ptree, property_t *parent) {
    property_t *prop;
    if(NULL == parent) {
        LLIST_POP(ptree->props, prop, next);
    }else{
        LLIST_POP(parent->next, prop, next);
    }
    VITAL_NOT_NULL(prop);
    FATAL_IF(property_is_global(prop));
    (void) obj_release(prop->value, ptree->owner);
    (void) obj_release(prop->name, ptree->owner);
    ITEM_POOL_FREE(&local_properties, property_t, prop);
    ptree->count -= 1UL;
}

ptree_t *ptree_create(LLIST_HEAD_TYPE(property_t) globals) {
    ptree_t *rvalue = CREATE_BASE_INSTANCE(ptree_class, ptree_t);
    if(NULL != rvalue){
        size_t count = 0U;
        LLIST_FOREACH (globals, property_t, prop, next) {
            ABQ_VITAL(property_is_global(prop));
            count++;
        }
        *rvalue = (ptree_t) {
                .props=globals,
                .count=count,
                .owner = (cvar_t) rvalue
        };
    }
    return rvalue;
}

static void ptree_delete(cvar_t old_tree) {
    ptree_t * tree = ptree_resolve(old_tree);
    VITAL_NOT_NULL(tree);
    VITAL_IS_OK(ptree_clear(tree));
}

void ptree_set_owner(ptree_t *ptree, cvar_t owner) {
    VITAL_NOT_NULL(ptree);
    VITAL_NOT_NULL(owner);
    LOCK_CLASS(ptree_class);
    (void)obj_reserve((cvar_t)ptree, owner);
    (void)obj_release((cvar_t)ptree, ptree->owner);
    LLIST_FOREACH(ptree->props, property_t, prop, next) {
        if (property_is_global(prop)) {
            break; // Completed all local-properties
        } else {
            (void)obj_reserve(prop->name, owner);
            (void)obj_release(prop->name, ptree->owner);
            if (NULL != prop->value) {
                // first reserve the value with the new owner
                (void) obj_reserve(prop->value, owner);
                // then release the old owner's reference to the value
                (void) obj_release(prop->value, ptree->owner);
            }
        }
    }
    // update the owning reference
    ptree->owner = owner;
    UNLOCK_CLASS(ptree_class);
}

bool_t ptree_is_empty(const ptree_t *ptree) {
    bool_t rvalue = true;
    if (NULL != ptree) {
        property_t* prop = LLIST_FIRST(ptree->props);
        if((NULL != prop) && (false == property_is_global(prop))){
            rvalue = false;
        }
    }
    return rvalue;
}

int32_t ptree_size(const ptree_t *ptree) {
    int32_t rvalue = -1;
    if (NULL != ptree) {
        rvalue = (int32_t) ptree->count;
    }
    return rvalue;
}

// loads the property if an exact match is found
//  also loads the parent property of where it should be
//  if it was to be created.
// A parent value of NULL indicates that it
//  should be placed at the head of the list
static property_t* ptree_find_property(const ptree_t *ptree, const cstr_t name) {
    property_t *retval=NULL;
    LLIST_LOOP(ptree->props, retval, next) {
        if (0 == property_compare_names(retval, name)) {
            break;
        }
    }
    return retval;
}

err_t ptree_put(ptree_t *ptree, const cstr_t name, cvar_t value) {
    err_t retval = CHECK_NULL(ptree);
    if (EXIT_SUCCESS != retval) {
        // Return error code as is
    } else {
        LOCK_CLASS(ptree_class);
        property_t *match= ptree_find_property(ptree, name);
        if (NULL != match) {
            // need to use 'property_set' to set global properties
            FATAL_IF(property_is_global(match));
            // Previous entry found matching the given name, simply replace the value
            item_field_set(ptree->owner, &match->value, value);
            retval = EALREADY;
        } else {
            // create a leaf and fatal if error
            retval = ptree_create_leaf(ptree, name, value);
        }
        UNLOCK_CLASS(ptree_class);
    }
    return retval;
}

err_t ptree_put_if_absent(ptree_t *ptree, const cstr_t name, cvar_t value) {
    err_t retval = CHECK_NULL(ptree);
    if (EXIT_SUCCESS == retval) {
        LOCK_CLASS(ptree_class);
        property_t *match = ptree_find_property(ptree, name);
        if (NULL == match) {
            // create a leaf and fatal if error
            retval = ptree_create_leaf(ptree, name, value);
        }else{
            retval = EEXIST;
        }
        UNLOCK_CLASS(ptree_class);
    }
    return retval;
}

cvar_t ptree_get(const ptree_t *ptree, const cstr_t name) {
    cvar_t retval = NULL;
    if(NULL == ptree) {
        // (void) abq_status_set(EFAULT, false)
    } else {
        LOCK_CLASS(ptree_class);
        property_t *match = ptree_find_property(ptree, name);
        if (NULL != match) {
            // need to use 'property_get' to get global properties
            FATAL_IF(property_is_global(match));
            retval = match->value;
        }
        UNLOCK_CLASS(ptree_class);
    }
    return retval;
}

cvar_t ptree_take(ptree_t *ptree, const cstr_t name, cvar_t ref) {
    cvar_t retval = NULL;
    if (NULL == ptree){
        (void) abq_status_set(EFAULT, false);
    } else {
        // only check properties under locals
        LOCK_CLASS(ptree_class);
        property_t *last=NULL, *prop = LLIST_FIRST(ptree->props);
        while((NULL != prop) && (!property_is_global(prop))) {
            if (0 == property_compare_names(prop, name)) {
                retval = prop->value;
                (void) obj_reserve(retval, ref);
                ptree_remove_leaf(ptree, last);
                break;
            }
            last = prop;
            prop = LLIST_NEXT(last, next);
        }
        UNLOCK_CLASS(ptree_class);
    }
    return retval;
}
err_t ptree_detach(ptree_t *ptree, const cstr_t name) {
    err_t retval = EALREADY;
    if(NULL == ptree){
        retval = EFAULT;
    } else {
        // only check properties under locals
        LOCK_CLASS(ptree_class);
        property_t *last=NULL, *prop  = LLIST_FIRST(ptree->props);
        while((NULL != prop) && (!property_is_global(prop))) {
            if (0 == property_compare_names(prop, name)) {
                ptree_remove_leaf(ptree, last);
                break;
            }
            last = prop;
            prop = LLIST_NEXT(last, next);
        }
        UNLOCK_CLASS(ptree_class);
    }
    return retval;
}

err_t ptree_clear(ptree_t *ptree) {
    err_t rvalue = CHECK_NULL(ptree);
    if(EXIT_SUCCESS == rvalue) {
        LOCK_CLASS(ptree_class);
        while (false == ptree_is_empty(ptree)) {
            ptree_remove_leaf(ptree, NULL);
        }
        UNLOCK_CLASS(ptree_class);
    }
    return rvalue;
}

bool_t ptree_has_key(const ptree_t *ptree, const cstr_t name) {
    bool_t rvalue = false;
    if(NULL == ptree){
        (void) abq_status_set(EFAULT, false);
    } else {
        LOCK_CLASS(ptree_class)
        property_t *match = ptree_find_property(ptree, name);
        if(NULL != match){
            rvalue = true;
        }
        UNLOCK_CLASS(ptree_class);
    }
    return rvalue;
}

bool_t ptree_has_value(const ptree_t *ptree, cvar_t value,
        compare_function_t value_compare){
    bool_t retval = false;
    LOCK_CLASS(ptree_class);
    LLIST_FOREACH(ptree->props, property_t, prop, next) {
        if(property_is_global(prop)) {
            break;
        }else if(NULL != value_compare) {
            if(0 == value_compare(value, prop->value)) {
                retval = true;
            }
        } else {
            if(0 == var_compare(value, prop->value)) {
                retval = true;
            }
        }
    }
    UNLOCK_CLASS(ptree_class);
    return retval;
}

ptree_t* ptree_resolve(cvar_t item) {
    ptree_t * retval = NULL;
    CLASS_RESOLVE(ptree_class, ptree_t, retval, item);
    return retval;
}

static ptree_t *ptree_properties_of(cvar_t item) {
    return ptree_resolve(item);
}

static int8_t ptree_compare(cvar_t left, cvar_t right) {
    int8_t retval = 0;
    ptree_t* first =  ptree_resolve(left);
    ptree_t* second = ptree_resolve(right);
    if((NULL == first) || (NULL == second)) {
        retval = identity_compare(left, right);
    } else if(left == right) {
        // no need to compare properties
    } else {
        property_t *left_prop = LLIST_FIRST(first->props);
        property_t *right_prop = LLIST_FIRST(second->props);
        while((NULL != left_prop) || (NULL != right_prop)) {
            retval = property_compare(left_prop, left, right_prop, right);
            if (0 != retval) {
                break; // Found a difference in properties or their values
            }
            // Iterate to next set of properties
            VITAL_NOT_NULL(left_prop);
            left_prop = LLIST_NEXT(left_prop, next);
            VITAL_NOT_NULL(right_prop);
            right_prop = LLIST_NEXT(right_prop, next);
        }
    }
    return retval;
}

static ptree_t *properties_of_item(cvar_t item, class_ptr item_meta) {
    ptree_t *rvalue = NULL;
    class_ptr meta = (NULL == item_meta) ? class_of(item) : item_meta;
    if ((NULL == item) || (NULL == meta)
            || (NULL == meta->properties_of)) {
        // NULL & unclassified items have no properties
    } else {
        rvalue = meta->properties_of(item);
    }
    return rvalue;
}


ptree_t *ptree_clone_properties(cvar_t target, bool_t deep_clone) {
    ptree_t *retval = NULL;
    ptree_t *clonable = properties_of_item(target, NULL);
    if (NULL == target) {
        retval = ptree_create(NULL);
    } else if (NULL == clonable) {
        (void) abq_status_set(EINVAL, false);
    } else {
        FATAL_IF(deep_clone);
        retval = ptree_create(NULL);
        if (NULL == retval) {
            (void) abq_status_set(ENOMEM, false);
        } else {
            LOCK_CLASS(ptree_class);
            LLIST_FOREACH(clonable->props, property_t, prop, next) {
                err_t err = ptree_put(retval,
                        prop->name, property_get_value(prop, target));;
                if (EXIT_SUCCESS != err) {
                    (void) obj_release_self((cvar_t) retval);
                    (void) abq_status_set(err, false);
                    retval = NULL;
                    break;
                }
                // copy class info?
            }
            UNLOCK_CLASS(ptree_class);
        }
    }
    return retval;
}

err_t transfer_properties(cvar_t source, var_t dest) {
    err_t rvalue = CHECK_NULL(source);
    if (0 == rvalue) {
        rvalue = CHECK_NULL(dest);
    }
    if (0 == rvalue) {
        LOCK_CLASS(ptree_class);
        ptree_t *clonables = properties_of_item(source, NULL);
        ptree_t *targetables = properties_of_item(dest, NULL);
        if ((NULL == clonables) || (NULL == targetables)) {
            rvalue = abq_status_take(EINVAL);
        } else {
            err_t rv = 0;
            LLIST_FOREACH(clonables->props, property_t, prop, next) {
                property_t *match = ptree_find_property(targetables, prop->name);
                if(NULL == match) {
                    rv = ptree_create_leaf(targetables,
                            prop->name, property_get_value(prop, source));
                }else{
                    rv = property_set_value(match, dest,
                            property_get_value(prop, source));
                }
                if (0 == rvalue) {
                    rvalue = rv;
                }
            }
        }
        UNLOCK_CLASS(ptree_class);
    }
    return rvalue;
}

bool_t item_has_property(cvar_t item, class_ptr meta, const property_t *property) {
    bool_t retval = false;
    ptree_t* ptree = properties_of_item(item, meta);
    if (NULL != ptree) {
		property_t *prop = NULL;
        LLIST_LOOP(ptree->props, prop, next){
			if(property == prop) {
				retval = true;
				break;
			}
        }
    }
    return retval;
}

property_t *property_by_name(cvar_t item, class_ptr item_meta, const cstr_t name) {
    property_t *match = NULL;
    LOCK_CLASS(ptree_class);
    ptree_t *props = properties_of_item(item, item_meta);
    if(NULL == props) {
        if(NULL != item) {
            (void)abq_status_set(ENOSYS, false);
        }
    }else{
        match = ptree_find_property(props, name);
        if(match == NULL){
            // create a leaf and fatal if error
            err_t status = ptree_create_leaf(props, name, NULL);
            if(status_code_is_ok(status)){
                match = ptree_find_property(props, name);
            }
        }
    }
    UNLOCK_CLASS(ptree_class);
    return match;
}

cvar_t property_get(cvar_t item, const cstr_t name) {
    cvar_t retval = NULL;
    LOCK_CLASS(ptree_class);
    property_t * match = property_by_name(item, NULL, name);
    retval = property_get_value(match, item);
    UNLOCK_CLASS(ptree_class);
    return retval;
}

// see also ptree_put
err_t property_set(cvar_t item, class_ptr item_meta, const cstr_t name, cvar_t value) {
    err_t retval = EXIT_SUCCESS;
    LOCK_CLASS(ptree_class);
    ptree_t *props = properties_of_item(item, item_meta);
    if (NULL == props) {
        retval = abq_status_take(EPERM);
    } else {
        property_t *match = ptree_find_property(props, name);
        if (match == NULL) {
            // create a leaf and fatal if error
            retval = ptree_create_leaf(props, name, value);
        } else {
            retval = property_set_value(match, item, value);
        }
    }
    UNLOCK_CLASS(ptree_class);
    return retval;
}

err_t props_release_globals(cvar_t item, LLIST_HEAD_DEF(property_t, globals)){
    LLIST_FOREACH(globals, property_t, prop, next) {
        cvar_t field = property_get_value(prop, item);
        (void) obj_release(field, item);
    }
    return EXIT_SUCCESS;
}
