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
 * @file ontrac/traverser.h
 * @date May 29, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief Utility class for walking or "traversing" over a tree of 'class'ified items without using recursion
 *
 */

#ifndef ONTRAC_ONTRAC_TRAVERSER_H_
#define ONTRAC_ONTRAC_TRAVERSER_H_
#include <ontrac/ontrac/abq_props.h>
#include <ontrac/ontrac/var_list.h>

#ifndef MAX_TREE_DEPTH
#define MAX_TREE_DEPTH (32U)
#endif /* MAX_TREE_DEPTH */

typedef enum {
	ABQ_NODE_INVALID=0, // Uninitailized
	ABQ_NODE_OBJECT,
	ABQ_NODE_VARLIST,
	/** Primitives & the root node after it has been finalized */
	ABQ_NODE_COMPLETE
} abq_nodetype_t;

typedef union for_nodeiter { // parasoft-suppress MISRA2012-RULE-19_2-4 "Using union intentionally"
    /** ABQ_NODE_OBJECT: If iterating through a set of named properties (JSON Object) such as a ptree_t */
    property_t *nodeprop;
    /** ABQ_NODE_VARLIST: If iterating  through a vlist_t  */
    link_t *nodelink;
} nodeiter_t;

typedef struct for_abq_node abq_node_t;
struct for_abq_node {
	/** instance of the current node over which we are iterating */
    cvar_t item;
	/** class_t associated with of the current node and above item */
	class_ptr meta;
	/** Type of node in the traverser */
	abq_nodetype_t nodetype;
	/** Current property_t / link_t of the item as we iterate over it */
	nodeiter_t iter;
    /** -1 for new item state, 0 for endof_item state & empty container
     *  else 1 for first item and increments by 1 each time another child is encountered */
    int16_t child_counter;
};

static inline cstr_t abq_node_child_name(abq_node_t* node) {
    cstr_t retval = NULL;
	if((ABQ_NODE_OBJECT == node->nodetype) && (NULL != node->iter.nodeprop)) {
		retval = node->iter.nodeprop->name;
	}
	return retval;
}

typedef struct for_traverser traverser_t;
struct for_traverser {
	/** Active depth of the traverser_t as we iterate over items */
	size_t depth;
	/** stack of the traverser_t for when iterating recursively */
	abq_node_t stack[MAX_TREE_DEPTH];
    /** true if we want to dive into properties of children and children's children, etc. False otherwise */
    bool_t is_recursive;
};

/** class for traverser_t instances */
extern const class_t traverser_class;
/**
 * @brief allocates and initializes an new instance of traverser_t*
 *
 * @param item: pointer an item to be traversed (NULL for de-serialization)
 * @param meta: associated class_t of item to be traversed
 * @param ref: reference preventing traversr_t instance from being garbage-collected. NULL for a self-ref
 * @return a newly allocated and initialized traverser_t instance, or NULL on error
 */
extern traverser_t *traverser_create(cvar_t item, class_ptr meta, bool_t is_recursive, cvar_t ref);

extern err_t traverser_reinit(traverser_t* traverser, cvar_t item, class_ptr meta);

/**
 * @brief walks through the tree of objects. when it encounters a new object the new object will have a NULL property for first and last step
 *
 * @param traverser: pointer to the traverser we wish to step through
 * @return 0 on successful step, ECANCELED when end of tree is reached, else other error code on failure
 */
extern err_t traverser_step(traverser_t *traverser);

extern bool_t traverser_has_child(traverser_t *traverser);
/**
 * @brief Loads the first node with a completed value into pointer, then iterates and returns true if found, or false for entire tree has been traversed
 *
 * @param traverser: pointer to the traverser we wish to step through
 * @param name: to be loaded with a pointer first node's property name (if any), or NULL for list iterations (consider index in future?)
 * @param value: to be loaded with a pointer to first node's value when found
 * @return true if a completed node value was found and loaded, false on end of iteration
 */
extern bool_t traverser_next(traverser_t *traverser, cstr_t *name, cvar_t *value);
/**
 * @brief increments the iteration stack and loads current traverser data into the "parent" field, replacing it with data for the item provided
 *
 * @param traverser: pointer to the traverser we wish to step through
 * @param item: the new iteration target if any, else will attempt to create a new target from item_meta
 * @param item_meta: the class_t of the new iteration target, if NULL will attempt to load it with class_of(item)
 * @return 0 on success, else an error code
 */
extern err_t traverser_begin_node(traverser_t *traverser, cvar_t item, class_ptr item_meta);

extern err_t traverser_set_child(traverser_t *traverser,
        abq_node_t* node, cvar_t item);

/**
 * @brief Uses class_of_item.coerce to validate the newly completed item, then internally invokes traverser_finish_item before marking the property as completed
 * @param traverser: pointer to the traverser we are using to deserialize an item
 * @return 0 on success, else an error code
 */
extern err_t traverser_finalize_node(traverser_t *traverser);

#define TYPE_PROPERTIES_OF_NAME(className) className ## _props
//#define TYPE_NESTED_DELETE_NAME(className) className ## _nested_delete
#define TYPE_DELETE_NAME(className) className ## _del
/** Defines a "serializable" object based on a linked-list of global properties for the instances */
#define DEFINE_SERIALIZABLE_CLASS(className, type, max_n, coerce_, compare_, delete_, globals_, ...)  \
static ptree_t *TYPE_PROPERTIES_OF_NAME(className)(cvar_t item);                        \
static void TYPE_DELETE_NAME(className)(cvar_t old_item);                               \
DEFINE_CLASS(className, type, max_n, NULL, coerce_, compare_,                           \
        TYPE_PROPERTIES_OF_NAME(className), TYPE_DELETE_NAME(className), __VA_ARGS__ ); \
