/* ****************************************************************************
 *
 *         AIRBIQUITY INC. PROPRIETARY AND CONFIDENTIAL INFORMATION
 *
 *  The software and information contained herein is the proprietary and
 *  confidential property of Airbiquity Inc. and shall not be disclosed
 *  in whole or in part. Possession, use, reproduction or transfer of
 *  this software and information is prohibited without the express written
 *  permission of Airbiquity Inc.
 *
 *  Copyright (c) 2019 Airbiquity Inc.  All rights reserved.
 *
 * ****************************************************************************
 */
/**
 * @file datasink.c
 * @date Jan 9, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#include <ontrac/stream/datasink.h>
#include <ontrac/util/abq_item_pool.h>

#ifndef MAX_SINKS
#define MAX_SINKS (24U)
#endif /* MAX_SINKS */

static void datasink_delete(cvar_t old_datasink);

DEFINE_CLASS(datasink_class, datasink_t, MAX_SINKS, NULL, NULL, NULL, NULL, datasink_delete);

// Expected to have ~ two listeners per datasink_t, one or more data-writers with an optional pipeline listeners
STATIC_ITEM_POOL(datasink_listener_t, (2*MAX_SINKS), datasink_listener_pool);

datasink_t* datasink_resolve(cvar_t item) {
    datasink_t* retval = NULL;
    CLASS_RESOLVE(datasink_class, datasink_t, retval, item);
    return retval;
}

static void datasink_delete(cvar_t old_datasink) {
    datasink_t *datasink = datasink_resolve(old_datasink);
    VITAL_NOT_NULL(datasink);
    if (!abq_io_is_closed(datasink->flags)) {
        // ugly hack, overwrite publisher state so that events fire
        datasink->publish_state = ABQ_PUBLISH_INACTIVE;
        EXPECT_IS_OK(datasink_close(datasink));
        ABQ_VITAL(abq_io_is_closed(datasink->flags));
    }
    ABQ_VITAL(LLIST_IS_EMPTY(datasink->active, next));
    ABQ_VITAL(LLIST_IS_EMPTY(datasink->pending, next));
    SET_RESERVED_FIELD(datasink, handle, NULL);
}

extern datasink_t* datasink_create(datasink_seek_funtion_t seek,
        datasink_write_function_t write, datasink_flush_function_t flush,
        datasink_close_function_t close, var_t handle) {
    VITAL_NOT_NULL(write);
    VITAL_NOT_NULL(close);
    datasink_t* retval = CREATE_BASE_INSTANCE(datasink_class, datasink_t);
    if (NULL != retval) {
        (void) obj_reserve(handle, retval);
        *retval = (datasink_t ) {
                        .seek = seek,
                        .write = write,
                        .flush = flush,
                        .close = close,
                        .active = NULL,
                        .pending = NULL,
                        .handle = handle,
                        .flags = ABQ_IO_FLUSH,
                        .publish_state = ABQ_PUBLISH_INACTIVE
        };
    }
    return retval;
}

int64_t datasink_seek (datasink_t *datasink, int64_t offset, abq_whence_t whence) {
    LOCK_CLASS(datasink_class);
    int64_t retval = -1;
    err_t status = datasink_get_status(datasink);
    if(EXIT_SUCCESS != status){
        abq_status_set(status, true);
    }else if(NULL == datasink->seek){
        abq_status_set(ENOSYS, true);
    }else{
        retval = datasink->seek(datasink, offset, whence);
    }
    UNLOCK_CLASS(datasink_class);
    return retval;
}

int64_t datasink_write (datasink_t *datasink, const void *source, int64_t count) {
    LOCK_CLASS(datasink_class);
    int64_t retval = -1;
    err_t status = datasink_get_status(datasink);
    if (EXIT_SUCCESS != status) {
        abq_status_set(status, true);
    }else if(abq_io_writable(datasink->flags)){
        // VITAL_NOT_NULL(datasink->write)
        retval = datasink->write(datasink, source, count);
    } else {
        abq_status_set(EWOULDBLOCK, true);
    }
    UNLOCK_CLASS(datasink_class);
    return retval;
}

err_t datasink_flush (datasink_t *datasink) {
    LOCK_CLASS(datasink_class);
    err_t retval = datasink_get_status(datasink);
    if (EXIT_SUCCESS != retval) {
        // Keep error as is
    } else if(NULL == datasink->flush) {
        datasink->flags |= ABQ_IO_FLUSH;
        retval = ENOSYS;
    } else {
        retval = datasink->flush(datasink);
        if (EXIT_SUCCESS == retval) {
            ABQ_EXPECT(abq_io_is_flushed(datasink->flags));
        }
    }
    UNLOCK_CLASS(datasink_class);
    return retval;
}

