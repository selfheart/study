//#line 2 "ontrac/traverser.c"
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
 * @file ontrac/traverser.c
 * @date May 29, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {Help Me}
 *
 */

#include <ontrac/ontrac/traverser.h>
#include <ontrac/ontrac/primitives.h>
#include <ontrac/util/byte_buffer.h>
#include <ontrac/util/abq_item_pool.h>

#ifndef MAX_TRAVERSERS
#define MAX_TRAVERSERS (32U)
#endif /* MAX_TRAVERSERS */

static void traverser_delete(cvar_t old_traverser);
static err_t traverser_finish_item(traverser_t *traverser);

DEFINE_CLASS(traverser_class, traverser_t, MAX_TRAVERSERS,
		NULL, NULL, NULL, NULL, traverser_delete);

static traverser_t* traverser_resolve(cvar_t item){
	traverser_t *retval = NULL;
	CLASS_RESOLVE(traverser_class, traverser_t, retval, item);
	return retval;
}

static void traverser_delete(cvar_t old_traverser){
	traverser_t* item = traverser_resolve(old_traverser);
	VITAL_NOT_NULL(item);
	err_t status = traverser_reinit(item, NULL, NULL);
	VITAL_IS_OK(status);
	ABQ_EXPECT(&null_class == traverser_root(item)->meta);
    // FATAL_IF(ref_dump_children(old_traverser))
}

static err_t traverser_nodeinit(traverser_t *traverser, abq_node_t *node, cvar_t item, class_ptr meta) {
    err_t retval = EXIT_SUCCESS;
    VITAL_NOT_NULL(traverser);
    VITAL_NOT_NULL(node);
    if (traverser->depth > ARRAY_COUNT(abq_node_t, traverser->stack)) {
        retval = ENOMEM;
    } else {
        ABQ_VITAL(traverser_leaf(traverser) == node);
        (void) obj_release(node->item, traverser);
        bytes_set(node, '\0', sizeof(abq_node_t));
        if (NULL == meta) {
            node->meta = class_of(item);
            // NULL item will return &null_class
            if (NULL == node->meta) {
                // Assume embedded string if unclassified
                node->item = string_class.coerce(item, traverser);
                if (NULL == node->item) {
                    retval = abq_status_take(EINVAL);
                } else {
                    node->meta = &string_class;
                }
            } else {
                retval = obj_reserve(item, traverser);
                node->item = item;
            }
        } else if (NULL == item) {
            // special case for de-serializing NULL object values from '{}' for top-level objects only
            if ((class_is_primitive(meta)) || (&byte_buffer_class == meta)) {
                // don't "create" primitives
                node->meta = class_realize(meta);
            } else if (NULL == meta->create) {
                // Each class_t is not de-serializable without a create method
                retval = abq_status_take(ENOSYS);
            } else {
                // new items in a list and or new properties on an object require
                node->item = meta->create(meta, traverser);
                if (NULL == node->item) {
                    retval = abq_status_take(ENOMEM);
                } else {
                    node->meta = meta;
                }
            }
        } else {
            node->item = classify(item, meta, traverser);
            if (NULL == node->item) {
                retval = abq_status_take(EINVAL);
            } else {
                node->meta = class_realize(meta);
            }
        }
        if (EXIT_SUCCESS != retval) {
            // don't do anything but return error code
        } else if (NULL == node->meta) {
            retval = EINVAL;
            ABQ_WARN_STATUS(retval, "Unclassified item");
        } else if ((class_is_primitive(node->meta))
                || (&byte_buffer_class == node->meta)) {
            // Non-iterable node is considered a complete state
            node->nodetype = ABQ_NODE_COMPLETE;
        } else if (class_is_list(node->meta)) {
            node->nodetype = ABQ_NODE_VARLIST;
            node->child_counter = -1;
            node->iter.nodelink = NULL;
        } else if (!class_is_serializable(node->meta)) {
            retval = ENOSYS;
            ABQ_WARN_STATUS(retval, CLASS_NAME(node->meta));
        } else {
            node->nodetype = ABQ_NODE_OBJECT;
            node->child_counter = -1;
            node->iter.nodeprop = NULL;
        }
    }
    return retval;
}

