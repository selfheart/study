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
 * @file ontrac/abq_class.h
 * @date Mar 27, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief a lean attempt at a MISRA compliant object oriented programming in c
 *
 * - A struct type can be passed into the DEFINE_CLASS macro\n
 *   along with a maximum number allocated instances and optional utility functions
 *
 * - Then one can use the class_t defined within the macro to allocate instances of these structs\n
 *   each of these allocated instances come with an *invisible(1) pointer to the class_t\n
 *   along with a list of references to the instance. (new instances initially has a reference to self)\n
 *   *(By invisible, it adds an additional obj_t struct included in the block size of the mem_usage_t)\n
 *   .
 *
 * - Instance of class Life-Cycle:
 *  -# Each instance has a list of references
 *  -# New instance are allocated with 1 self reference
 *  -# New references can be assigned via 'obj_reserve' or 'obj_reserve_self'
 *  -# Old references can be removed via 'obj_release' or 'obj_release_self'
 *  -# When all references are removed from an object it is deleted.
 *
 * - Utility functions that can optionally be included with the class itself: \n
 *    Each class_t may or may not implement the following utility functions
 *  -# create_function_t create: create a new instance of the object, needed for deserialization
 *  -# coerce_function_t coerce: attempt to coerce from one class of data to another
 *  -# compare_function_t compare: compares two instances of same class so that they can be equated and sorted
 *  -# properties_of_t properties_of: return a collection of properties for the given object, needed for (de)serialization
 *  -# delete_function_t on_delete: Allows an object to release references to children when it is garbage collected
 */

#ifndef SPIL_NIO_ABQ_CLASS_H
#define SPIL_NIO_ABQ_CLASS_H

#include <ontrac/ontrac/mem_usage.h>
#include <ontrac/util/abq_alist.h>
#include <ontrac/util/abq_item_pool.h>

/** typedef of class_t */
typedef struct for_class class_t;
/** typedef of class_ptr */
typedef const class_t *class_ptr;
/** typedef of obj_t */
typedef struct for_obj obj_t;

/** class_t for instances of class_t */
extern const class_t class_class;

/**
 * @brief used for creating new instances of a misc. class
 * Needed for deserialization of nested objects within the hierarchy
 *
 * @param meta The class to create an instance from
 * @return An initialized new instance of the class
 */
typedef cvar_t (*create_function_t)(class_ptr meta, cvar_t ref);
/**
 * @brief attempts to coerces data from one class_t to into an instance of the owning class
 *   Also called at completion of deserialization so that the function could be used to validate and internalize the data
 *
 * @param source: source data on which coercion is going to be attempted
 * @param ref: if coercion is successful, increment obj_refenece using ref, NULL for a self reference
 * @return NULL on failre or a coerced instance of the given data on which a call to 'class_of' will return the parent class.
 */
typedef cvar_t (*coerce_function_t)(cvar_t source, cvar_t ref);

#include <ontrac/ontrac/abq_props.h>
#include <ontrac/ontrac/var_stack.h>

/**
 * @brief load a collection of property accessors for a given instance
 * intended for use in object (de)serialization
 *
 * @param item: the instance to load properties for
 * @return a property tree pointer to a collection of properties for the given instance
 */
typedef ptree_t* (*properties_of_t)(cvar_t item);
/**
 * @brief a generic property getter, used to get the value of a property from a given instance
 *
 * @param item: the instance which holds the property value
 * @param prop: specifies which property we wish the get the value of
 * @return The property's associated value on the given instance
 */
typedef cvar_t (*field_getter_t)(cvar_t item, const property_t *prop);
/**
 * @brief a generic property setter, used to set the value of a property on a given instance
 *
 * @param item: the instance which holds the property value
 * @param prop: specifies which property we wish the set the value of
 * @param value: new value for the property on the given instance
 * @return 0 on success, else an error code specifying some insight into a reason for failure
 */
typedef err_t (*field_setter_t)(cvar_t item, property_t *prop, cvar_t value);
/**
 * @brief similar to Java's 'finalize' function, useful for releasing child objects
 * creating new objects while in a delete method will call abq_fatal to prevent recursion
 *
 * @param old_item: the instance of the class which is being garbage collected
 * @return an err_t i.e. 0 on success
 */
typedef void (*delete_function_t)(cvar_t old_item);

/** --private-- object meta-data, used to tie each instance to it's class and list of reference */
struct for_obj {
    /** the object's class */
    class_ptr meta;
#if !defined(NDEBUG)
    /** stack of active references (anchors) */
    cvar_t *refs;
    uint16_t refs_len;
    uint16_t refs_max;
#else /* NDEBUG */
    uint16_t ref_count;
#endif /* NDEBUG */
    /** pointer to the instance of the struct type usage in class definition */
    cvar_t member;
    /** --private-- internal linking used to link all obj_t instances together in a obj_hash_map */
    LLIST_LINK(obj_t, next);
};