err_t datasink_close(datasink_t *datasink) {
    LOCK_CLASS(datasink_class);
    err_t retval = class_check(&datasink_class, datasink);
    if (EXIT_SUCCESS != retval) {
        // Return error code as is
    } else if(abq_io_is_closed(datasink->flags)) {
        if (ABQ_PUBLISH_INACTIVE == datasink->publish_state) {
            ABQ_VITAL(LLIST_IS_EMPTY(datasink->active, next));
            ABQ_VITAL(LLIST_IS_EMPTY(datasink->pending, next));
        }
        retval = EALREADY;
    } else {
        // VITAL_NOT_NULL(datasink->close)
        retval = datasink->close(datasink);
        // Check if event was fired as part of the call to close
        if (CHECK_WOULDBLOCK(retval)) {
            // Expect that the implementation will
            //  publish the ABQ_IO_CLOSED event when complete
        } else if(abq_io_is_closed(datasink->flags)) {
            // Event was already published
        } else {
            datasink_publish(datasink, ABQ_IO_CLOSED);
        }
    }
    UNLOCK_CLASS(datasink_class);
    return retval;
}

bool_t datasink_has_listener(const datasink_t *datasink,
        datasink_event_callback_t callback, cvar_t cbdata) {
    LOCK_CLASS(datasink_class);
    bool_t retval = false;
    if(&datasink_class == class_of(datasink)) {
        if (NULL != datasink) {
            datasink_listener_t *listener = NULL;
            LLIST_FIND(datasink->active, datasink_listener_t, listener, next,
                    ((callback == listener->callback) && (cbdata == listener->cbdata)));
            if(NULL != listener) {
                retval = true;
            } else {
                LLIST_FIND(datasink->pending, datasink_listener_t, listener, next,
                        ((callback == listener->callback) && (cbdata == listener->cbdata)));
                if(NULL != listener) {
                    retval = true;
                }
            }
        }
    }
    UNLOCK_CLASS(datasink_class);
    return retval;
}

err_t datasink_add_datawriter(datasink_t * datasink,
        datasink_event_callback_t callback, cvar_t cbdata) {
    LOCK_CLASS(datasink_class);
    err_t retval = class_check(&datasink_class, datasink);
    if (EXIT_SUCCESS != retval) {
        // Return error code as is
    } else if(NULL == callback) {
        retval = EINVAL;
    } else if (abq_io_is_closed(datasink->flags)) {
        // datasink has been closed
        retval = NIO_CLOSED;
    } else {
        abq_io_flags ioflags = ABQ_IO_DATA;
        datasink_listener_t *listener = LLIST_FIRST(datasink->active);  // Check for active listener
        if((NULL != listener) && (abq_io_data_check(listener->eflags))) {
            // Active data-listener already exists, add new listener to pending datawriters
            ITEM_POOL_ALLOC(&datasink_listener_pool, datasink_listener_t, listener);
            if (NULL == listener) {
                retval = abq_status_take(ENOMEM);
            } else {
                (void) obj_reserve(cbdata, datasink);
                *listener = (datasink_listener_t ) {
                                .eflags = ioflags,
                                .callback = callback,
                                .cbdata = cbdata,
                                .next = NULL
                };
                // Append pipe-listeners to the end of the list of pending listeners
                LLIST_APPEND(datasink->pending,
                        datasink_listener_t, listener, next);
                // Not going to be the active data-listener, return a corresponding status-code
                retval = EINPROGRESS;
            }
        } else {
            // If no active data-listener was found, then there should also be no pending listeners
            ABQ_VITAL(LLIST_IS_EMPTY(datasink->pending, next));
            // Search for matching pipe-listener in active listeners
            LLIST_FIND_AND_REMOVE(datasink->active, datasink_listener_t, listener, next,
                    ((callback == listener->callback) && (cbdata == listener->cbdata)));
            if (NULL != listener) {
                // Callback function and cbdata matches the an active listener
                //  update flags on existing listener
                listener->eflags |= ioflags;
                // Push data-listeners to the beginning of the list
                LLIST_PUSH(datasink->active, listener, next);
                // Since we now have a datawriter, sink is no longer in a flushed state
                datasink->flags &= ~ABQ_IO_FLUSH;
            } else {
                // First time listener, create the active-listener
                ITEM_POOL_ALLOC(&datasink_listener_pool, datasink_listener_t, listener);
                if (NULL == listener) {
                    retval = abq_status_take(ENOMEM);
                } else {
                    (void) obj_reserve(cbdata, datasink);
                    *listener = (datasink_listener_t ) {
                                    .eflags = ioflags,
                                    .callback = callback,
                                    .cbdata = cbdata,
                                    .next = NULL
                    };
                    // Push data-listeners to the beginning of the list
                    LLIST_PUSH(datasink->active, listener, next);
                    // Since we now have a datawriter, sink is no longer in a flushed state
                    datasink->flags &= ~ABQ_IO_FLUSH;
                }
            }
            // Finally, invoke the callback if applicable
            if ((ABQ_IO_NOFLAGS != (ioflags & datasink->flags))
                    && (EXIT_SUCCESS == retval) ) {
                // Should call "publish" to manage potential recursion
                //  as opposed to invoking callback directly
                if (ABQ_PUBLISH_INACTIVE == datasink->publish_state) {
                    datasink_publish(datasink, datasink->flags);
                } else {
                    datasink->publish_state = ABQ_PUBLISH_DIRTY;
                }
            }
        }
    }
    UNLOCK_CLASS(datasink_class);
    return retval;
}

