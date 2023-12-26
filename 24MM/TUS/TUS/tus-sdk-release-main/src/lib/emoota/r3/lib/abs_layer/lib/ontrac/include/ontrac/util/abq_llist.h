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
 * @file ontrac/abq_llist.h
 * @date Mar 10, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief Linked-List macros for working with a linked set of structs
 *
 */

#ifndef HELLO_C_ABQ_LLIST_H
#define HELLO_C_ABQ_LLIST_H


//#ifndef VITAL_IS_NULL
//#include <assert.h>
//#define VITAL_IS_NULL(expr) assert(NULL == expr)
//#endif /* VITAL_IS_NULL */

/** Type of a head of linked struct of given type */
#define LLIST_HEAD_TYPE(type) type*

/** Defines the head of a linked list of the given type to the given name */
#define LLIST_HEAD_DEF(type, name) LLIST_HEAD_TYPE(type) name

/** Initialized the head of linked list */
#define LLIST_HEAD_INIT() NULL

/** Defines and initializes a list of the given type to the given name */
#define LLIST_HEAD(type, name) LLIST_HEAD_DEF(type, name) = LLIST_HEAD_INIT();

/** Defines a link of the given type to the given name */
#define LLIST_LINK(type, name) type *name

/** reference to the first link in a linked list given the head */
#define LLIST_FIRST(head) head

/** reference the next link in the given the current link and linking attribute */
#define LLIST_NEXT(entry, attrib) entry->attrib

/** Returns a condition testing if the given list is empty */
#define LLIST_IS_EMPTY(head, attrib) (NULL == LLIST_FIRST(head))

/** Pushes an entry to the front of the linked-list given head and the linking attribute */
#define LLIST_PUSH(head, entry, attrib) do{\
    VITAL_IS_NULL((entry)->attrib);        \
    (entry)->attrib = LLIST_FIRST(head);   \
    LLIST_FIRST(head) = (entry);           \
} while(0)

/** Appends an entry to the endof a linked-list given the head and the linking attribute */
#define LLIST_APPEND(head, type, entry, attrib) do{\
    type *ll_iter=NULL;                             \
    VITAL_IS_NULL(entry->attrib);               \
    if(LLIST_IS_EMPTY(head, attrib)){               \
        LLIST_FIRST(head) = entry;                  \
    }else{                                          \
        ll_iter = LLIST_FIRST(head);                \
        while(NULL != LLIST_NEXT(ll_iter, attrib)){ \
            ll_iter = LLIST_NEXT(ll_iter, attrib);  \
        }                                           \
        LLIST_NEXT(ll_iter, attrib) = entry;        \
    }                                               \
} while(0)

/** Pops the first entry from the linked list onto the entry parameter */
#define LLIST_POP(head, entry, attrib) do{ \
    entry=LLIST_FIRST(head);               \
    if (NULL != entry) {                   \
        LLIST_FIRST(head) = entry->attrib; \
        entry->attrib=NULL;                \
    }                                      \
} while(0)

/** A for loop MACRO over all the elements in the linked-list which initialized a new local-variable for storing the entries */
#define LLIST_FOREACH(head, type, name, attrib) for(type *name=head; NULL != name; name = name->attrib)

/** A for loop MACRO over all the elements in the linked-list which uses and existing variable for storing the entries */
#define LLIST_LOOP(head, entry, attrib) for(entry=head; NULL != entry; entry = entry->attrib)

/** Finds an entry in the linked-list that matches a condition, sets entry to NULL if none are found */
#define LLIST_FIND(head, type, entry, attrib, condition) do{ \
    type *matching_entry=NULL;                               \
    LLIST_LOOP(head, entry, attrib) {                        \
        if(condition){                                       \
            matching_entry=entry;                            \
            break;                                           \
        }                                                    \
    }                                                        \
    entry=matching_entry;                                    \
} while(0)

/** Removes the given entry from the given linked-list */
#define LLIST_REMOVE(head, type, entry, attrib) do{         \
    if(LLIST_FIRST(head) == entry){                         \
        LLIST_POP(head, entry, attrib);                     \
    }else{                                                  \
        LLIST_FOREACH(head, type, ll_entry, attrib){        \
            if(ll_entry->attrib == entry){                  \
                LLIST_POP(ll_entry->attrib, entry, attrib); \
                break;                                      \
            }                                               \
        }                                                   \
    }                                                       \
}while (0)

/** Finds and removes entry linked-list that matches a condition, sets entry to NULL if none are found */
#define LLIST_FIND_AND_REMOVE(head, type, entry, attrib, condition) do{ \
    type *matching_entry=NULL;                                          \
    entry = LLIST_FIRST(head);                                          \
    if(NULL != entry && condition) {                                    \
        matching_entry = entry;                                         \
        LLIST_POP(head, entry, attrib);                                 \
    } else {                                                            \
        LLIST_FOREACH(head, type, ll_entry, attrib){                    \
            entry = ll_entry->attrib;                                   \
            if(NULL != entry &&condition){                              \
                matching_entry = entry;                                 \
                LLIST_POP(ll_entry->attrib, entry, attrib);             \
                break;                                                  \
            }                                                           \
        }                                                               \
    }                                                                   \
    entry=matching_entry;                                               \
}while (0)

#endif //HELLO_C_ABQ_LLIST_H
