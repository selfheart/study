//#line 2 "ontrac/var_list.c"
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
 * @file ontrac/var_list.c
 * @date Mar 22, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {Help Me}
 *
 */

#include <ontrac/ontrac/var_list.h>

#ifndef MAX_VLIST_INSTANCES
#define MAX_VLIST_INSTANCES (512U)
#endif /* MAX_VLIST_INSTANCES */

static void vlist_delete(cvar_t old_list);

DEFINE_CLASS(vlist_class, vlist_t, MAX_VLIST_INSTANCES,
        NULL, NULL, NULL, NULL, vlist_delete);

static int8_t link_var_compare(cvar_t item, const link_t *link, compare_function_t item_cmp_fctn) {
    int8_t rvalue;
    if(NULL == link){
        rvalue=-1;
    }else if(NULL !=item_cmp_fctn){
        rvalue=item_cmp_fctn(item, link->item);
    }else { // if no compare function they are the same as far as we are concerned
        rvalue=0;
    }
    return rvalue;
}

vlist_t *vlist_create(class_ptr class_of_items) {
    vlist_t *rvalue = CREATE_BASE_INSTANCE(vlist_class, vlist_t);
    if(NULL != rvalue){
        *rvalue = (vlist_t) {
            // if we were passed a list of 'list_of_*' instead of the real class ...
                .class_of_items = (NULL == class_of_items) ? NULL : class_of_items->class_ref,
                .first=NULL,
                .last=NULL,
                .count=0,
                .sorted=false
        };
    }
    return rvalue;
}

vlist_t* vlist_resolve(cvar_t item) {
    vlist_t * retval = NULL;
    CLASS_RESOLVE(vlist_class, vlist_t, retval, item);
    return retval;
}

static void vlist_delete(cvar_t old_list) {
    VITAL_IS_OK(class_check(&vlist_class, old_list));
    vlist_t * vlist = vlist_resolve(old_list);
    VITAL_NOT_NULL(vlist);
    VITAL_IS_OK(vlist_clear(vlist));
}

bool_t vlist_is_empty(const vlist_t *vlist) {
    bool_t rvalue = true;
    if((NULL != vlist) && (0UL != vlist->count)) {
        rvalue = false;
    }
    return rvalue;
}

int32_t vlist_size(const vlist_t *vlist) {
    int32_t rvalue = -1;
    if(NULL == vlist){
        (void)abq_status_set(EFAULT, false);
        rvalue = -1;
    }else{
        rvalue = (int32_t) vlist->count;
    }
    return rvalue;
}

// lookup the proceeding link in the list
//  where the link would be next placed in the list
//  NULL if we need to insert @ root
//  fifo flag is used to switch between insert before or after equivalent items
static link_t *vlist_locate_parent(vlist_t *vlist, cvar_t item, bool_t fifo) {
    link_t * parent=NULL;
    compare_function_t cmp_fctn = NULL;
    if((vlist->sorted) && (NULL != vlist->class_of_items)) {
        cmp_fctn = vlist->class_of_items->compare;
    }
    int8_t cmp_val = link_var_compare(item, vlist->last, cmp_fctn);
    if(fifo ? (0 <= cmp_val) : (0 < cmp_val)){
        // if the item should be placed after last, return last as parent
        parent = vlist->last;
    }else if(link_var_compare(item, vlist->first, cmp_fctn) > 0) {
        // if the leaf is greater then first, find parent
        FOREACH_LINK(vlist, link) {
            cmp_val = link_var_compare(item, link->next, cmp_fctn);
            if(fifo ? (0 > cmp_val) : (0 >= cmp_val)){
                parent = link;
                break;
            }
        }
        VITAL_NOT_NULL(parent);
    }else{
        // ROOT is the "parent", return NULL
    }
    return parent;
}

// lookup the parent of item already in the list
// A return value of NULL indicates root is the parent,
//  whereas a return value of vlist->last indicates no parent was found
static link_t *vlist_match_parent(vlist_t *vlist, cvar_t item) {
    link_t *parent = NULL;
    compare_function_t cmp_fctn = identity_compare;
    if((NULL != vlist->class_of_items) && (NULL != vlist->class_of_items->compare)) {
        cmp_fctn = vlist->class_of_items->compare;
    }
    if(0 != link_var_compare(item, vlist->first, cmp_fctn)){
        FOREACH_LINK(vlist, link){
            int8_t cmp_val = link_var_compare(item, link->next, cmp_fctn);
            if(0 >= cmp_val){
                if(0 == cmp_val) {
                    // Match
                    parent = link;
                }else if(vlist->sorted){
                    // we have past the location where parent would have been
                }else{
                    continue; // A match may still exist
                }
                break;
            }
        }
        if(NULL == parent) {
            parent = vlist->last;
        }
    }else{
        parent = NULL;
        // root is parent, leave parent as NULL
    }
    return parent;
}