err_t datasink_add_pipelistener(datasink_t * datasink,
        datasink_event_callback_t callback, cvar_t cbdata) {
    LOCK_CLASS(datasink_class);
    err_t retval = class_check(&datasink_class, datasink);
    if (EXIT_SUCCESS != retval) {
        // Return error code as is
    } else if(NULL == callback) {
        retval = EINVAL;
    } else if (abq_io_is_closed(datasink->flags)) {
        // datasink has been closed, todo consider ramifications of invoking the callback directly
        retval = NIO_CLOSED;
    } else {
        datasink_listener_t *listener = NULL;
        abq_io_flags ioflags = ABQ_IO_PIPELINE;
        // Search for matching pipe listener in active listeners
        LLIST_FIND(datasink->active, datasink_listener_t, listener, next,
                ((callback == listener->callback) && (cbdata == listener->cbdata)));
        if (NULL != listener) {
            // Callback function and cbdata matches the an active listener
            //  update flags on existing listener
            listener->eflags |= ioflags;
        } else {
            // No active-listener matches the criterial, create a new one
            ITEM_POOL_ALLOC(&datasink_listener_pool, datasink_listener_t, listener);
            if (NULL == listener) {
                retval = abq_status_take(ENOMEM);
            } else {
                (void) obj_reserve(cbdata, datasink);
                *listener = (datasink_listener_t ) {
                                .eflags = ioflags,
                                .callback = callback,
                                .cbdata = cbdata,
                                .next = NULL
                };
                // Append pipe-listeners to the end of the list
                LLIST_APPEND(datasink->active,
                        datasink_listener_t, listener, next);
            }
        }
        // Finally, invoke the callback if applicable
        if ((ABQ_IO_NOFLAGS != (ioflags & datasink->flags))
                && (EXIT_SUCCESS == retval) ) {
            // Should call "publish" to manage potential recursion
            //  as opposed to invoking callback directly
            if (ABQ_PUBLISH_INACTIVE == datasink->publish_state) {
                datasink_publish(datasink, datasink->flags);
            } else {
                datasink->publish_state = ABQ_PUBLISH_DIRTY;
            }
        }
        UNLOCK_CLASS(datasink_class);
    }
    return retval;
}

err_t datasink_set_combolistener(datasink_t * datasink, datasink_event_callback_t callback, cvar_t cbdata) {
    err_t retval = datasink_add_pipelistener(datasink, callback, cbdata);
    if(status_code_is_ok(retval)) {
        retval = datasink_add_datawriter(datasink, callback, cbdata);
    }
    return retval;
}

err_t datasink_remove_listener(datasink_t * datasink, datasink_event_callback_t callback, cvar_t cbdata) {
    err_t retval = CHECK_NULL(datasink);
    if (EXIT_SUCCESS != retval) {
        // Return error code as is
    } else if(NULL == callback) {
        retval = EINVAL;
    } else {
        LOCK_CLASS(datasink_class);
        retval = EALREADY;
        datasink_listener_t *listener = NULL;
        LLIST_FIND_AND_REMOVE(datasink->active, datasink_listener_t, listener, next,
                ((callback == listener->callback) && (cbdata == listener->cbdata)));
        if (NULL != listener) {
            retval = EXIT_SUCCESS;
            bool_t was_datawriter = abq_io_data_check(listener->eflags);
            // Found at least one listener, overwrite the NOT_FOUND status code and search for more
            (void) obj_release(listener->cbdata, datasink);
            listener->cbdata = NULL;
            ITEM_POOL_FREE(&datasink_listener_pool, datasink_listener_t, listener);
            if (was_datawriter) {
                if (LLIST_IS_EMPTY(datasink->pending, next)) {
                    // All datawriters have been pushed through sink, mark as flushed as able
                    (void) datasink_flush(datasink);
                } else {
                    // If we removed the datawriter and there are more pending
                    //  we need to iterate to the next listener
                    LLIST_POP(datasink->pending, listener, next);
                    LLIST_PUSH(datasink->active, listener, next);
                    retval = EINPROGRESS;
                }
            }
        }else{
            LLIST_FIND_AND_REMOVE(datasink->pending, datasink_listener_t, listener, next,
                    ((callback == listener->callback) && (cbdata == listener->cbdata)));
            if(NULL != listener) {
                retval = EXIT_SUCCESS;
                // Found at least one listener, overwrite the NOT_FOUND status code and search for more
                (void) obj_release(listener->cbdata, datasink);
                listener->cbdata = NULL;
                ITEM_POOL_FREE(&datasink_listener_pool, datasink_listener_t, listener);
            }
        }
        if (ABQ_PUBLISH_INACTIVE == datasink->publish_state) {
            datasink_publish(datasink, datasink->flags);
        } else {
            datasink->publish_state = ABQ_PUBLISH_DIRTY;
        }
        UNLOCK_CLASS(datasink_class);
    }
    return retval;
}

