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
 * @file ontrac/abq_str.h
 * @date Mar 26, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief string instances, currently restricted to utf8 encoded strings
 *
 */

#ifndef SDM_UTIL_ABQ_STR_H
#define SDM_UTIL_ABQ_STR_H

#include <ontrac/unicode/utf8_utils.h>
#include <ontrac/ontrac/abq_class.h>
/** class_t for classified instances for str_t (string constants are not classified with a class) */
extern const class_t string_class;
/** a 'list_of_*' class used for specifying property types using for serialization */
extern const class_t list_of_string_class;

/**
 * @brief create a null terminated str_t with a class of string_class
 *
 * @param source: source data which we will copy onto then new string
 * @param n: maximum number of bytes to read from source, -1 will continue reading until terminator
 * @param trim: should leading & tailing whitespace be included in the new string? true for no, false for yes
 * @return a str_t referencing the newly created string
 */
extern cstr_t str_create(cstr_t source, int32_t n, bool_t trim);
/**
 * @brief creates a '\0' terminated str_t with a string_class
 *
 * @param decoder: A source of data to read the string from
 * @return a str_t referencing the newly created string
 */
extern cstr_t abq_decode_str(abq_decoder_t *decoder);
/**
 * @brief resolves an instance of string_class to the str_t
 *
 * @param pointer to item to be resolved
 * @return resolved str_t or NULL on failure
 */
extern cstr_t str_resolve(cvar_t item);
/**
 * @brief coerce an item to a str_t instance with a given anchor (ref)
 *
 * @param source: Source item which needs to be coerced into a str_t instance, unclassified items will be assumed to be a compiled cstr_t
 * @param ref: Some reference identifier to the object (often a pointer to parent object)
 * @return An instance of str_t representing the source data, or NULL on failure to coerce
 */
extern cstr_t str_coerce(cvar_t source, cvar_t ref);
/**
 * @brief lookup of matching string in the string_pool
 *
 * @param source: match data to lookup in the internal pool of strings
 * @return pointer to the string in the pool if found, else NULL
 */
extern cstr_t str_lookup(cstr_t source);
/**
 * @brief Creates a clone of the given string with lower-case lettering
 * this class will always create a new instance of the string with a self reference
 * even if the original string was already lower-case
 *
 * @param source: the source string we wish to convert to lower case
 * @param n: maximum number of characters to include in the lower case return value
 * @return an classified instance of str_t with lower case characters if successful, otherwise NULL
 */
extern cstr_t str_to_lower(const cstr_t source, int32_t n);
#include <ontrac/ontrac/var_list.h>
/**
 * @brief splits string into segments that where joined with separator
 *
 * @param source: the source string which we which to split into segments
 * @param separator: the delimiting sequence of characters which serve as boundaries between instances
 * @param keep_empty_strings: true if one wishes to keep empty strings between delimiters
 * @return a newly self-referencing vlist owning all of the newly allocated string segments
 */
extern vlist_t *str_split(cstr_t source, const cstr_t separator,
        bool_t trim_strings, bool_t keep_empty_strings);
/**
 * @brief joins strings listed in str_list into a singular string coupled by separator
 *
 * @param str_list: a list of string types to join together into a larger string
 * @param separator:  the delimiting sequence of characters which serve as boundaries between concatenations
 * @param release_list_when_done: true if we should call obj_release_self(str_vlist) when done
 * @return a singular string joining a separator between each the strings in str_list
 */
extern cstr_t str_join(vlist_t *str_list, const cstr_t separator, bool_t release_list_when_done);
extern cstr_t str_format_int(int64_t value);
extern cstr_t str_format_oct(uint64_t value);

#include <ontrac/ontrac/abq_props.h>
/** defines a default global property getter used to access the field of a struct */
#define DEFINE_MAPPED_GETTER(uniqueName, parentType, propertyName,          \
                                listOfMappings, enumField, stringField)     \
static cvar_t ABQ_GETTER_NAME(uniqueName)(cvar_t item, const property_t *prop) { \
    cvar_t rvalue = NULL;                                           \
    VITAL_NOT_NULL(item);                                           \
    VITAL_NOT_NULL(prop);                                           \
    parentType *parent = NULL;                                      \
    (void) bytes_copy(&parent, &item, sizeof(cvar_t));              \
    for(int32_t i=0; NULL!=listOfMappings[i].stringField; i++){     \
        if(listOfMappings[i].enumField==parent->propertyName) {     \
            rvalue = (cvar_t) listOfMappings[i].stringField;        \
            break;                                                  \
        }                                                           \
    }                                                               \
    return rvalue;                                                  \
}

/** defines a default global property setter used to access the field of a struct */
#define DEFINE_MAPPED_SETTER(uniqueName, parentType, propertyName, \
                                listOfMappings, enumField, stringField) \
static err_t ABQ_SETTER_NAME(uniqueName)(cvar_t item,                  \
                        property_t *prop, cvar_t value) {           \
    VITAL_NOT_NULL(item);                                           \
    VITAL_NOT_NULL(prop);                                           \
    err_t retval = EXIT_SUCCESS;                                    \
    parentType *parent = NULL;                                      \
    (void) bytes_copy(&parent, &item, sizeof(cvar_t));              \
    int32_t i=0;                                                    \
    cstr_t input = ptr2cstr(value);                                 \
    for(i=0; NULL != listOfMappings[i].stringField; i++) {          \
        cstr_t key = listOfMappings[i].stringField;                 \
        if(0 == utf8_compare_insensitive(input, key, -1)) {         \
            parent->propertyName = listOfMappings[i].enumField;     \
            break;                                                  \
        }                                                           \
    }                                                               \
    if(NULL == listOfMappings[i].stringField) {                     \
        parent->propertyName = listOfMappings[i].enumField;         \
        if(NULL != input) {ABQ_ERROR_MSG_Z(prop->name, input);}     \
    }                                                               \
    return retval;                                                  \
}

/** defines a enum to string global property mapping based on a list of mappings */
#define DEFINE_MAPPED_GLOBAL_PROPERTY(uniqueName, parentType, propertyName, \
              listOfMappings, enumField, stringField, lastProperty)         \
DEFINE_MAPPED_GETTER(uniqueName, parentType, propertyName,                  \
                        listOfMappings, enumField, stringField);            \
DEFINE_MAPPED_SETTER(uniqueName, parentType, propertyName,                  \
                  listOfMappings, enumField, stringField);                  \
EMPTY_CLASS(uniqueName ##_cls, listOfMappings, NULL, NULL, NULL, NULL,      \
        ABQ_GETTER_NAME(uniqueName), ABQ_SETTER_NAME(uniqueName),           \
        invalid_delete, &string_class, static);                             \
DEFINE_GLOBAL_PROPERTY(uniqueName, propertyName,                            \
        &(((parentType*)NULL)->propertyName), &(uniqueName ##_cls), lastProperty)

#endif /* SDM_UTIL_ABQ_STR_H */