/** struct for class_t definition  */
struct for_class {
    /** Always points to &class_class for valid class_t objects */
    class_ptr magic;
    /** Name of a given class (e.i. "class_class" for class of class_t) */
    str_t class_name;
    /** A mem_usage_t reserved for instances of this given class */
    mem_usage_t *mem_usage;
    /** optional instance creation method */
    create_function_t create;
    /** optional instance coercion function */
    coerce_function_t coerce;
    /** optional instance comparison method */
    compare_function_t compare;
    /** optional instance property set loader */
    properties_of_t properties_of;
    /** optional property getter function for fields of the given class */
    field_getter_t getter;
    /** optional property setter function for fields of the given class*/
    field_setter_t setter;
    /** optional instance delete method, if set should only be invoked by the garbage collector */
    delete_function_t on_delete;
    /** a pointer to related vlist_class for this kind of item, or a reverse reference back to the type of items for vlist_class */
    class_ptr class_ref;
};
/**
 * @brief compares addresses of left and right pointers
 *
 * @param left the first pointer to compare
 * @param right the second pointer to compare
 * @return the value 0 if left pointer has equal address to right pointer
 *          a value less then 0 if left pointer has higher address then right pointer
 *          a value greater then 0 if left pointer has smaller address then right pointer
 */
extern int8_t identity_compare(cvar_t left, cvar_t right);
/**
 * @brief intended for the comparison of comparable items, first sorts them by class_t, then by individual class of item's compare method if any
 *
 * @param left: the left instance to be compared
 * @param right: the right instance to be compared
 * @return the value 0 if left instance has equal value to right instance
 *          a value less then 0 if left instance has higher value then right instance
 *          a value greater then 0 if left instance has smaller value then right instance
 */
extern int8_t var_compare(cvar_t left, cvar_t right);
/**
 * @brief Look up the class of an instance, will kill program with abq_fatal if the object is being deleted
 *
 * @param item: the instance of which we wish to know class information about
 * @return a pointer to the class_t of the object, or NULL if it has no class
 */
extern class_ptr class_of(cvar_t item);
/**
 * Check that an instance is of a given class
 *
 * @param meta: pointer to the expected class of instance
 * @param item: instance to check class of
 * @return 0 if class is a match, EINVAL if not a match, or EFAULT if meta is NULL
 */
extern err_t class_check(class_ptr meta, cvar_t item);
/**
 * @brief adds the given reference to list of references for the given instance
 *
 * @param item: the instance to which we are reserving a reference
 * @param ref: Some reference identifier to the object (often a pointer to parent object)
 * @return 0 on success or non-zero on error (EINVAL if instance has no class)
 */
extern err_t obj_reserve(cvar_t item, cvar_t ref);
/**
 * @brief adds a self reference to list of references for the given instance
 *
 * @param item: the instance to which we are referencing
 * @return 0 on success or non-zero on error (EINVAL if instance has no class)
 */
static inline err_t obj_reserve_self(cvar_t item) {
    return obj_reserve(item, item);
}
/**
 * @brief removes the given reference from list of references for the given instance, and deletes it when all references are removed
 *
 * @param item: the instance to which we are releasing the reference
 * @param ref: Some reference identifier to the object (often a pointer to parent object)
 * @return 0 on success or non-zero on error (EINVAL if instance has no class, or ENOLINK if reference was not found)
 */
extern err_t obj_release(cvar_t item, cvar_t ref);
/**
 * @brief removes a self reference from list of references for the given instance, and deletes it when all references are removed
 *
 * @param item: the instance to which we are releasing the self-reference
 * @return 0 on success or non-zero on error (EINVAL if instance has no class, or ENOLINK if reference was not found)
 */
static inline err_t obj_release_self(cvar_t item) {
    return obj_release(item, item);
}

/**
 * @brief quickly changes ownership of newly create created instance from a self reference to the ref parameter
 *
 * @param item: a newly created instance of a class that only has the default self reference
 * @param ref: a reference to which to change the instance's reference to
 * @return 0 on success, most errors will cause fatal results to train the programmer on it's limited usage
 */