void datasink_publish(datasink_t * datasink, abq_io_flags new_flags) {
    LOCK_CLASS(datasink_class);
    if (&datasink_class == class_of(datasink)) {
        // Put here to eliminate recursive/indefinite publications
        if ((ABQ_PUBLISH_INACTIVE != datasink->publish_state)
                && (false == abq_is_aborting())) {
            // Check for additional flags set in new_flags
            if (((new_flags | datasink->flags) != datasink->flags)) {
                datasink->flags |= new_flags;
                datasink->publish_state = ABQ_PUBLISH_DIRTY;
            }
        } else {
            // To prevent data-collection during a publish
            //  which may happen when publishing a ABQ_IO_CLOSE event
            (void) obj_reserve_self(datasink);
            datasink->flags |= new_flags;
            datasink_listener_t *listener = NULL;
            while (ABQ_PUBLISH_CLEAN != datasink->publish_state) {
                datasink->publish_state = ABQ_PUBLISH_CLEAN;
                listener = LLIST_FIRST(datasink->active);
                while ((ABQ_PUBLISH_CLEAN == datasink->publish_state)
                        && (NULL != listener)) {
                    if (ABQ_IO_NOFLAGS != (datasink->flags & listener->eflags)) {
                        datasink_event_callback_t callback = listener->callback;
                        cvar_t cbdata = listener->cbdata;
                        listener = LLIST_NEXT(listener, next);
                        // Invoke the callback
                        (void) obj_reserve(cbdata, datasink);
                        callback(datasink, cbdata);
                        (void) obj_release(cbdata, datasink);
                    } else {
                        // Iterate without a callback
                        listener = LLIST_NEXT(listener, next);
                    }
                }
            }
            if (abq_io_is_closed(datasink->flags)) {
                // Remove all active listeners from the list once datasink has been closed
                while (false == LLIST_IS_EMPTY(datasink->active, next)) {
                    LLIST_POP(datasink->active, listener, next);
                    listener->callback = NULL;
                    (void) obj_release(listener->cbdata, datasink);
                    listener->cbdata = NULL;
                    ITEM_POOL_FREE(&datasink_listener_pool, datasink_listener_t, listener);
                }
                // Remove all pending listeners from the list once datasink has been closed
                while (false == LLIST_IS_EMPTY(datasink->pending, next)) {
                    LLIST_POP(datasink->pending, listener, next);
                    listener->callback = NULL;
                    (void) obj_release(listener->cbdata, datasink);
                    listener->cbdata = NULL;
                    ITEM_POOL_FREE(&datasink_listener_pool, datasink_listener_t, listener);
                }
            }
            // Done publishing event,
            datasink->publish_state = ABQ_PUBLISH_INACTIVE;
            (void) obj_release_self(datasink);
        }
    }
    UNLOCK_CLASS(datasink_class);
}

static void datasink_bufwriter(datasink_t *sink, cvar_t cbdata) {
    byte_buffer_t * buf = buf_resolve(cbdata);
    VITAL_NOT_NULL(buf);
    err_t status = buf_push_to_sink(buf, sink);
    while(EXIT_SUCCESS == status) {
        status = buf_push_to_sink(buf, sink);
    }
    if (CHECK_WOULDBLOCK(status)) {
        // Wait for sink to become writable
    } else {
        VITAL_IS_OK(datasink_remove_listener(sink, datasink_bufwriter, cbdata));
        if(ENODATA != status) {
            DATASINK_ERROR(sink, status);
        }
    }
}

err_t datasink_add_bufwriter(datasink_t *datasink, const byte_buffer_t* buf) {
    return datasink_add_datawriter(datasink, datasink_bufwriter, buf);
}

void datasink_close_on_event(datasink_t * datasink, cvar_t cbdata) {
    VITAL_IS_NULL(cbdata);
    EXPECT_IS_OK(datasink_close(datasink));
}
