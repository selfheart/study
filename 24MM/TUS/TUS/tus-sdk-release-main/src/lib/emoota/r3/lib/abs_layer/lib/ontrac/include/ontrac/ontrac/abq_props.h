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
 * @file ontrac/abq_props.h
 * @date Apr 17, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief Used to define individual properties of an object and store them in a collection (property tree a.k.a. ptree_t)
 *
 * - A ptree_t (property tree )which is created with a NULL list of
 *    global properties can be used as a simple dictionary
 *    storing variable data in 'value' and mapping it to a 'name' key
 *
 * - By further providing a list of global properties, we can use the mapping
 *    to identify getters and setters for accessing fields within a struct.
 *    which should be of great assistance in the effort to (de)serialize
 *    specific kinds of data structures
 *
 * - Initially there will be 2 kinds of properties:
 *   -# "global" properties are created using the below
 *       DECLARE_GLOBAL_PROPERTY macro, these are defined
 *       once per property per -class-.
 *       They should never be modified in any way during runtime.
 *       And provide getters and setters for modified data within a structure
 *       mapped to the properties 'name' attribute
 *   -# "local" properties are created using property_create(...)
 *       and are created once per property per -instance-
 *
 */

#ifndef SPIL_NIO_LANG_ABQ_PROPS_H_
#define SPIL_NIO_LANG_ABQ_PROPS_H_

#include <ontrac/util/abq_llist.h>
#include <ontrac/unicode/utf8_utils.h>
/** typedef of property_t */
typedef struct for_property property_t;
/** typedef of ptree_t */
typedef struct for_ptree ptree_t;
/** The ptree_t definition */
struct for_ptree {
    /** Set of properties relating to an item*/
    LLIST_HEAD_DEF(property_t, props);
    /** total number of global+local properties in the ptree */
    size_t count;
    /** pointer to owner of the collected properties, initially the tree itself, DO NOT reserve a reference to the owner as the owner should own the property tree*/
    cvar_t owner;
    /** @todo property change listeners */
};

/**
 * @brief a classified pointer property getter, used to get the value of a property from a given instance
 *
 * @param item: the instance which holds the property value
 * @param prop: specifies which property we wish the get the value of
 * @return The property's associated value on the given instance
 */
extern cvar_t global_property_getter(cvar_t item, const property_t *prop);
/**
 * @brief a generic property setter, used to set the value of a property on a given instance
 *
 * @param item: the instance which holds the property value
 * @param prop: specifies which property we wish the set the value of
 * @param value: new value for the property on the given instance
 * @return 0 on success, else an error code specifying some insight into a reason for failure
 */
extern err_t global_property_setter(cvar_t item, property_t *prop, cvar_t value);

#include <ontrac/ontrac/abq_class.h>

/** class for ptree_t instances */
extern const class_t ptree_class;

/** An extended instance of a name-value pair with customizable getters and setters for the property value */
struct for_property {
    /** The property's key, Changed from 'key' to 'name' to enforce the idea that only string keys are supported, furthermore they are compared using a case-insensitive function */
    cstr_t name;
    /** The property's value
     *  - local properties: stores the value directly in each property,
     *  - global properties: stores (cvar_t) offsetof(type, field) Alternatively: &((parent_type*)NULL)->field
     **/
    cvar_t value;
    /** Used to specify a particular class for the value this property will work with */
    class_ptr class_of_value;
    /** internal linking mechanism restricted for use within the ptree_t (for locals) or linked together at compile time (globals)*/
    LLIST_LINK(property_t, next);
};

/**
 * @brief creates and initializes a new instance of ptree_t with a self reference
 *
 * @param globals: an optional LLIST of property_t structures of globally defined properties relating to an object.
 * @return pointer to a new instance of a ptree_t or NULL on error
 */
extern ptree_t *ptree_create(LLIST_HEAD_DEF(property_t, globals));
/**
 * @brief resolves an instance of ptree_class to the ptree_t*
 *
 * @param pointer to item to be resolved
 * @return resolved ptree_t* or NULL on failure
 */
extern ptree_t* ptree_resolve(cvar_t item);
/**
 * Sets the "owning" object of properties kept within this tree, used by serialiable classes
 * when set, the ptree_t replaces the reference it has on each property value to be a reference by the owning object instead
 * Also calls obj_takeover on the ptree_t instance with the owner as the reference
 * See DEFINE_SERIALIZABLE_CLASS for a usage example
 *
 * @param ptree: a pointer to the ptree_t to be owned
 * @param owner: an reference used to take ownership of values within child properties
 * @return 0 on success, else an error code
 */