extern err_t obj_takeover(cvar_t item, cvar_t ref);
/**
 * @brief 'class'ifies a potentially class-less variable with the given class type, then references it with object
 * if meta->coerce is not NULL, will call that method ONLY if item is not already classified
 * then checks if the variable is already of the given class, and reserves it if it is
 * the checks if the variable has no associated class, then creates a new obj_t used associate the obj_t with that class
 * at this point if the object has a different class associated with it, sets abq_status to EINVAL and returns NULL
 *
 * @param item: the item to associate with a given class
 * @param meta: the class to associate with a given item
 * @param ref: Some reference identifier to the object (often a pointer to parent object)
 * @return an item classified with meta, or NULL on failure with abq_status set
 */
extern cvar_t classify(cvar_t item, class_ptr meta, cvar_t ref);
/**
 * @brief just like 'classify' except it skips the call to meta->coerce so that it can be called from within coercion methods
 *
 * @param item: the item to associate with a given class
 * @param meta: the class to associate with a given item
 * @param ref: Some reference identifier to the object (often a pointer to parent object)
 * @return an item classified with meta, or NULL on failure with abq_status set
 */
extern cvar_t item_of_class(cvar_t item, class_ptr meta, cvar_t ref);
/**
 * @brief just like 'classify' except it ALWAYS calls meta->coerce  (if available)
 *
 * @param item: the item to associate with a given class
 * @param meta: the class to associate with a given item
 * @param ref: Some reference identifier to the object (often a pointer to parent object)
 * @return an item classified with meta, or NULL on failure with abq_status set
 */
extern cvar_t abq_coerce(cvar_t item, class_ptr meta, cvar_t ref);
/**
 * @brief lookup the obj_t container of a classified obj
 * only anticipated use outside of abq_class.c is for coercion, or when we need to remove a 'const' qualifier
 *
 * @param item: the item to lookup
 * @return pointer to related obj_t, or NULL if not found
 */
extern obj_t *obj_lookup(cvar_t item);
/**
 * @brief convenience method to 1) overwrite a pointer field, 2) obj_reserve on reference to new value, 3) obj_release reference old value
 *
 * @param item: an structure which contains a pointer field
 * @param field: a pointer field within the item's structure
 * @param new_value a pointer value to which the field should be set after the call
 */
extern void item_field_set(cvar_t item,
        cvar_t* field, cvar_t new_value);
/**
 * @brief convenience method to 1) overwrite a pointer field, 2) obj_takeover on reference to new value, 3) obj_release reference old value
 *
 * @param item: an structure which contains a pointer field
 * @param field: a pointer field within the item's structure
 * @param new_value a pointer value to which the field should be set after the call
 */
extern void item_field_take(cvar_t item,
        cvar_t* field, cvar_t new_value);

/**
 * @deprecated
 * @brief does nothing
 * @return EXIT_SUCCESS
 */
#define run_garbage_collection() EXIT_SUCCESS
/**
 *  @brief allocates a item tracked with a obj_t given meta member and initial ref
 *
 * @param meta: type of item to be created
 * @param ref: initial reference used to stop the instance from being deleted
 * @return: pointer to new instance data, or NULL on error
 */
extern cvar_t class_new_instance(class_ptr meta, cvar_t ref);
/**
 * @brief for creating "Flexible" sized instance of a class, also see NEW_FLEX_INSTANCE macro below
 *
 * @param meta: pointer to a class_t which we wish to create an instance of
 * @param size: size of the instance's struct type (does not include the obj_t)
 * @return a new instance of the given class, or NULL on failure
 */
extern byte_t* new_instance_flex(class_ptr meta, size_t size);
/**
 * @brief for creating a vlist_t for items of a particular kind of class
 *
 * @param vlist_meta: class_t pointer defined with DEFINE_LIST_OF_CLASS macro
 * @return pointer to a new vlist_t instance, or NULL on failure
 */
extern cvar_t list_of_class_create(class_ptr vlist_meta, cvar_t ref);
/**
 * @brief for classes which shouldn't be used to track dynamic instances, fatal if called
 *
 * @param old_item: the item that is being garbage collected
 */
extern void invalid_delete(cvar_t old_item);

/** class_t for vlist_t instances (also declared in var_list.h) */
extern const class_t vlist_class;
static inline bool_t class_is_list(class_ptr meta) {
    bool_t retval = false;
    if (NULL != meta) {
        if(&vlist_class == meta) {
            retval = true;
        }else if(list_of_class_create == meta->create) {
            retval = true;
        }else{
            // vstacks ?
            retval = false;
        }
    }
    return retval;
}
static inline class_ptr class_realize(class_ptr meta) {
	class_ptr retval = meta;
	if (NULL == retval) {
	    // Return NULL as is
	} else if(list_of_class_create == retval->create){
        // if meta is a list_of_* class, switch to vlist_class
    	retval = &vlist_class;
    } else if((invalid_delete == retval->on_delete)
            && (NULL != retval->class_ref)){
        retval = retval->class_ref;
    } else {
        // retval is already a "real" class, leave as is
    }
	return retval;
}