err_t traverser_reinit(traverser_t* traverser, cvar_t item, class_ptr meta) {
	err_t retval = CHECK_NULL(traverser);
	if (EXIT_SUCCESS == retval) {
		while(0UL != traverser->depth) {
			retval = traverser_finish_item(traverser);
			if (ECANCELED == retval) {
				ABQ_VITAL(traverser_is_completed(traverser));
				break;
			} else {
				VITAL_IS_OK(retval);
			}
		}
		retval = traverser_nodeinit(traverser,
				traverser_leaf(traverser), item, meta);
	}
	return retval;
}

traverser_t* traverser_create(cvar_t item, class_ptr meta, bool_t is_recursive, cvar_t ref) {
	traverser_t *retval = CREATE_BASE_INSTANCE(traverser_class, traverser_t);
	if (NULL != retval) {
		// bytes_set(retval, '\0', sizeof(traverser_t)) // Done in CREATE_BASE_INSTANCE(...)
		retval->is_recursive = is_recursive;
		abq_node_t *root = traverser_root(retval);
		err_t status = traverser_nodeinit(retval, root, item, meta);
		if (EXIT_SUCCESS == status) {
			// Don't want root node to start in a completed state
			//  else a reader's call to traverser_step will skip the root item
			if (ABQ_NODE_COMPLETE == root->nodetype) {
				root->nodetype = ABQ_NODE_INVALID;
			}
			VITAL_IS_OK(obj_takeover(retval, ref));
		} else {
			(void) abq_status_set(status, false);
			VITAL_IS_OK(obj_release_self(retval));
			retval = NULL;
		}
	}
	return retval;
}
static inline link_t *vlist_first_link(cvar_t item) {
	vlist_t *list = vlist_resolve(item);
	return (NULL == list) ? NULL : list->first;
}

// moves to next object property or list link of current instance, but does not call dive recurively
static err_t traverser_iterate(traverser_t *traverser) {
    err_t retval = EXIT_SUCCESS;
    abq_node_t *node = traverser_leaf(traverser);
    if(NULL == node) {
    	retval = EFAULT;
    } else {
    	switch(node->nodetype) {
    	case (ABQ_NODE_VARLIST):
    		if(0 > node->child_counter) {
				VITAL_IS_NULL(node->iter.nodelink);
				node->iter.nodelink = vlist_first_link(node->item);
				node->child_counter = 0;
    		} else {
				VITAL_NOT_NULL(node->iter.nodelink);
				node->child_counter += 1;
				node->iter.nodelink
					= node->iter.nodelink->next;
    		}
    		break;
    	case (ABQ_NODE_OBJECT):
			if(0 > node->child_counter) {
				// No longer start of objec
				node->child_counter = 0;
                VITAL_IS_NULL(node->iter.nodeprop);
                ptree_t* ptree = node->meta->properties_of(node->item);
                node->iter.nodeprop = LLIST_FIRST(ptree->props);
			}else{
				VITAL_NOT_NULL(node->iter.nodeprop);
				node->child_counter += 1;
                node->iter.nodeprop = LLIST_NEXT(node->iter.nodeprop, next);
            }
            // Skip over NULL properties
            while ((NULL != node->iter.nodeprop)
                                && (NULL == property_get_value(node->iter.nodeprop, node->item))) {
                node->iter.nodeprop = LLIST_NEXT(node->iter.nodeprop, next);
            }
    		break;
    	case (ABQ_NODE_COMPLETE):
            abq_fatal(__FILENAME__, __LINE__, // parasoft-suppress CERT_C-MSC41-a-1 "c0347. This string does not contain sensitive information."
                      EILSEQ, "?"); // parasoft-suppress CERT_C-MSC41-a-1 "c0348. This string does not contain sensitive information."
			break;
    	default:
            abq_fatal(__FILENAME__, __LINE__, // parasoft-suppress CERT_C-MSC41-a-1 "c0349. This string does not contain sensitive information."
                      EINVAL, "?"); // parasoft-suppress CERT_C-MSC41-a-1 "c0350. This string does not contain sensitive information."
    		break;
    	};
    }
    return retval;
}