extern void ptree_set_owner(ptree_t *ptree, cvar_t owner);
/**
 * @brief check if the ptree contains any -local- properties global properties will be included in a call to ptree_size but not in this method
 *
 * @param ptree: a pointer to the ptree_t to be examined
 * @return false if the pointer is valid and has at lease one local property, true otherwise
 */
extern bool_t ptree_is_empty(const ptree_t *ptree);
/**
 * @brief lookup the combined number of global and local properties included in this property tree
 *
 * @param ptree: a pointer to the ptree_t to be examined
 * @return: the combined number of global and local properties included in this property tree, -1 on error
 */
extern int32_t ptree_size(const ptree_t *ptree);
/**
 * @brief invokes the property setter of a property matching the given name with NULL item, first creating a local property if no matching property was found
 *  this function will kill the program if an attempt is made to put a global_property
 *  preferred way to set properties is to use 'property_set' with the item in question if you are working with global properties
 *  or if you have a reference to the nested property_t invoke the setter directly
 *  also uses 'ABQ_VITAL' to verify pointer to a valid ptree_t instance and no errors when setting values
 *
 * @param ptree: pointer to the ptree_t instance to be modified
 * @param name: name of the property to be modified
 * @param value: new value to which we wish to set the property to
 * @return EXIT_SUCCESS if new leaf was created
 * @return EALREADY if existing property was updated
 * @return an error code on failure
 */
extern err_t ptree_put(ptree_t *ptree, cstr_t name, cvar_t value);
/**
 * @brief creates a new local property for the given name with the given value if and only if an existing value wasn't found
 *
 * @param ptreepointer to the ptree_t instance to be modified
 * @param name: name of the new property to be created
 * @param value: new value to which we wish to set the property to
 * @return EEXIST if exists, 0 on success, else a different error code
 */
extern err_t ptree_put_if_absent(ptree_t *ptree, const cstr_t name, cvar_t value);
/**
 * @brief gets the value of a local property within the referenced ptree_t,
 *  will attempt to dereference a NULL pointer if an attempt is made to get a global property
 *  should use 'property_set' or access the property_t directly when working with globals
 *
 * @param ptree: pointer to the ptree_t instance to be modified
 * @param name: name of the property to be retrieved
 * @return the current value of the named property, or NULL if not found
 */
extern cvar_t ptree_get(const ptree_t *ptree, const cstr_t name);
/**
 * @brief removes a local property within the referenced ptree_t, but first adding ref as an anchor
 *
 * @param ptree: pointer to the ptree_t instance to be modified
 * @param name: name of the property to be removed
 * @param ref: new anchor to be used to reference child, NULL for self-reference
 * @return the value of the removed property if successful, NULL if not found
 */
extern cvar_t ptree_take(ptree_t *ptree, const cstr_t name, cvar_t ref);
/**
 * @brief removes a local property within the referenced ptree_t,
 *
 * @param ptree: pointer to the ptree_t instance to be modified
 * @param name: name of the property to be removed
 * @param ref: new anchor to be used to reference child, NULL for self-reference
 * @return the value of the removed property if successful, NULL if not found
 */
extern err_t ptree_detach(ptree_t *ptree, const cstr_t name);
/**
 * Removes all local properties from the given ptree
 *
 * @param ptree: pointer to the ptree_t instance to be modified
 * @return 0 on success, or an error code if passed a bad reference
 */
extern err_t ptree_clear(ptree_t *ptree);
/**
 * @brief checks if the ptree has a property of the given name
 *
 * @param ptree: pointer to the ptree_t instance to be inspected
 * @param name:  name of the property to be searched for
 * @return true if a property matching the given name was found
 */
extern bool_t ptree_has_key(const ptree_t *ptree, const cstr_t name);
/**
 * @brief checks if the ptree has a property of the given value
 *
 * @param ptree: pointer to the ptree_t instance to be inspected
 * @param value: value of the property to be searched for
 * @param value_compare: optional compare_function_t used to compare values
 * @return true if a property matching the given value was found
 */