static ptree_t *TYPE_PROPERTIES_OF_NAME(className)(cvar_t item) {                       \
    ptree_t *rvalue = NULL;                                                             \
    type *instance = NULL;                                                              \
    CLASS_RESOLVE(className, type, instance, item);                                     \
    if (NULL == instance) {                                                             \
        (void) abq_status_set(EINVAL, false);                                           \
    } else {                                                                            \
        if(NULL == instance->internals) {                                               \
            instance->internals = ptree_create(globals_);                               \
            ptree_set_owner(instance->internals, (var_t)instance);                      \
        }                                                                               \
        rvalue = instance->internals;                                                   \
    }                                                                                   \
    return rvalue;                                                                      \
}                                                                                       \
static void TYPE_DELETE_NAME(className)(cvar_t old_item) {                              \
    type *instance = NULL;                                                              \
    CLASS_RESOLVE(className, type, instance, old_item);                                 \
    VITAL_NOT_NULL(instance);                                                           \
    (void) props_release_globals(old_item, globals_);                                   \
    (void) obj_release((cvar_t)instance->internals, old_item);                          \
    instance->internals = NULL;                                                         \
    delete_function_t nested_delete = delete_;                                          \
    if(NULL != nested_delete) {                                                         \
        (void) nested_delete(instance);                                                 \
    }                                                                                   \
}
/**
 * @brief creates a "deep-clone" of the given serializable instance where the copy is to use the given class for deserialization
 * recursively clones all high level objects (ptree_t, vlist_t, other serialiables) but not the JSON primitives (NULL, bool, number, string)
 *
 * @param serializable: a classified instance to clone properties from
 * @param class_of_item: the expected class of resulting clone
 * @return an instance with class of class_of item, or NULL on failure
 */
extern cvar_t build_class_instance(cvar_t serializabcle, class_ptr class_of_item);

static inline bool_t traverser_is_root(traverser_t *traverser) {
	return (1UL >= traverser->depth);
}

static inline abq_node_t* traverser_root(traverser_t *traverser) {
	return &traverser->stack[0];
}

static inline abq_node_t* traverser_leaf(traverser_t *traverser) {
	return (traverser_is_root(traverser)) ? traverser_root(traverser) : &traverser->stack[traverser->depth-1UL];
}

static inline abq_node_t* traverser_branch(traverser_t *traverser) {
	return (traverser_is_root(traverser)) ? NULL : &traverser->stack[traverser->depth-2UL];
}

static inline bool_t traverser_is_new_node(traverser_t *traverser) {
    bool_t retval = false;
	abq_node_t* node = traverser_leaf(traverser);
	switch(node->nodetype) {
	case (ABQ_NODE_VARLIST):
		retval = (0 > node->child_counter);
		break;
	case (ABQ_NODE_OBJECT):
		retval = (0 > node->child_counter);
		break;
	case (ABQ_NODE_COMPLETE):
		retval = true; /* non-iterable node, is considered beginning & end */
		break;
	default:
		VITAL_IS_OK(EINVAL);
		break;
	}
    return retval;
}

static inline bool_t traverser_do_break(traverser_t *traverser) {
    bool_t retval = false;
    abq_node_t* node = traverser_leaf(traverser);
    switch (node->nodetype) {
        case (ABQ_NODE_VARLIST):
            if(NULL != node->iter.nodelink) {
                node->iter.nodelink = NULL;
                retval = true;
            }
            break;
        case (ABQ_NODE_OBJECT):
            if(NULL != node->iter.nodeprop) {
                node->iter.nodeprop = NULL;
                retval = true;
            }
            break;
        case (ABQ_NODE_COMPLETE):
            break;
        case (ABQ_NODE_INVALID):
            break;
        default:
            VITAL_IS_OK(EINVAL);
            break;
    }
    return retval;
}

static inline bool_t traverser_is_endof_node(traverser_t *traverser) {
    bool_t retval = false;
    abq_node_t* node = traverser_leaf(traverser);
    switch (node->nodetype) {
        case (ABQ_NODE_VARLIST):
            retval = (0 <= node->child_counter) && (NULL == node->iter.nodelink);
            break;
        case (ABQ_NODE_OBJECT):
            retval = (0 <= node->child_counter) && (NULL == node->iter.nodeprop);
            break;
        case (ABQ_NODE_COMPLETE):
            retval = true; /* non-iterable node, is considered beginning & end */
            break;
        case (ABQ_NODE_INVALID):
            ABQ_VITAL(0UL == traverser->depth);
            retval = false; /* hack for iterating over root once if primitive */
            break;
        default:
            VITAL_IS_OK(EINVAL);
            break;
    }
    return retval;
}

static inline bool_t traverser_is_completed(traverser_t *traverser) {
    bool_t retval = true;
    if(&traverser_class == class_of(traverser)) {
        if((ABQ_NODE_COMPLETE != traverser_root(traverser)->nodetype)
                || (0UL != traverser->depth)) {
            retval = false;
        }
    }
    return retval;
}

#endif /* ONTRAC_ONTRAC_TRAVERSER_H_ */
