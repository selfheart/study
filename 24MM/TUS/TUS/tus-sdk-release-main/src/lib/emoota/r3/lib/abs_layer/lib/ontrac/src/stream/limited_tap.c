/*
 * limited_tap.c
 *
 *  Created on: Jul 1, 2019
 *      Author: mvogel
 */


#include <ontrac/stream/datatap.h>

#ifndef MAX_LIMITED_TAPS
// Expected use is per http-request / http-response
# ifdef MAX_HTTP_REQUESTS
#define MAX_LIMITED_TAPS (MAX_HTTP_REQUESTS)
# else
#define MAX_LIMITED_TAPS (16U)
# endif /* MAX_HTTP_REQUESTS */
#endif /* MAX_LIMITED_TAPS */

typedef struct for_limited_tap limited_tap_t;
struct for_limited_tap {
    /** Lower-layer data source */
    datatap_t *rawtap;
    /** Upper-layer for consumption by application */
    datatap_t *limited;
    /** Total amount of data pass through this datatap_t */
    size_t bytecount;
    /** Amount of data still yet to be read by the consumer */
    size_t remaining;
    /** Flag used to indicate if rawtap should be closed when limited tap is done */
    bool_t close_when_done;
};

STATIC_ITEM_POOL(limited_tap_t, MAX_LIMITED_TAPS, limited_taps_pool);

static void limited_tap_datareader(datatap_t * datatap, cvar_t cbdata);

static err_t limited_tap_close(datatap_t *datatap) {
    limited_tap_t *limiter = NULL;
    ITEM_POOL_RESOLVE(&limited_taps_pool, limited_tap_t, limiter, datatap->handle);
    VITAL_NOT_NULL(limiter);
    datatap->close = NULL;
    datatap->handle = NULL;
    VITAL_IS_OK(datatap_remove_listener(datatap,
            limited_tap_datareader, limiter));
    VITAL_IS_OK(datatap_remove_listener(limiter->rawtap,
            limited_tap_datareader, limiter));
    if(limiter->close_when_done) {
        EXPECT_IS_OK(datatap_close(limiter->rawtap));
    }
    datatap->flags &= ~ABQ_IO_READ; // Can't read more data
    if (0UL == limiter->remaining) {
        // Read all data
        datatap->flags |= ABQ_IO_STOP;
    }
    // First publish closed event to upper layer
    datatap_publish(datatap, ABQ_IO_CLOSED); // MUST set ABQ_IO_CLOSED flag
    // Do NOT close rawtap (lower layer stream)
    // Finally release all resources
    EXPECT_IS_OK(obj_release(limiter->rawtap, datatap));
    EXPECT_IS_OK(obj_release(datatap, limiter->rawtap));
    ITEM_POOL_FREE(&limited_taps_pool, limited_tap_t, limiter);
    return EXIT_SUCCESS;
}

static int64_t limited_tap_read (datatap_t *datatap, void *dest, int64_t nbyte) {
    limited_tap_t *limiter = NULL;
    VITAL_NOT_NULL(datatap);
    ITEM_POOL_RESOLVE(&limited_taps_pool, limited_tap_t, limiter, datatap->handle);
    VITAL_NOT_NULL(limiter);
    int64_t amount = -1L;
    if (0UL == limiter->remaining) {
        datatap_publish(datatap, ABQ_IO_STOP);
        if (NULL != datatap->handle) {
            EXPECT_IS_OK(limited_tap_close(datatap));
        }
        (void) abq_status_set(ECANCELED, true);
    } else if(0L == nbyte) {
        amount = 0L;
    } else {
        amount = (int64_t) limiter->remaining;
        if((0L <= nbyte) && (nbyte < amount)) {
            amount = nbyte;
        }
        // First attempt to read data from cache if any
        if(buf_has_data(limiter->rawtap->cache)) {
            amount = (int64_t) buf_read_bytes(limiter->rawtap->cache, dest, (size_t)amount);
            ABQ_VITAL(0 < amount);
        } else {
            amount = datatap_read(limiter->rawtap, dest, amount);
        }
        if (0 < amount) {
            // ABQ_TRACE_MSG_X("amount:", amount)
            // ABQ_TRACE_MSG_X("remaining:", limiter->remaining)
            // ABQ_TRACE_MSG_Y("data:", dest, amount)
            ABQ_VITAL(limiter->remaining >= (size_t) amount);
            limiter->remaining -= (size_t) amount;
        }
    }
    return amount;
}