err_t traverser_step(traverser_t *traverser){
    err_t retval = CHECK_NULL(traverser);
    if(EXIT_SUCCESS == retval) {
    	abq_node_t *node = traverser_leaf(traverser);
    	if(0U == traverser->depth) {
    		if (ABQ_NODE_COMPLETE == node->nodetype) {
    			retval = ECANCELED;
    		} else {
    			// Begin the root item
    			traverser->depth += 1UL;
    			if (ABQ_NODE_INVALID == node->nodetype) {
    				// Hack for primitives at the root level
    				node->nodetype = ABQ_NODE_COMPLETE;
    			}
    		}
    	}else if(traverser->is_recursive){
        	// Recursive iteration
        	switch(node->nodetype){
        	case (ABQ_NODE_VARLIST):
        			if(0 > node->child_counter) {
        				// Iterator to first child or endif_item
                    	retval = traverser_iterate(traverser);
        			}else if(NULL != node->iter.nodelink){
        				retval = traverser_begin_node(traverser,
        						node->iter.nodelink->item, NULL);
        			} else {
        				retval = traverser_finish_item(traverser);
        				if (EXIT_SUCCESS != retval) {
        					// Return error as is
        				} else if (traverser_is_endof_node(traverser)) {
        					// Don't iterate if endof_item
        				} else {
                        	retval = traverser_iterate(traverser);
                        }
        			}
        		break;
        	case (ABQ_NODE_OBJECT):
				if(0 > node->child_counter) {
					// Iterator to first child or endif_item
					retval = traverser_iterate(traverser);
				}else if(NULL != node->iter.nodeprop) {
	                    retval = traverser_begin_node(traverser,
	                            property_get_value(node->iter.nodeprop, node->item),
	                            property_get_class(node->iter.nodeprop));
    			} else {
    				retval = traverser_finish_item(traverser);
    				if (EXIT_SUCCESS != retval) {
    					// Return error as is
    				} else if (traverser_is_endof_node(traverser)) {
    					// Don't iterate if endof_item
    				} else {
                    	retval = traverser_iterate(traverser);
                    }
    			}
        		break;
        	case (ABQ_NODE_COMPLETE):
				retval = traverser_finish_item(traverser);
				if (EXIT_SUCCESS != retval) {
					// Return error as is
				} else if (traverser_is_endof_node(traverser)) {
					// Don't iterate if endof_item
				} else {
					retval = traverser_iterate(traverser);
				}
				break;
        	default:
                abq_fatal(__FILENAME__, __LINE__, // parasoft-suppress CERT_C-MSC41-a-1 "c0351. This string does not contain sensitive information."
                          EPERM, "?"); // parasoft-suppress CERT_C-MSC41-a-1 "c0352. This string does not contain sensitive information."
        		break;
        	}
        } else {
        	// Non-Recursive iteration
        	if(traverser_is_endof_node(traverser)) {
				retval = ECANCELED;
        	}else{
				retval = traverser_iterate(traverser);
        	}
        }
    }
    return retval;
}

bool_t traverser_has_child(traverser_t *traverser) {
	bool_t retval = false;
	if((NULL != traverser) && (0U != traverser->depth)) {
		abq_node_t *node = traverser_leaf(traverser);
    	switch(node->nodetype){
    	case (ABQ_NODE_VARLIST):
			if((NULL != node->iter.nodelink)
					&& (NULL != node->iter.nodelink->item)){
				retval = true;
			}
    		break;
		case (ABQ_NODE_OBJECT):
			if((NULL != node->iter.nodeprop) &&
					(NULL != property_get_value(node->iter.nodeprop, node->item))) {
				retval = true;
			}
			break;
		case (ABQ_NODE_COMPLETE):
			retval = false;
			break;
		default:
            abq_fatal(__FILENAME__, __LINE__, // parasoft-suppress CERT_C-MSC41-a-1 "c0353. This string does not contain sensitive information."
                      ENOSYS, "?"); // parasoft-suppress CERT_C-MSC41-a-1 "c0354. This string does not contain sensitive information."
			break;
    	}
	}
	return retval;
}