static void vlist_child_insert(vlist_t *vlist, link_t *parent, link_t *child) {
    if(NULL == parent) {
        // indicates parent is root
        LLIST_PUSH(vlist->first, child, next);
        if(NULL == vlist->last) {
            vlist->last = child;
        }
    }else{
        if(parent == vlist->last) {
            vlist->last = child;
        }
        LLIST_PUSH(parent->next, child, next);
    }
    vlist->count++;
}

static err_t vlist_child_create(vlist_t *vlist, link_t *parent, cvar_t item) {
    // internal function, skip the typecheck
    err_t retval = 0;
    link_t *child = NULL;
    var_t block = abq_malloc(sizeof(link_t));
    (void) bytes_copy(&child, &block, sizeof(var_t));
    if (NULL == child) {
        retval = abq_status_take(ENOMEM);
    } else {
        bytes_set(child, '\0', sizeof(link_t));
        if(NULL != item) {
            child->item = classify(item, vlist->class_of_items, (cvar_t)vlist);
            if(NULL == child->item){
                retval = abq_status_pop();
            }
        }
        if (EXIT_SUCCESS != retval) {
            abq_free((cvar_t)child);
        }else{
            vlist_child_insert(vlist, parent, child);
        }
    }
    if (EXIT_SUCCESS == retval) {
        if (NULL == child) {
            retval = ENOBUFS;
        }
    }
    return retval;
}

static cvar_t vlist_child_disown(vlist_t *vlist, link_t *parent, cvar_t ref) {
    link_t *child = NULL;
    if (NULL == parent) {
        VITAL_NOT_NULL(vlist->first);
        child = vlist->first;
        vlist->first = child->next;
        child->next = NULL;
        if (NULL == vlist->first) {
            vlist->last = NULL;
        }
    } else {
        VITAL_NOT_NULL(parent->next);
        child = parent->next;
        parent->next = child->next;
        if (vlist->last == child) {
            vlist->last = parent;
        }
        // child->next = NULL : by not setting this to null,
        //  we can continue looping after removing current link within loop
        //  TODO: check for concurrent modifications errors
    }
    cvar_t retval = child->item;
    if (NULL == ref) {
        (void) obj_release(child->item, (cvar_t) vlist);
        retval = NULL;
    } else {
#if !defined(NDEBUG)
        obj_t* obj = obj_lookup(child->item);
        if (NULL != obj) {
            uint16_t index = obj->refs_max;
            cvar_t match = NULL;
            ABQ_SEEK_LAST(obj->refs, cvar_t,
                    obj->refs_len, index, match, (match == vlist));
            FATAL_IF(index >= obj->refs_len);
            // Replace the anchor with new reference
            obj->refs[index] = (cvar_t) ref;
        }else{
            // Unclassified item, no need to updated refs
        }
#endif /* NDEBUG */
    }
    // Free memory used by the list of the item
    abq_free((cvar_t)child);
    // And finally decrement list->count
    vlist->count -= 1UL;
    return retval;
}

err_t vlist_add(vlist_t *vlist, cvar_t item) {
    err_t retval = CHECK_NULL(vlist);
    if (EXIT_SUCCESS == retval) {
        LOCK_CLASS(vlist_class);
        link_t *parent = vlist_locate_parent(vlist, item, true);
        retval = vlist_child_create(vlist, parent, item);
        UNLOCK_CLASS(vlist_class);
    }
    return retval;
}

err_t vlist_insert(vlist_t *vlist, size_t index, cvar_t item) {
    err_t retval = CHECK_NULL(vlist);
    if (EXIT_SUCCESS == retval) {
        LOCK_CLASS(vlist_class);
        if (vlist->sorted) {
            retval = ENOSYS;
        } else if (index > vlist->count) {
            // Index out of bounds exception
            retval = ERANGE;
        } else if (index == 0UL) {
            retval = vlist_child_create(vlist, NULL, item);
        } else {
            link_t *parent = vlist->first;
            size_t location = 1UL;
            while (index > location) {
                parent = parent->next;
                location++;
            }
            retval = vlist_child_create(vlist, parent, item);
        }
        UNLOCK_CLASS(vlist_class);
    }
    return retval;
}