static void limited_tap_datareader(datatap_t * datatap, cvar_t cbdata) { /* parasoft-suppress MISRAC2012-RULE_8_13-a-4 "m0019. Not using 'const' specifier to adhere to callback type." */
    limited_tap_t *limiter = NULL;
    ITEM_POOL_RESOLVE(&limited_taps_pool, limited_tap_t, limiter, cbdata);
    VITAL_NOT_NULL(limiter);
    if (abq_io_pipe_check(datatap->flags)) {
        EXPECT_IS_OK(limited_tap_close(limiter->limited));
    } else if((NULL == limiter->limited)
            || (abq_io_is_closed(limiter->limited->flags))){
        VITAL_IS_OK(datatap_remove_listener(limiter->rawtap,
                limited_tap_datareader, cbdata));
    } else {
        ABQ_EXPECT(datatap == limiter->rawtap);
        datatap_publish(limiter->limited, datatap->flags);
    }
}

datatap_t *limited_tap_create(datatap_t *rawtap,
        size_t amount, bool_t close_when_done,
        datatap_event_callback_t on_completed, cvar_t ctx) {
    datatap_t *retval = NULL;
    err_t status = datatap_get_status(rawtap);
    if (EXIT_SUCCESS != status) {
        (void) abq_status_set(status, false);
    } else {
        byte_buffer_t *cache = buf_allocate(BUFFER_SMALL);
        if (NULL == cache) {
            abq_status_set(ENOMEM, false);
        } else {
            limited_tap_t *limiter = NULL;
            ITEM_POOL_ALLOC(&limited_taps_pool, limited_tap_t, limiter);
            if (NULL == limiter) {
                (void) abq_status_set(ENOMEM, false);
            } else {
                retval = datatap_create(NULL, limited_tap_read,
                        limited_tap_close, cache, limiter);
                if (NULL == retval) {
                    status = ENOMEM;
                } else {
                    // Explicit looped reference until datatap_t is closed
                    (void) obj_reserve(retval, rawtap);
                    (void) obj_reserve(rawtap, retval);
                    *limiter = (limited_tap_t) {
                            .rawtap = rawtap,
                            .limited = retval,
                            .bytecount = amount,
                            .remaining = amount,
                            .close_when_done = close_when_done
                    };
                    status = datatap_add_pipelistener(retval,
                            limited_tap_datareader, limiter);
                    if(EXIT_SUCCESS == status) {
                        status = datatap_set_combolistener(rawtap,
                                  limited_tap_datareader, limiter);
                        if((EXIT_SUCCESS == status) && (NULL != on_completed)) {
                            status = datatap_add_pipelistener(retval, on_completed, ctx);
                        }
                    }
                }
                if (EXIT_SUCCESS != status) {
                    (void) abq_status_set(status, true);
                    EXPECT_IS_OK(datatap_remove_listener(rawtap,
                                limited_tap_datareader, limiter));
                    (void) obj_release(retval, rawtap);
                    (void) obj_release(ctx, rawtap);
                    ITEM_POOL_FREE(&limited_taps_pool, limited_tap_t, limiter);
                    (void) obj_release_self(retval);
                    retval = NULL;
                } else if(0UL == amount) {
                    // Will close the datatap_t when next read is attempted
                    retval->flags |= ABQ_IO_STOP;
                } else if(buf_has_data(rawtap->cache)) {
                    // Cached data is available for reading
                    retval->flags |= ABQ_IO_READ;
                } else {
                    // setup to read incoming data
                }
            }
            VITAL_IS_OK(obj_release_self(cache));
        }
    }
    return retval;
}

size_t limited_tap_remaining(const datatap_t *limited) {
    size_t retval = 0UL;
    if ((NULL != limited) && (NULL != limited->handle)) {
        limited_tap_t *limiter = NULL;
        ITEM_POOL_RESOLVE(&limited_taps_pool, limited_tap_t, limiter, limited->handle);
        VITAL_NOT_NULL(limiter);
        retval = limiter->remaining;
    }
    return retval;

}