/**
 * Checks a given class to check if it is JSON "primitive", either a NULL, boolean, number or string
 *
 * @param meta: pointer to a class_t to check if it is serializable or not
 * @return true if the class_t is one of number_class, string_class, bool_class, and null_class
 * @return false if the given meta does not match a primitive type
 */
extern bool_t class_is_primitive(class_ptr meta);
/**
 * @brief sets item pointer to by item_ptr to variable if and only if variable is classified with the given meta
 *
 * @param meta: class_t used to track items if a given type
 * @param item_ptr: points to a field of the correct type for associated class
 * @param variable: the item to check for classification
 */
extern void class_resolve_item(class_ptr meta,
        cvar_t *item_ptr, cvar_t variable);
/**
 * Checks a given class to check if it is "serializable" which requires it to have a create method and a properties_of method
 *
 * @param meta: pointer to a class_t to check if it is serializable or not
 * @return true if the class_t has a function defined for 'create' and either 'properties_of' function or a "list"
 * @return false for primitive classes such as number_class, string_class, bool_class, and null_class despite being serializable
 */
static inline bool_t class_is_serializable(class_ptr meta) {
    bool_t retval = false;
    if((NULL != meta) && (NULL != meta->create)){
        if (NULL != meta->properties_of) {
            retval = true;
        } else {
            retval = class_is_list(meta);
        }
    }
    return retval;
}


/** names the property getter for a property of a global scope */
#define ABQ_GETTER_NAME(uniqueName) uniqueName ## _get
/** names the property setter for a property of a global scope */
#define ABQ_SETTER_NAME(uniqueName) uniqueName ## _set

/** macro used to load the mem_usage variable name for a given class name */
#define MEM_USAGE_FOR_CLASS(className) className ## _mu
#define OBJ_AND_TYPE(type_) obj_ ## type_

/** macro used to define a class */
#define DEFINE_CLASS(name, type, max_n, unused_, coerce_, compare_, propertiesOf_, onDelete_, ...) \
typedef struct {obj_t obj; type item;} obj_ ## type;                                          \
STATIC_ITEM_POOL(obj_ ## type, max_n, name ## _mu);                                           \
static inline type* type ## _create(class_ptr meta, cvar_t ref) {                             \
    type* retval = NULL;                                                                      \
    cvar_t instance = class_new_instance(meta, ref);                                          \
    bytes_copy(&retval, &instance, sizeof(cvar_t));                                           \
    return retval;                                                                            \
}                                                                                             \
__VA_ARGS__ const class_t name = {                                                            \
    .magic = &class_class,                                                                    \
    .class_name = #name,                                                                      \
    .mem_usage = &MEM_USAGE_FOR_CLASS(name),                                                  \
    .create = class_new_instance,                                                             \
    .coerce = coerce_,                                                                        \
    .compare = compare_,                                                                      \
    .properties_of = propertiesOf_,                                                           \
    .getter = global_property_getter,                                                         \
    .setter = global_property_setter,                                                         \
    .on_delete = onDelete_,                                                                   \
    .class_ref = &name                                                                        \
};

static inline cstr_t CLASS_NAME(class_ptr meta) {
    return (NULL == meta) ? abq_null_str : meta->class_name;
}

#define CLASS_RESOLVE(className, type_, item_, var_) class_resolve_item(&className, ptr2ptrptr(&(item_)), (cvar_t)(var_))

#define EMPTY_CLASS(name, type, unused_, coerce_, compare_, propertiesOf_, getter_, setter_, onDelete_, _ref_class, ...)         \
__VA_ARGS__ const class_t name = {                      \
    .magic = &class_class,                              \
    .class_name = #name,                                \
    .mem_usage = NULL,                                  \
    .create = NULL,                                     \
    .coerce = coerce_,                                  \
    .compare = compare_,                                \
    .properties_of = propertiesOf_,                     \
    .getter = getter_,                                  \
    .setter = setter_,                                  \
    .on_delete = onDelete_,                             \
    .class_ref = _ref_class                             \
}

#define DEFINE_LIST_OF_CLASS(listOfClassName, className, ...)       \
__VA_ARGS__ const class_t listOfClassName = {                       \
    .magic = &class_class,                                          \
    .class_name = #listOfClassName,                                 \
    .mem_usage = NULL,                                              \
    .create = list_of_class_create,                                 \
    .coerce = NULL,                                                 \
    .compare = var_compare,                                         \
    .properties_of = NULL,                                          \
    .getter = global_property_getter,                               \
    .setter = global_property_setter,                               \
    .on_delete = invalid_delete,                                    \
    .class_ref = &className                                         \
}