bool_t traverser_next(traverser_t *traverser, cstr_t *name, cvar_t *value) {
    bool_t retval = false;
    if (&traverser_class == class_of(traverser)) {
        err_t status = EXIT_SUCCESS;
        cstr_t the_name = NULL;
        cvar_t the_value = NULL;
    	do {
    		status = traverser_step(traverser);
			abq_node_t *node = (traverser->is_recursive)
					? traverser_branch(traverser) : traverser_leaf(traverser);
			if (EXIT_SUCCESS != status) {
				break; // Completed iteration
			} else if (NULL != node) {
				switch(node->nodetype){
				case (ABQ_NODE_VARLIST):
					// the_name = NULL
					if(traverser->is_recursive) {
						if ((traverser_is_endof_node(traverser)) || (traverser_is_new_node(traverser))) {
							the_value = traverser_leaf(traverser)->item;
						}
					} else {
						if((NULL != node->iter.nodelink)
								&& (NULL != node->iter.nodelink->item)){
							the_value = node->iter.nodelink->item;
						}
					}
					break;
				case (ABQ_NODE_OBJECT):
					if(traverser->is_recursive) {
						if ((traverser_is_endof_node(traverser)) || (traverser_is_new_node(traverser))) {
							if (NULL != node->iter.nodeprop) {
								the_name = node->iter.nodeprop->name;
							}
							the_value = traverser_leaf(traverser)->item;
						}
					} else {
						if(NULL != node->iter.nodeprop) {
							the_value = property_get_value(node->iter.nodeprop, node->item);
							if(NULL != the_value) {
								the_name = node->iter.nodeprop->name;
							}
						}
					}
					break;
				case (ABQ_NODE_COMPLETE):
					if(traverser_is_root(traverser)) {
						// Skip root item ? done
						// status = ECANCELED
					} else {
						// the_name = NULL
						the_value = node->item;
					}
					break;
				default:
		            abq_fatal(__FILENAME__, __LINE__, // parasoft-suppress CERT_C-MSC41-a-1 "c0355. This string does not contain sensitive information."
                        ENOSYS, "?"); // parasoft-suppress CERT_C-MSC41-a-1 "c0356. This string does not contain sensitive information."
					break;
				}
			} else {
				ABQ_VITAL(traverser->is_recursive);
				FATAL_IF(traverser_is_completed(traverser));
				if ((traverser_is_endof_node(traverser)) || (traverser_is_new_node(traverser))) {
					the_value = traverser_root(traverser)->item;
				}
			}
    	} while(NULL == the_value) ;

    	if(NULL != the_value) {
    		retval = true;
    	} else {
    		if (ECANCELED != status) {
    			// Expected to be an error, just printing a log messages
    			EXPECT_IS_OK(status);
    		}
    		retval = false;
    	}
        if(NULL != name) {
            *name = the_name;
        }
        if(NULL != value) {
            *value = the_value;
        }
    }
    return retval;
}

static inline class_ptr vlist_class_of_items(const vlist_t* vlist, class_ptr if_null){
	class_ptr retval = if_null;
	if((NULL != vlist) && (NULL != vlist->class_of_items)){
		retval = vlist->class_of_items;
	}
	return retval;
}

err_t traverser_begin_node(traverser_t *traverser, cvar_t item, class_ptr item_meta) {
    err_t retval = CHECK_NULL(traverser);
    if((EXIT_SUCCESS == retval) && (NULL != item)) {
        for(size_t index=0UL; index < traverser->depth; index++){
        	if(item == traverser->stack[index].item){
                // TODO use JSON references (i.e. "$ref":"#{path}") instead of failure
                retval = EEXIST;
                break;
        	}
        }
    }
    if (EXIT_SUCCESS != retval) {
        // Return error code as is
    } else if (traverser->depth >= ARRAY_COUNT(abq_node_t, traverser->stack)) {
        retval = ENOMEM;
    } else {
        abq_context_lock();
        class_ptr meta = item_meta;
        abq_node_t* branch = traverser_leaf(traverser);
        if (0UL == traverser->depth) {
            traverser->depth += 1UL;
            // Begin the root node (initialize)
            if (class_is_primitive(branch->meta)) {
                // Continue using newly specified item_meta as provided
            } else {
                ABQ_VITAL(NULL != branch->item);
                // meta = branch->meta => unused
                retval = EALREADY;
            }
        } else if (ABQ_NODE_VARLIST == branch->nodetype) {
            traverser->depth += 1UL;
            meta = vlist_class_of_items(vlist_resolve(branch->item), meta);
        } else if (ABQ_NODE_OBJECT == branch->nodetype) {
            traverser->depth += 1UL;
            if (NULL != branch->iter.nodeprop) {
                meta = property_get_class(branch->iter.nodeprop);
                if (NULL == meta) {
                    meta = item_meta;
                }
            }
        } else {
            retval = EILSEQ;
        }
		if (EXIT_SUCCESS == retval) {
			// leaf may no longer be equal to above branch
			retval = traverser_nodeinit(traverser,
					traverser_leaf(traverser), item, meta);
		} else if(EALREADY == retval) {
			ABQ_VITAL(traverser_leaf(traverser) == branch);
			retval = EXIT_SUCCESS;
		} else {
			// Return error code as is
		}
		abq_context_unlock();
	}
    return retval;
}