cvar_t vlist_get(const vlist_t *vlist, size_t index) {
    cvar_t rvalue = NULL;\
    if(NULL == vlist) {
        (void) abq_status_set(EFAULT, false);
    }else if(index >= vlist->count){
        (void) abq_status_set(ERANGE, false);
    }else{
        LOCK_CLASS(vlist_class);
        link_t *link = vlist->last;
        if((index+1UL) != vlist->count){
            size_t location = 0;
            link = vlist->first;
            while(index > location) {
                link = link->next;
                location++;
            }
        }
        rvalue = link->item;
        UNLOCK_CLASS(vlist_class);
    }
    return rvalue;
}

cvar_t vlist_first(const vlist_t *vlist) {
    link_t *link = NULL;
    if(NULL == vlist) {
        (void) abq_status_set(EFAULT, false);
    }else{
        // This is basically atomic anyway, skipping locking
        link = vlist->first;
    }
    return (NULL==link) ? NULL : link->item;
}

cvar_t vlist_last(const vlist_t *vlist) {
    link_t *link = NULL;
    if(NULL == vlist) {
        (void) abq_status_set(EFAULT, false);
    }else{
        // This is basically atomic anyway, skipping locking
        link = vlist->last;
    }
    return (NULL==link) ? NULL : link->item;
}

cvar_t vlist_match(vlist_t *vlist, cvar_t item) {
    cvar_t rvalue = NULL;
    if(NULL == vlist) {
        (void) abq_status_set(EFAULT, false);
    }else if(NULL == vlist->class_of_items){
        (void) abq_status_set(ENOSYS, false);
    }else{
        LOCK_CLASS(vlist_class);
        link_t *parent = vlist_match_parent(vlist, item);
        if (parent != vlist->last) {
            if (NULL == parent) {
                rvalue = vlist->first->item;
            } else {
                rvalue = parent->next->item;
            }
        }
        UNLOCK_CLASS(vlist_class);
    }
    return rvalue;
}

bool_t vlist_contains(vlist_t *vlist, cvar_t item) {
    bool_t rvalue = false;
    if(NULL == vlist) {
        rvalue = false;
    } else {
        LOCK_CLASS(vlist_class);
        link_t *parent = vlist_match_parent(vlist, item);
        if (parent != vlist->last) {
            rvalue = true;
        } else {
            rvalue = false;
        }
        UNLOCK_CLASS(vlist_class);
    }
    return rvalue;
}

int32_t vlist_index_of(vlist_t *vlist, cvar_t item, size_t from_index) {
    int32_t index = -1;
    if(NULL == vlist) {
        (void) abq_status_set(EFAULT, false);
    }else if(from_index >= vlist->count){
        (void) abq_status_set(ERANGE, false);
    } else {
        LOCK_CLASS(vlist_class);
        int8_t cmp_val = -1;
        compare_function_t cmp_fctn = identity_compare;
        if((NULL != vlist->class_of_items)
                && (NULL != vlist->class_of_items->compare)) {
            cmp_fctn = vlist->class_of_items->compare;
        }
        FOREACH_LINK(vlist, link) {
            index += 1;
            if (index >= (int32_t)from_index) {
                cmp_val = link_var_compare(item, link, cmp_fctn);
                if(0 >= cmp_val){
                    if(0 == cmp_val) {
                        // Match
                    }else if(vlist->sorted){
                        // we have past the location where parent would have been
                    }else{
                        continue; // A match may still exist
                    }
                    break;
                }
            }
        }
        if(0 != cmp_val) {
            // Failed to find a match;
            index = -1;
        }
        UNLOCK_CLASS(vlist_class);
    }
    return index;
}


err_t vlist_remove(vlist_t *vlist, cvar_t item) {
    err_t retval = CHECK_NULL(vlist);
    if (EXIT_SUCCESS == retval) {
        LOCK_CLASS(vlist_class);
        // default to a Not-Found err code
        retval = EALREADY;
        if(NULL != vlist->first){
            link_t *parent = NULL;
            parent = vlist_match_parent(vlist, item);
            if(parent != vlist->last) {
                (void) vlist_child_disown(vlist, parent, NULL);
                retval = EXIT_SUCCESS;
            }
        }
        UNLOCK_CLASS(vlist_class);
    }
    return retval;
}

cvar_t vlist_pop(vlist_t *vlist, cvar_t ref) {
    cvar_t retval = NULL;
    LOCK_CLASS(vlist_class);
    if((NULL != vlist) && (NULL != vlist->first)) {
        retval = vlist_child_disown(vlist, NULL,
                ((NULL != ref) ? ref : vlist->first->item));
    }
    UNLOCK_CLASS(vlist_class);
    return retval;
}