extern bool_t ptree_has_value(const ptree_t *ptree, cvar_t cvalue, compare_function_t value_compare);
/**
 * @brief clones the property tree
 * @todo move to traverser and implement deep_clone
 *
 * @param target: pointer to a ptree_t to be cloned
 * @param deep_clone: should it clone the propertied of child values
 * @return a copy of the referenced ptree_t on success, NULL otherwise
 */
extern ptree_t *ptree_clone_properties(cvar_t target, bool_t deep_clone);
/**
 * @brief copies each property from source onto dest
 *   does not remove properties contained in dest but not found in source
 *
 * @param source: source target from which to get properties
 * @param dest: dest target onto which we will set properties
 * @return 0 on success, else an error code
 */
extern err_t transfer_properties(cvar_t source, var_t dest);

extern bool_t item_has_property(cvar_t item, class_ptr meta, const property_t *property);
/**
 * @brief looks up the property_t for the named property and returns a pointer,
 *  if property_t with given name is not found, attempts to create a local property with a NULL value
 *
 * @param item: the item to which the named property_t belongs
 * @param item_meta: optional, the class of the item, if NULL it will be loaded via class_of(item)
 * @param name: the name of the property_t we wish to load
 * @return pointer to a property_t associated with item and name, or NULL if unable
 */
extern property_t *property_by_name(cvar_t item, class_ptr item_meta, const cstr_t name);
extern class_ptr property_get_class(const property_t *prop);
/**
 * @brief lookup property with matching name on item and get it'd current value
 *
 * @param item: classified item with a set if properties
 * @param name: identifier used to match a given property
 * @return value of given property if found, else NULL
 */
extern cvar_t property_get(cvar_t item, const cstr_t name);
extern cvar_t property_get_value(const property_t *prop, cvar_t item);
/**
 * @brief looks up the property_t for the named property and invokes it's setter function
 *
 * @param item: the item to which the named property belongs
 * @param item_meta: optional, the class of the item, if NULL it will be loaded via class_of(item)
 * @param name: the name of the property we wish to get the value of
 * @param value: new value for the property
 * @return 0 on success, else an error code which depends on what went wrong
 */
extern err_t property_set(cvar_t item, class_ptr item_meta, const cstr_t name, cvar_t value);
extern err_t property_set_value(property_t *prop, cvar_t item, cvar_t value);

/**
 * @brief sets global properties listed on item to NULL
 *
 * @param item: an instance to which the global properties apply
 * @param LLIST_HEAD_DEF(property_t, globals): list of global properties to clear
 * @return 0 on success, else an error code
 */
extern err_t props_release_globals(cvar_t item, LLIST_HEAD_DEF(property_t, globals));

/**
 * @brief iterates to the next property in the referenced ptree based on the previous property
 *
 * @param ptree: pointer to a ptree_t which we are iterating through
 * @param prev: the prior property which we last iterated to, NULL to start from beginning
 * @return a pointer to the next property on the tree, or NULL when complete
 */
static inline property_t* ptree_iterate(const ptree_t *ptree, const property_t *prev){
    VITAL_NOT_NULL(ptree);
    return (NULL == prev) ? LLIST_FIRST(ptree->props) : LLIST_NEXT(prev, next);


/** loop over all of the properties within the tree, starting with globals */
#define PTREE_LOOP(ptree, name_var, value_type, value_var) \
    for(property_t *prop = LLIST_FIRST(ptree->props); \
        NULL != prop \
        && ((name_var = prop->name) || true) \
        && ((value_var = (value_type) property_get_value(prop, ptree->owner)) || true); \
        prop = LLIST_NEXT(prop, next))
}

/** defines a globally scoped property, linking it to an optional previous property */
#define DEFINE_GLOBAL_PROPERTY(uniqueName, propertyName, propertyValue, propertyClass, lastProperty) \
static property_t uniqueName = {                                           \
        .name = #propertyName,                                          \
        .value = (cvar_t) (propertyValue),                              \
        .next = lastProperty,                                           \
        .class_of_value = propertyClass,                                \
}

/** Defines a default property getter and setter for a global property then defines the property itself */
#define DEFINE_DEFAULT_GLOBAL_PROPERTY(uniqueName, parentType,      \
        propertyName, propertyType, propertyClass, lastProperty)    \
DEFINE_GLOBAL_PROPERTY(uniqueName, propertyName,  &(((parentType*)NULL)->propertyName), propertyClass, lastProperty)

#endif /* SPIL_NIO_LANG_ABQ_PROPS_H_ */