static err_t traverser_finish_item(traverser_t *traverser) {
    err_t retval = CHECK_NULL(traverser);
    if (EXIT_SUCCESS != retval) {
    	// Return error code as is
    }else if(traverser_is_root(traverser)){
    	traverser_root(traverser)->nodetype = ABQ_NODE_COMPLETE;
    	traverser->depth = 0UL;
    	retval = ECANCELED;
    }else{
		abq_context_lock();
		abq_node_t *node = traverser_leaf(traverser);
		(void) obj_release(node->item, traverser);
        node->nodetype = ABQ_NODE_COMPLETE;
		node->item = NULL;
		traverser->depth -= 1UL;
		abq_context_unlock();
    }
    return retval;
}

static err_t abq_branch_add_link(abq_node_t* branch, cvar_t item){
	err_t retval = EXIT_SUCCESS;
	vlist_t *list = vlist_resolve(branch->item);
	if(NULL == list) {
		retval = abq_status_take(EINVAL);
	} else {
#if !defined(NDEBUG)
	    class_ptr meta = class_of(item);
	    if ((NULL != meta) && (!class_is_primitive(meta))
	            && (vlist_contains(list, item))) {
	        ABQ_WARN_MSG_Z("Duplicate detected: ", CLASS_NAME(meta));
	    }
#endif /* !defined(NDEBUG) */
        cvar_t coerced = abq_coerce(item, list->class_of_items, NULL);
		if((NULL == coerced) && (NULL != item)) {
			retval = abq_status_take(EINVAL);
		}else{
			retval = vlist_add(list, coerced);
			if (EXIT_SUCCESS != retval) {
			    // return error as is
			} else {
			    ABQ_VITAL(list->last->item == (cvar_t)coerced);
			    branch->iter.nodelink = list->last;
                if (0 > branch->child_counter) {
                    branch->child_counter = 1;
                }else{
                    branch->child_counter += 1;
                }
			}
		}
		(void) obj_release_self(coerced);
	}
	return retval;
}

static err_t abq_branch_set_leaf(abq_node_t* branch, cvar_t item){
	err_t retval = EXIT_SUCCESS;
	if (NULL == branch->item) {
		retval = EILSEQ;
	} else if(NULL == branch->iter.nodeprop) {
        cstr_t name = str_resolve(item);
		if (name == NULL) {
			retval = EILSEQ;
		} else {
			// Select next property based on the provided name
			branch->iter.nodeprop
				= property_by_name(branch->item, branch->meta, name);
		}
	} else {
        retval = property_set_value(branch->iter.nodeprop,
                branch->item, item);
        // And set nodeprop to NULL so that validation passes
        // branch->iter.nodeprop = NULL -> moved to json_reader_decode
        if(0 > branch->child_counter) {
            branch->child_counter = 1;
        }else{
            branch->child_counter += 1;
        }
	}
	return retval;
}

err_t traverser_set_child(traverser_t *traverser, abq_node_t* node, cvar_t item) {
	err_t retval = CHECK_NULL(node);
	if(EXIT_SUCCESS == retval) {
		switch(node->nodetype){
		case (ABQ_NODE_VARLIST):
			retval = abq_branch_add_link(node, item);
			break;
		case (ABQ_NODE_OBJECT):
			retval = abq_branch_set_leaf(node, item);
			break;
		case (ABQ_NODE_COMPLETE):
			retval = EILSEQ;
			break;
        case (ABQ_NODE_INVALID):
            // Uninitialized node (expected to be root node)
            retval = traverser_nodeinit(traverser, node, item, node->meta);
            break;
		default:
            abq_fatal(__FILENAME__, __LINE__, // parasoft-suppress CERT_C-MSC41-a-1 "c0357. This string does not contain sensitive information."
                      ENOSYS, "?"); // parasoft-suppress CERT_C-MSC41-a-1 "c0358. This string does not contain sensitive information."
			break;
		}
	}
	return retval;
}