/** A macro for creating an instance of the class and return the allocated memory */
#define CREATE_BASE_INSTANCE(className, type) type ## _create(&className, NULL)
/** Used for creating "Flexible" sized instance of a class such as StringType, uses default memory instead of it's own mem_usage*/
#define NEW_FLEX_INSTANCE(className, typePtr, size) (typePtr) new_instance_flex(&className, size)

static inline cvar_t* ptr2ptrptr(cvar_t address) {
#if !defined(NDEBUG)
    // Assert alignment is OK
    ABQ_VITAL(0U == ABQ_ALIGN_LAST_OFFSET(address, cvar_t));
#endif /* DEBUG */
    return (cvar_t*) address;
}

static inline size_t addr_offset(cvar_t item, cvar_t field_ptr) {
    uintptr_t item_addr = (uintptr_t) item;
    uintptr_t field_addr = (uintptr_t) field_ptr;
#if !defined(NDEBUG)
    // Assert alignment is OK
    ABQ_VITAL(item_addr <= field_addr);
#endif /* DEBUG */
    return (size_t) (field_addr - item_addr);
}

static inline cvar_t* item_get_field(cvar_t item, size_t offset) {
    VITAL_NOT_NULL(item);
    cbyte_t* octets = ptr2cstr(item);
    return ptr2ptrptr(&octets[offset]);
}

/**
 * @Similar to the offsetof macro defined in <stddef.h> but takes a instance instead of the struct type & it computes @ runtime
 *
 * @param item: instance of a type which contains the given field
 * @param field: name of a field on the given instance
 * @return size_t representing offset of field within given item
 */
#define FIELD_OFFSET(item, field) addr_offset((cvar_t)(item), (cvar_t)&(item)->field)

#define SET_RESERVED_FIELD(item, field, value) item_field_set((cvar_t)item, ptr2ptrptr(&(item)->field), (cvar_t)value)

#define TAKE_RESERVED_FIELD(item, field, value) item_field_take((cvar_t)item, ptr2ptrptr(&(item)->field), (cvar_t)value)

#if !defined(NDEBUG)
extern bool_t objs_dump_refs(const mem_usage_t* mem_usage );
extern bool_t ref_dump_children(cvar_t ref);
extern cvar_t item_to_debug;
extern ALIST_HEAD_DEF(class_ptr) locked_classes;

#define LOCK_CLASS(className) do {                              \
    mem_usage_lock_mutex((className).mem_usage);                \
    if(!ALIST_IS_FULL(&locked_classes)) {                       \
        ALIST_PUSH(&locked_classes, class_ptr, &(className));   \
    }                                                           \
}while(0);

#define UNLOCK_CLASS(className) do{                                 \
    int32_t index = -1;                                             \
    class_ptr match = NULL;                                         \
    ALIST_FIND(&locked_classes, class_ptr, match, index,            \
            (match == &(className)));                               \
    if( (0 <= index) && (index < ALIST_SIZE(&locked_classes))) {    \
        ALIST_REMOVE_AT_QUICK(&locked_classes, class_ptr, index);   \
    }                                                               \
    mem_usage_unlock_mutex(className.mem_usage);                    \
}while(0);
#else /* NDEBUG */
#define LOCK_CLASS(className) mem_usage_lock_mutex(className.mem_usage);
#define UNLOCK_CLASS(className) mem_usage_unlock_mutex(className.mem_usage);
#endif /* NDEBUG */

// bitshift(s) -> hash capacity
// 1 -> 2
// 3 -> 8
// 5 -> 32
// 7 -> 128
#define CLASS_HASH_CAPACITY (256U)
/**
 * @brief generate a distributed hash value within the range of 0 to (CLASS_HASH_CAPACITY-1)
 *
 * @param ptr: address used to generate the hash value
 * @return a hash value ~evenly distributed between 0 and (CLASS_HASH_CAPACITY-1)
 */
extern size_t obj_address_hash(const void *ptr);
/**
 * @brief runs garbage collection, then checks all bins in the obj_hash_map are empty, exposes each obj_t that has not been freed
 */
extern void check_objs_and_mem_is_free(void);
/**
 * @brief invoke class->delete(item) on all classified items prior to releasing ontrac resources "clean" restart
 */
extern void abq_free_all_objs_and_mem(void);

#endif //SPIL_NIO_ABQ_CLASS_H