cvar_t vlist_pop_at(vlist_t *vlist, cvar_t ref, size_t index) {
    cvar_t retval = NULL;
    if(NULL == vlist) {
        (void) abq_status_set(EFAULT, false);
    }else if(index >= vlist->count){
        (void)abq_status_set(ERANGE, false);
    }else{
        LOCK_CLASS(vlist_class);
        if (0UL == index) {
            retval = vlist_child_disown(vlist, NULL,
                    ((NULL != ref) ? ref : vlist->first->item));
        }else{
            size_t location = 1;
            link_t *parent = vlist->first;
            while(index > location) {
                parent = parent->next;
                location++;
            }
            retval = vlist_child_disown(vlist, parent,
                    ((NULL != ref) ? ref : parent->next->item));
        }
        UNLOCK_CLASS(vlist_class);
    }
    return retval;
}

err_t vlist_clear(vlist_t *vlist) {
    err_t rvalue=CHECK_NULL(vlist);
    if (EXIT_SUCCESS == rvalue){
        LOCK_CLASS(vlist_class);
        while (0UL != vlist->count) {
            (void) vlist_child_disown(vlist, NULL, NULL);
        }
        UNLOCK_CLASS(vlist_class);
    }
    return rvalue;
}

err_t vlist_set_sorted(vlist_t *vlist, bool_t sorted) {
    err_t rvalue = CHECK_NULL(vlist);
    if (EXIT_SUCCESS != rvalue){
        // do nothing
    }else if(NULL == vlist->class_of_items){
        rvalue = ENODATA;
    }else if (NULL == vlist->class_of_items->compare){
        rvalue = ENOSYS;
    }else if(sorted == vlist->sorted){
        // EALREADY ?
    }else {
        LOCK_CLASS(vlist_class);
        vlist->sorted = sorted;
        link_t *chain = vlist->first;
        // clear the list of any knowledge
        vlist->first = NULL;
        vlist->last = NULL;
        vlist->count = 0;
        // re-add the items to the list
        //  placing them in sorted order
        while (NULL != chain) {
            // pop link from chain
            link_t *link = chain;
            chain = link->next;
            // NULL out linkage so that no assert error when inserting link into new list
            link->next = NULL;
            // and add it to the list
            link_t *parent = vlist_locate_parent(vlist, link->item, true);
            vlist_child_insert(vlist, parent, link);
        }
        UNLOCK_CLASS(vlist_class);
    }
    return rvalue;
}

int8_t vlist_compare_vlist(const vlist_t *left, const vlist_t *right) {
    int8_t rvalue = 0;
    if (NULL == left) {
        rvalue = (int8_t) ((NULL == right) ? 0 : 1);
    } else if (NULL == right) {
        rvalue = -1;
    } else {
        compare_function_t comp_fctn = var_compare;
        if (NULL != left->class_of_items) {
            if (NULL != right->class_of_items) {
                rvalue = class_class.compare((cvar_t) left->class_of_items,
                        (cvar_t) right->class_of_items);
            }
            if (NULL != left->class_of_items->compare) {
                comp_fctn = left->class_of_items->compare;
            }
        } else if ((NULL != right->class_of_items)
                && (NULL != right->class_of_items->compare)) {
            comp_fctn = right->class_of_items->compare;
        } else {
            // leave comp_fctn as var_compare
        }
        link_t *left_link = left->first;
        link_t *right_link = right->first;
        while (0 == rvalue) {
            if (NULL == left_link) {
                if (NULL == right_link) {
                    ABQ_VITAL(0 == rvalue);
                    break; // end of list, everything matched
                } else {
                    // left list has more items then right list
                    rvalue = 1;
                }
            } else if (NULL == right_link) {
                // right list has more items then left list
                rvalue = -1;
            } else {
                rvalue = comp_fctn(left_link->item, right_link->item);
                // iterate to the next link and compare those values (after while (0 == rvalue) check)
                left_link = left_link->next;
                right_link = right_link->next;
            }
        };
    }
    return rvalue;
}

vlist_t *vlist_from_array(class_ptr item_type, cvar_t *array, size_t size) {
    vlist_t *vlist = vlist_create(item_type);
    if(NULL != vlist) {
        for(size_t i=0UL;i<size;i++) {
            err_t err = vlist_add(vlist, array[i]);
            if(EXIT_SUCCESS != err){
                (void)abq_status_set(err, false);
                VITAL_IS_OK(obj_release_self(vlist));
                vlist=NULL;
                break;
            }
        }
    }
    return vlist;
}