err_t traverser_finalize_node(traverser_t *traverser) {
    err_t retval = CHECK_NULL(traverser);
    if (EXIT_SUCCESS != retval) {
    	// Return error code as is
    }else if(traverser_is_root(traverser)){
    	traverser_root(traverser)->nodetype = ABQ_NODE_COMPLETE;
    	traverser->depth = 0UL;
    	retval = ECANCELED;
    }else{
        abq_context_lock();
		abq_node_t *child = traverser_leaf(traverser);
		abq_node_t *parent = traverser_branch(traverser);
		retval = traverser_set_child(traverser, parent, child->item);
        if(EXIT_SUCCESS != retval){
        	// Return to parent despite coercion error
        	EXPECT_IS_OK(traverser_finish_item(traverser));
        }else{
        	retval = traverser_finish_item(traverser);
        }
    	abq_context_unlock();
    }
    return retval;
}

static err_t traverser_copy(traverser_t *source, traverser_t *dest){
    err_t retval = EXIT_SUCCESS;
    if((NULL == source)|| (NULL == dest)) {
        retval = EFAULT;
    } else {
        abq_node_t *from = traverser_leaf(source);
        abq_node_t *into = traverser_leaf(dest);
    	if ((class_is_primitive(from->meta))
    	        || (&byte_buffer_class == from->meta)) {
			retval = traverser_set_child(dest, into, from->item);
		} else if(ABQ_NODE_OBJECT == from->nodetype) {
			if (traverser_is_new_node(source)) {
				retval = traverser_begin_node(dest, NULL, &ptree_class);
			} else if (traverser_is_endof_node(source)) {
				retval = traverser_finalize_node(dest);
				if(EXIT_SUCCESS == retval) {
				    ABQ_VITAL(traverser_do_break(dest));
				}
			} else {
                cstr_t property_name = abq_node_child_name(from);
				ABQ_VITAL(NULL != property_name);
				// Refresh leaf (which may have changed)
				into = traverser_leaf(dest);
				into->iter.nodeprop = property_by_name(into->item,
						into->meta, property_name);
				if(NULL == into->iter.nodeprop) {
					retval = NOT_FOUND;
					ABQ_DUMP_ERROR(retval, property_name);
				}
				EXPECT_NOT_NULL(into->iter.nodeprop);
			}
		} else if(ABQ_NODE_VARLIST == from->nodetype) {
			if (traverser_is_new_node(source)) {
				retval = traverser_begin_node(dest, NULL, &vlist_class);
			} else if (traverser_is_endof_node(source)) {
				retval = traverser_finalize_node(dest);
                if(EXIT_SUCCESS == retval) {
                    ABQ_VITAL(traverser_do_break(dest));
                }
			} else {
				// No need to create the link here
			}
		} else {
            abq_fatal(__FILENAME__, __LINE__, // parasoft-suppress CERT_C-MSC41-a-1 "c0359. This string does not contain sensitive information."
                      ENOSYS, "?"); // parasoft-suppress CERT_C-MSC41-a-1 "c0360. This string does not contain sensitive information."
		}
    }
    return retval;
}

cvar_t build_class_instance(cvar_t serializable, class_ptr class_of_item) {
    cvar_t retval = NULL;
    // class_of_item only applies to the newly created instance,
    //  use NULL for old instance so that it loads the class of the item
    traverser_t *source
        = traverser_create(serializable, NULL, true, NULL);
    class_ptr meta = (NULL == class_of_item)
            ? class_of(serializable) : class_of_item;
    if (NULL == meta) {
        ABQ_DUMP_ERROR(EINVAL, "unclassified");
    } else {
        traverser_t *dest
            = traverser_create(NULL, meta, true, NULL);
        err_t status = traverser_step(source);
        while (EXIT_SUCCESS == status) {
            status = traverser_copy(source, dest);
            if (EXIT_SUCCESS == status) {
                status = traverser_step(source);
            }
        }
        if (ECANCELED == status) {
            retval = abq_coerce(traverser_root(dest)->item, meta, NULL);
        } else {
            (void) abq_status_set(status, false);
        }
        (void) obj_release_self(dest);
    }
    (void) obj_release_self(source);
    return retval;
}
