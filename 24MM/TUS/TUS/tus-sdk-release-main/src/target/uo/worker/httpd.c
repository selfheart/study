/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

//
#include <libwebsockets.h>

#include "ctx.h"
#include "logger.h"
#include "tup_parser.h"
#include "worker/httpd.h"

struct httpd_state {
    char session[MAX_SESSION_ID_BYTES + 1];
    void *tup_parser;
    const struct httpd_config *config;
};

static int
check_sock_for_read(int fd, int *has_hup)
{
    // note: ppoll() & epoll() are Linux-specific
    int milliseconds = 0;
    struct pollfd fds = {
        .fd = fd,
        .events = POLLIN | POLLHUP | POLLERR,
    };
    if (has_hup)
        *has_hup = 0;
    int nfd = poll(&fds, 1, milliseconds);
    if (nfd > 0) {
        // has events
        if (fds.revents & POLLHUP) {
            LOG_WARN("POLLHUP for %d", fd);
            if (has_hup)
                *has_hup = 1;
        }
        if (fds.revents & POLLERR) {
            LOG_ERROR("POLLERR for %d", fd);
            if (has_hup)
                *has_hup = 1;
        }
        if (fds.revents & POLLIN) {
            return 1; // recvmsg() will not block
        }
        return 0; // no data to read
    }
    if (nfd == -1) {
        perror("poll");
        if (has_hup)
            *has_hup = 1;
    }
    return 0; // read will block or fail
}

static void
drop_tup_parser(struct httpd_state *hs, int has_stream)
{
    if (has_stream) {
        // assert(stream_param == param of parser's stream))
        // ToDo: check ref count?
        tup_detach_stream(hs->tup_parser);
    }
    if (tup_destroy_parser((struct tup_parser **)(&(hs->tup_parser)))) {
        LOG_ERROR("failed to tup_destroy_parser(%p)", hs->tup_parser);
        // not need to keep broken parsers
        hs->tup_parser = NULL;
    }
}

static int match_prefix(const char **p, const char *prefix);

static int
set_tup_parser(struct httpd_state *hs, const char *url)
{
    tup_parser *parser = NULL;
    int err = tup_create_parser(&parser);
    if (err) {
        LOG_ERROR("failed to tup_create_parser(%d)", err);
        return err;
    }
    if (match_prefix(&url, "file://")) {
        struct tup_file_stream file_stream = {
            .path = url,
        };
        struct tup_stream_param params = {
            .type = TUP_STREAM_FILE,
            .stream = { .file = &file_stream },
        };
        LOG_DEBUG("TUP_STREAM_FILE ((%d)", params.type);
        err = tup_attach_stream(parser, &params);
    } else {
        struct tup_net_stream net_stream = {
            .url = url,
        };
        struct tup_stream_param params = {
            .type = TUP_STREAM_NET,
            .stream = { .net = &net_stream },
        };
        LOG_DEBUG("TUP_STREAM_NET ((%d)", params.type);
        err = tup_attach_stream(parser, &params);
    }

    int has_stream = 0;
    if (err) {
        LOG_ERROR("failed to attach TUP_STREAM_NET '%s'", url);
        drop_tup_parser(hs, has_stream);
        return err;
    }
    has_stream = 1;

    int validate_err = tup_validate(parser);
    if (validate_err) {
        LOG_ERROR("failed to validate '%s' (%d)", url, validate_err);
        drop_tup_parser(hs, 1);
    } else {
        LOG_INFO("new TUP parser %p for '%s' (%d)", parser, url, validate_err);
        if (hs->tup_parser) {
            // drop old one
            LOG_ERROR("already has a TUP parser as %p, drop", hs->tup_parser);
            drop_tup_parser(hs, has_stream);
        }
        hs->tup_parser = parser;
    }
    return err;
}

static int
handle_ipc(struct worker *wk, struct httpd_state *hs)
{
    int err;

    enum ipc_code code = IPC_CODE__RESERVED;
    int arg = 0;
    char buf[IPC_BUF_SIZE];
    size_t actual = 0;

    err = worker_recv(wk, &code, &arg, buf, sizeof(buf) - 1, &actual);
    if (err) {
        return err;
    }
    buf[actual] = '\0';
    switch (code) {
    case IPC_CODE_TERMINATE:
        LOG_INFO("got IPC_CODE_TERMINATE (code=%#x, '%s')", code, buf);
        wk->state = WORKER_STATE_SHUTDOWN;
        break;

    case IPC_CODE_PING:
        LOG_DEBUG("got IPC_CODE_PING (code=%#x, %#x, '%s')", code, arg, buf);
        wk->ppid = arg;
        err = worker_notify(wk, code, getpid(), buf, actual);
        break;

    case IPC_CODE_HTTPD_SET_TUP_URL:
        LOG_DEBUG("got IPC_CODE_HTTPD_SET_TUP_URL: '%s'", buf);
        if (buf[0]) {
            // got a URL, construct a new parser
            err = set_tup_parser(hs, buf);
            if (!err) {
                err = worker_notify(wk, code, arg, NULL, 0);
            } else {
                // todo: report an failure in better way?
                err = worker_notify(wk, code, -1, NULL, 0);
            }
        } else {
            if (hs->tup_parser) {
                LOG_INFO("release TUP parser %p", hs->tup_parser);
                drop_tup_parser(hs, 1);
            } else {
                LOG_WARN("cannot release %p", hs->tup_parser);
            }
            err = worker_notify(wk, code, arg, NULL, 0);
        }
        break;

    default:
        LOG_WARN("unexpected code %d", code);
        break;
    }
    return err;
}

static int
loop(struct worker *wk, struct lws_context *context)
{
    int err = 0;
    struct httpd_state *hs = lws_context_user(context);

    LOG_DEBUG("LWS context=%p, hs=%p", context, hs);

    while (wk->state == WORKER_STATE_RUNNING) {
        int may_read, shall_halt;
        may_read = check_sock_for_read(wk->sock, &shall_halt);
        // LOG_DEBUG("may_read=%d, shall_halt=%d", may_read, shall_halt);
        if (may_read) {
            err = handle_ipc(wk, hs);
            if (err) {
                wk->state = WORKER_STATE_SHUTDOWN;
                break;
            }
        } else {
            if (shall_halt) {
                wk->state = WORKER_STATE_SHUTDOWN;
            }
        }
        if (wk->state != WORKER_STATE_RUNNING)
            break;

        // ToDo: avoid sleeping in lws_service()?
        int n = lws_service(context, 0);
        if (n < 0) {
            LOG_WARN("lws_service() -> %d", n);
            break;
        } else {
            LOG_HIDDEN("lws_service() -> %d", n);
        }
    }

    return err;
}

struct per_session {
    enum {
        PSSREQ_BAD = 0,
        PSSREQ_TUP_DATA = 1, // inner package
        PSSREQ_TUP_UPDATEFLOW,
        PSSREQ_TUP_INNER_DATA, // data#N from inner-package#[ipkg_idx]
    } request;

    // HTTP_STATUS_*
    int status;

    // WSI_TOKEN_(POST|DELETE|GET|PUT)_URI /
    int method;

    const char *mime_type;
    char msg[32]; // for error pages

    // ipkg index is counted from 1 (0: outer, 1: ipkg#1, ...)
    // note: to be adjusted by -1 for libtup's ipkg-index
    int ipkg_index;
    tup_parser *inner_parser; // derived from main parser's ipkg_index-th pkg
    int inner_index;

    ssize_t full_size;

    // data range to send for a client
    // note:
    // - 'tail' may be smaller than 'full_size' for ranged requests
    // - range in HTTP response wil use 'tail-1'
    ssize_t current;
    ssize_t tail;

    int no_more_write;
    tup_tlv *tlv;

    struct lws_spa *spa; // stateful post arguments
};

// ToDo: dummy param definitions
static const char *param_names[2] = {
    "param1", "param2",
    /* rests are for arbitrary names */
};

static int
file_upload_cb(void *data, const char *name, const char *filename, char *buf,
  int len, enum lws_spa_fileupload_states state)
{
    (void)data;
    (void)name;
    (void)filename;
    // struct per_session *pss = (struct per_session *)data;
    switch (state) {
    case LWS_UFS_CONTENT:
        LOG_DEBUG("LWS_UFS_CONTENT: '%s' (%d)", buf, len);
        break;
    case LWS_UFS_FINAL_CONTENT:
        LOG_DEBUG("LWS_UFS_FINAL_CONTENT: '%s' (%d)", buf, len);
        break;
    case LWS_UFS_OPEN:
        LOG_DEBUG("LWS_UFS_OPEN: '%s' (%d)", buf, len);
        break;
    case LWS_UFS_CLOSE:
        LOG_DEBUG("LWS_UFS_CLOSE: '%s' (%d)", buf, len);
        break;
    }
    // should return -1 in case of fatal error, and 0 if OK.
    return 0;
}

static int
match_prefix(const char **p, const char *prefix)
{
    const char *cur = *p;
    if (*cur == '/' && *prefix == '/') {
        // compact "[/]+" to '/'
        while (*cur == '/')
            cur++;
        while (*prefix == '/')
            prefix++;
        LOG_HIDDEN("<%s, %s>", cur, prefix);
    }
    size_t len = strlen(prefix);
    if (strncmp(cur, prefix, len) == 0) {
        *p = cur + len;
        LOG_DEBUG("cur='%s', matched '%s'", cur, prefix);
        return 1;
    }
    LOG_HIDDEN("cur='%s', not matched '%s'", cur, prefix);
    return 0;
}

static int
match_number(const char **p, int base, long *result)
{
    const char *cur = *p;

    char *end;
    long ret = strtol(cur, &end, base);
    if (cur != end) {
        // some characters have been consumed
        LOG_DEBUG("cur='%s', matched = %ld", cur, ret);
        *p = end;
        *result = ret;
        return 1;
    }
    LOG_HIDDEN("cur='%s', not matched NNNN", cur);
    return 0;
}

static tup_tlv *
get_tlv_for_script(struct tup_parser *parser, int ipkg_index, ssize_t *size)
{
    uint64_t value_size;
    int terr;
    tup_get_tlv_param param = {};

    if (ipkg_index > 0) {
        terr = tup_get_inner_package_info(parser, ipkg_index - 1, &(param.tlv));
        if (terr) {
            LOG_ERROR("ipkg_index (%d-1) -> terr %d", ipkg_index, terr);
            return NULL;
        }
        // found PKG_INFO, set the tlv as parent_tlv to use for UPDATEFLOW
        assert(param.tlv != NULL);
        param.parent_tlv = param.tlv;
        param.tlv = NULL;
        LOG_HIDDEN("tup_get_inner_package_info(%d)-> parent %p", ipkg_index,
          param.parent_tlv);
    }

    terr = tup_find_tlv(parser, TUP_ID_UPDATEFLOW, &param);
    if (terr) {
        LOG_ERROR("tup_find_tlv(%d) -> terr %d", ipkg_index, terr);
        return NULL;
    }
    LOG_DEBUG("tup_get_find_tlv(TUP_ID_UPDATEFLOW)->%p", param.tlv);

    terr = tup_get_tlv_value_size(param.tlv, &value_size);
    if (terr) {
        LOG_ERROR("tup_get_tlv_value_size(%p, %d) -> terr %d", param.tlv,
          ipkg_index, terr);
        return NULL;
    }
    if (value_size > SSIZE_MAX) {
        LOG_ERROR("tup_get_tlv_value_size(%d) > SSIZE_MAX", ipkg_index);
        return NULL;
    }
    *size = value_size;
    return param.tlv;
}

static int
set_inner_parser(struct httpd_state *hs, struct per_session *pss)
{
    if (pss->inner_parser) {
        LOG_DEBUG("already has a inner parser %p", pss->inner_parser);
        return EINVAL;
    }

    if (hs->tup_parser) {
        LOG_DEBUG("create a paser for inner TUP#%d of %p", pss->ipkg_index,
          hs->tup_parser);
    } else {
        return EINVAL;
    }

    tup_parser *parser = NULL;
    int err = tup_create_parser(&parser);
    if (err) {
        LOG_ERROR("failed to tup_create_parser(%d)", err);
        return err;
    }
    struct tup_loop_stream stream = {
        .parent = hs->tup_parser,
        .index = pss->ipkg_index - 1, // .index starts from 0
    };
    struct tup_stream_param params = {
        .type = TUP_STREAM_LOOP,
        .stream = { .loop = &stream },
    };
    LOG_DEBUG("TUP_STREAM_LOOP (%d)", params.type);
    err = tup_attach_stream(parser, &params);
    if (err) {
        LOG_ERROR("failed to attach TUP_STREAM_LOOP for '%p'", stream.parent);
        tup_destroy_parser(&parser);
        return err;
    }

    int validate_err = tup_validate(parser);
    if (validate_err) {
        LOG_ERROR("failed to validate (%d)", validate_err);
        tup_detach_stream(hs->tup_parser);
        tup_destroy_parser(&parser);
        return validate_err;
    }
    LOG_INFO("new TUP parser %p for '%p' #%d", parser, stream.parent,
      stream.index);
    pss->inner_parser = parser;
    return 0;
}

/*
 /tus/tup/
  - meta/
    - updateflow
  - ipkgX/ : inner pacakged for X = 1, 2...
    - data : raw inner package data (like inner TUP itself)
    - meta/
      - updateflow
    - tup/ : 'data' parsed as a TUP (if the ipkg is actially a TUP)
      - ipkgY/
        - data : data for Y
 */
static int
classify_path(struct httpd_state *hs, const char *path, struct per_session *pss)
{
    const char *cur = path;

    pss->request = PSSREQ_BAD;
    pss->status = HTTP_STATUS_BAD_REQUEST;
    pss->tlv = NULL;
    pss->full_size = LWS_ILLEGAL_HTTP_CONTENT_LEN; // not known yet
    pss->mime_type = "application/json";           // ToDo

    if (match_prefix(&cur, "/tup")) {
        /* SDK 020 '/tus/tup/... */
    } else {
        // used to serve on SDK010:
        // http://192.168.0.2:80/tus/0/ipkg1/metadata
        // (http://192.168.0.2:80/tus/0/ipkg1/tup) #exported, but not used
        // http://192.168.0.2:80/tus/0/ipkg1/data
        while (*cur == '/')
            cur++;
        cur = strchr(cur, '/');
        if (cur) {
            LOG_WARN("interpret '%s' as /tup%s", path, cur);
            // assume "tup" has been requested
        } else {
            LOG_WARN("failed to interpret '%s'", path);
            return 0;
        }
    }

    // check whether a TUP is registered for HTTPd.
    if (hs->tup_parser) {
        // todo: add refcount?
        LOG_DEBUG("serve TUP for '%s' by hs->tup_parser = %p", path,
          hs->tup_parser);
    } else {
        LOG_DEBUG("missing hs->tup_parser for %s", path);
        return 0; // no TUP to feed
    }

    if (match_prefix(&cur, "/data")) {
#if 0 // cannot get raw data from a TUP parser nor now
        pss->status = HTTP_STATUS_OK;
        pss->request = PSSREQ_TUP_DATA;
        pss->mime_type = "application/octet-stream";
        LOG_INFO("top TUP data (%d)", pss->request);
        return 1; // may serve
#else
        return 0;
#endif
    }
    if (match_prefix(&cur, "/meta")) {
        // hrequested outer-tup's metadata
        if (match_prefix(&cur, "/updateflow")) {
            pss->tlv = get_tlv_for_script(hs->tup_parser, 0, &(pss->full_size));
            if (!pss->tlv) {
                pss->status = HTTP_STATUS_SERVICE_UNAVAILABLE;
                return 0;
            }
            pss->status = HTTP_STATUS_OK;
            pss->request = PSSREQ_TUP_UPDATEFLOW;
            pss->mime_type = "text/plain"; // ToDo: actually a blob?
            LOG_INFO("top TUP script (%d)", pss->request);
            return 1; // may serve
        }
        LOG_WARN("unknown metadata name '%p'", cur);
        pss->status = HTTP_STATUS_NOT_FOUND;
        return 0;
    }

    if (match_prefix(&cur, "/ipkg")) {
        long n;
        if (match_number(&cur, 10, &n)) {
            uint32_t count = 0;
            if (tup_get_inner_package_count(hs->tup_parser, &count)) {
                LOG_DEBUG("tup_get_inner_package_count() failed(%p)",
                  hs->tup_parser);
                pss->status = HTTP_STATUS_SERVICE_UNAVAILABLE; // bad ipkgN
                return 0;
            }
            if ((0 < n) && (n <= count)) {
                int idx = n;
                pss->ipkg_index = idx;
                LOG_INFO("IPKG#%d of %d", idx, count);
                if (match_prefix(&cur, "/data")) {
                    tup_get_range range = {};
                    if (tup_get_inner_package_size(hs->tup_parser, (idx - 1),
                          &range)) {
                        LOG_DEBUG("tup_get_inner_package_size() failed(%p)",
                          hs->tup_parser);
                        pss->status = HTTP_STATUS_SERVICE_UNAVAILABLE;
                        return 0;
                    }
                    pss->full_size = range.raw_size; // decompressed size
                    pss->status = HTTP_STATUS_OK;
                    pss->request = PSSREQ_TUP_DATA;
                    pss->mime_type = "application/octet-stream";
                    LOG_INFO("ipkg data (%d) full_size=%zd", pss->request,
                      pss->full_size);
                    return 1;
                }
                if (match_prefix(&cur, "/meta")) {
                    if (match_prefix(&cur, "/updateflow")) {
                        pss->tlv = get_tlv_for_script(hs->tup_parser, idx,
                          &(pss->full_size));
                        if (!pss->tlv) {
                            return 0;
                        }
                        pss->request = PSSREQ_TUP_UPDATEFLOW;
                        pss->mime_type = "text/plain"; // ToDo: actually a blob?
                        LOG_INFO("ipkg %d TUP script (%d)", idx, pss->request);
                        return 1;
                    }
                    LOG_WARN("unknown metadata name '%p'", cur);
                }

                if (match_prefix(&cur, "/tup")) {
                    // "ipkgX/tup/...", interpret inner pkg #idx as a TUP
                    if (set_inner_parser(hs, pss)) {
                        LOG_WARN("cannot parse ipkg#%d as a TUP?",
                          pss->ipkg_index);
                        return 1;
                    } else {
                        assert(pss->inner_parser != NULL);
                    }
                    if (match_prefix(&cur, "/ipkg")) {
                        // going to serve ipkgX/tup/ipkgY/data
                        long n;
                        if (match_number(&cur, 10, &n)) {
                            int inner_index = n;
                            if ((inner_index < 0) || (n != (long)inner_index)) {
                                LOG_WARN("invalid inner_index %ld", n);
                                return 0;
                            }
                            tup_get_range range = {};
                            if (tup_get_inner_package_size(pss->inner_parser,
                                  (inner_index - 1), &range)) {
                                LOG_DEBUG(
                                  "tup_get_inner_package_size() failed(%p)",
                                  pss->inner_parser);
                                pss->status = HTTP_STATUS_SERVICE_UNAVAILABLE;
                                return 0;
                            }
                            pss->full_size =
                              range.raw_size; // decompressed size
                            pss->status = HTTP_STATUS_OK;
                            pss->request = PSSREQ_TUP_INNER_DATA;
                            pss->inner_index = inner_index;
                            pss->mime_type = "application/octet-stream";
                            LOG_INFO("ipkg data (%d) full_size=%zd",
                              pss->request, pss->full_size);
                            return 1;
                        }
                    }
                    LOG_WARN("unknown metadata name '%p'", cur);
                }
            } else {
                LOG_WARN("bad index %ld?", n);
                pss->status = HTTP_STATUS_SERVICE_UNAVAILABLE;
                return 0;
            }
        }
    }
    pss->status = HTTP_STATUS_NOT_FOUND;
    return 0;
}

static int
guess_method_from_lws(struct lws *wsi, char *buf, size_t limit)
{
    // todo: assumed to have LWS_WITH_HTTP_UNCOMMON_HEADERS
    memset(buf, 0, limit);
#define HAS_WSI_TOKEN(NAME)                         \
    do {                                            \
        int t = WSI_TOKEN_##NAME##_URI;             \
        if (lws_hdr_copy(wsi, buf, limit, t) > 0) { \
            LOG_DEBUG(#NAME "='%s'", buf);          \
            return t;                               \
        }                                           \
    } while (0)

    HAS_WSI_TOKEN(GET);
    HAS_WSI_TOKEN(POST);
    HAS_WSI_TOKEN(PUT);
    HAS_WSI_TOKEN(HEAD);
    HAS_WSI_TOKEN(DELETE);
#undef HAS_WSI_TOKEN
    return -1;
}

// hand-made range support as LWS_WITH_RANGE tends to be off by default
static int
guess_range_from_lws(struct lws *wsi, char *buf, size_t limit, long *head,
  long *tail)
{
    memset(buf, 0, limit);
    *head = 0;
    *tail = -1;

    if (lws_hdr_copy(wsi, buf, limit, WSI_TOKEN_HTTP_RANGE) <= 0) {
        LOG_DEBUG("not a ranged request %s", buf);
        return 0;
    }
    LOG_DEBUG("RANGE='%s'", buf);
    const char *p = buf;

    if (!match_prefix(&p, "bytes=")) {
        LOG_WARN("non-bytes range? %s", p);
        return -1;
    }

    long n;
    if (*p == '-') {
        // 'bytes=-N' : 'last N bytes'
        p++;
        if (match_number(&p, 10, &n)) {
            *tail = n;
            LOG_DEBUG("(h,t)=(%ld, %ld)", *head, *tail);
            return 1;
        } else {
            LOG_ERROR("expected a number '%s'", p);
            return -1;
        }
    }

    if (match_number(&p, 10, &n)) {
        *head = n;
        if (*p == '\0') {
            // 'bytes=N' : 'starting from N'
            LOG_DEBUG("(h,t)=(%ld, %ld)", *head, *tail);
            return 1;
        }
        if (*p == '-') {
            p++;
            if (*p == '\0') {
                // 'bytes=N-' : 'starting from N'
                LOG_DEBUG("(h,t)=(%ld, %ld)", *head, *tail);
                return 1;
            }
            if (match_number(&p, 10, &n)) {
                // 'bytes=N-M'
                *tail = n;
                LOG_DEBUG("(h,t)=(%ld, %ld)", *head, *tail);
                return 1;
            }
        } else {
            LOG_ERROR("expected N-M, got '%s'", p);
            return -1;
        }
    }
    LOG_ERROR("expected a number '%s'", p);
    return -1;
}

static int
http_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user,
  void *in, size_t len)
{
    struct per_session *pss = (struct per_session *)user;

    /* for HTTP/2, ensure LWS_PRE bytes are writable before body */
    char buf[LWS_PRE + 2048] = {};

    // lws_ expects 'unsigned char'
    char *start = (&buf[LWS_PRE]);
    char *p = start;
    char *end = (&buf[sizeof(buf) - 1]);
    char *prefix = "?";

    switch (reason) {
    case LWS_CALLBACK_CLOSED_HTTP:
        // 5
        {
            prefix = NULL; // "[CLOSED_HTTP]";
            break;
        }
    case LWS_CALLBACK_HTTP:
        // == 12
        {
            prefix = "[HTTP]";
            const char *relpath = in;
            LOG_DEBUG("%s: relpath='%s' (%p)", prefix, relpath, relpath);

            lws_get_peer_simple(wsi, buf, sizeof(buf) - 1);
            LOG_DEBUG("%s: connected from '%s'", prefix, buf);

            memset(pss, 0, sizeof(*pss));
            pss->method = guess_method_from_lws(wsi, buf, sizeof(buf) - 1);
            LOG_DEBUG("pss->method=%d", pss->method);

            struct httpd_state *hs = lws_context_user(lws_get_context(wsi));
            LOG_HIDDEN("%s: hs=%p, hs->tup_parser=%p", prefix, hs,
              hs->tup_parser);

            if (classify_path(hs, relpath, pss) <= 0) {
                LOG_WARN("failed to interpret '%s'", relpath);
                return 1;
            }

            long content_len = pss->full_size;
            // send all of content_len by default
            pss->current = 0;
            pss->tail = content_len;
            // clip pss->current and pss->tail if 'range:' was given
            // note: cannot process ranged requests if pss->full_size == -1
            if (pss->status == HTTP_STATUS_OK && (pss->full_size > 0)) {
                long r_head, r_tail;
                int ranged = guess_range_from_lws(wsi, buf, sizeof(buf) - 1,
                  &r_head, &r_tail);
                if (ranged > 0) {
                    if ((r_tail < 0) || (pss->full_size <= r_tail)) {
                        r_tail = pss->full_size - 1;
                    }
                    if (0 < r_head || r_tail < pss->full_size - 1) {
                        LOG_DEBUG("PARTIAL_CONTENT: (%ld-%ld)/%ld", r_head,
                          r_tail, pss->full_size);
                        // report 206
                        pss->status = HTTP_STATUS_PARTIAL_CONTENT;
                        pss->current = r_head;
                        // value for range headers are inclusive
                        pss->tail = r_tail + 1;
                        content_len = pss->tail - pss->current;
                    } else {
                        LOG_DEBUG(" not PARTIAL: (%ld-%ld)/%ld", r_head, r_tail,
                          pss->full_size);
                    }
                } else if (ranged < 0) {
                    // report 416
                    pss->status = HTTP_STATUS_REQ_RANGE_NOT_SATISFIABLE;
                    LOG_DEBUG(" REQ_RANGE_NOT_SATISFIABLE: (%d)", ranged);
                } else {
                    // not a ranged request
                }
            } else {
                // will feed a error page, attach no size info
            }
            LOG_DEBUG("HTTP status=%d, mime_type=%s", pss->status,
              pss->mime_type);

            unsigned char header[512] = {};
            unsigned char *head = header;
            unsigned char *tail = header + sizeof(header) - 1;
            int lw_err;

            lw_err = lws_add_http_common_headers(wsi, pss->status,
              pss->mime_type, content_len, &head, tail);
            if (lw_err == 0) {
                // Accept-Ranges: bytes
                lw_err = lws_add_http_header_by_token(wsi,
                  WSI_TOKEN_HTTP_ACCEPT_RANGES, (unsigned char *)"bytes", 5,
                  &head, tail);
                if (lw_err) {
                    LOG_ERROR("failed to claim accept_range? %d", lw_err);
                }

                if (pss->status == HTTP_STATUS_PARTIAL_CONTENT) {
                    char range_str[64] = {};
                    snprintf(range_str, sizeof(range_str) - 1,
                      "bytes %ld-%ld/%ld", pss->current, pss->tail - 1,
                      pss->full_size);
                    lw_err = lws_add_http_header_by_token(wsi,
                      WSI_TOKEN_HTTP_CONTENT_RANGE, (unsigned char *)range_str,
                      strlen(range_str), &head, tail);
                    if (lw_err) {
                        LOG_ERROR("%s: failed to set content-range() failed %d",
                          prefix, lw_err);
                        return 1;
                    }
                }
                lw_err = lws_finalize_write_http_header(wsi, header, &head,
                  tail);
                if (lw_err) {
                    LOG_ERROR("%s: lws_finalize_write_http_header() failed %d",
                      prefix, lw_err);
                    return 1;
                }
            } else {
                LOG_ERROR("%s: lws_add_http_common_headers() failed %d", prefix,
                  lw_err);
                return 1;
            }
            // successfully made a response header, prepare the body */
            lws_callback_on_writable(wsi);
            return 0; // done
        }
    case LWS_CALLBACK_HTTP_BODY:
        // == 13
        //
        // curl -d "param1=value1&param2=value2" -X POST
        // http://localhost:7681/tus/1

        {
            prefix = "[HTTP_BODY]";
            LOG_DEBUG("%s: in=%p", prefix, in);

            if (!pss->spa) {
                pss->spa = lws_spa_create(wsi, param_names,
                  LWS_ARRAY_SIZE(param_names),
                  2048, // max storage
                  file_upload_cb,
                  pss // 'data' for file_upload_cb
                );
                LOG_DEBUG("%s: made spa=%p", prefix, pss->spa);
            } else {
                LOG_DEBUG("%s: already has spa %p?", prefix, pss->spa);
            }
            /* parse the POST */
            if (lws_spa_process(pss->spa, in, (int)len)) {
                return -1;
            }
            break;
        }
    case LWS_CALLBACK_HTTP_BODY_COMPLETION:
        // == 14
        {
            prefix = "[HTTP_BODY_COMPLETION]";
            LOG_DEBUG("%s: in=%p", prefix, in);

            if (pss->spa) {
                // let SPA know no more data
                LOG_DEBUG("%s: fini spa=%p", prefix, pss->spa);
                lws_spa_finalize(pss->spa);
            }
            for (size_t n = 0; n < LWS_ARRAY_SIZE(param_names); n++) {
                const char *s = lws_spa_get_string(pss->spa, n);
                if (s) {
                    LOG_DEBUG("\t %ld: (len %d) '%s'", n,
                      lws_spa_get_length(pss->spa, n), s);
                }
            }

            /* response body will be made by LWS_CALLBACK_HTTP_WRITEABLE */
            return 0;
        }
    case LWS_CALLBACK_HTTP_WRITEABLE:
        // == 16: you can write more down the http protocol link now
        {
            prefix = "[HTTP_WRITEABLE]";
            struct httpd_state *hs = lws_context_user(lws_get_context(wsi));

            if (!hs || !pss || pss->no_more_write) {
                LOG_DEBUG("%s: (missing states?)", prefix);
                break;
            }
            struct tup_parser *parser = hs->tup_parser;

            // max size of data for this chunk
            ssize_t chunk_size = lws_ptr_diff(end, p);
            if (pss->tail > pss->current) {
                if (pss->tail <= pss->current + chunk_size) {
                    chunk_size = pss->tail - pss->current;
                    pss->no_more_write = 1;
                }
            }

            switch (pss->request) {
            case PSSREQ_TUP_DATA: {
                LOG_HIDDEN("%s: send body", prefix);

                tup_get_ipkg_param params = {
                    .flags = (TUP_FLAG_DECRYPT | TUP_FLAG_DECOMPRESS),
                    .offset = pss->current,
                    .buffer_size = chunk_size,
                    .buffer = p,
                };
                int terr = tup_get_inner_package_data(parser,
                  (pss->ipkg_index - 1), &params);
                if (terr) {
                    LOG_ERROR("tup_get_inner_package_data(ipkg#%d) -> terr %d",
                      pss->ipkg_index, terr);
                    return 1;
                }
                LOG_DEBUG("tup_get_inner_package_data(ipkg#%d) -> %zd bytes",
                  pss->ipkg_index, chunk_size);
                p += chunk_size;
                pss->current += chunk_size;
                break;
            }
            case PSSREQ_TUP_INNER_DATA: {
                LOG_DEBUG("%s: send body", prefix);
                parser = pss->inner_parser;

                int idx = pss->inner_index - 1;
                assert(idx >= 0);

                tup_get_ipkg_param params = {
                    .flags = (TUP_FLAG_DECRYPT | TUP_FLAG_DECOMPRESS),
                    .offset = pss->current,
                    .buffer_size = chunk_size,
                    .buffer = p,
                };
                int terr = tup_get_inner_package_data(parser, idx, &params);
                if (terr) {
                    LOG_ERROR("tup_get_inner_package_data(ipkg#%d) -> terr %d",
                      pss->ipkg_index, terr);
                    return 1;
                }
                LOG_DEBUG("tup_get_inner_package_data(ipkg#%d) -> %zd bytes",
                  pss->ipkg_index, chunk_size);
                p += chunk_size;
                pss->current += chunk_size;
                break;
            }
            case PSSREQ_TUP_UPDATEFLOW: {
                if (!(pss->tlv)) {
                    LOG_ERROR("missing pss->tlv for ipkg %d", pss->ipkg_index);
                    return 1;
                }
                int terr = tup_get_tlv_value(pss->tlv, pss->current, p,
                  chunk_size);
                if (terr == 0) {
                    p += chunk_size;
                    pss->current += chunk_size;
                } else {
                    LOG_ERROR("tup_get_tlv_value() %d", terr);
                    return 1;
                }
                break;
            }
            case PSSREQ_BAD:
                LOG_DEBUG("%s: make error resp", prefix);
                p += lws_snprintf(p, lws_ptr_diff(end, p),
                  "{\"reason\":\"%s\"}\n", pss->msg);
                pss->no_more_write = 1;
                break;
            }
            const int prepared = lws_ptr_diff(p, start);
            if (prepared < 0) {
                LOG_ERROR("can feed nothing? %d", prepared);
                pss->no_more_write = 1;
                return 1;
            }
            enum lws_write_protocol wprot = (pss->no_more_write) ?
              LWS_WRITE_HTTP_FINAL :
              LWS_WRITE_HTTP;

            int n = lws_write(wsi, (uint8_t *)start, prepared, wprot);
            if (n != lws_ptr_diff(p, start)) {
                LOG_ERROR("tried to write %d, got %d", prepared, n);
                return 1;
            } else {
                LOG_HIDDEN("%d/%d bytes fed by lws_write()%s", n, prepared,
                  wprot == LWS_WRITE_HTTP_FINAL ? " [FIN]" : "");
            }

            if (wprot == LWS_WRITE_HTTP_FINAL) {
                // note: connection may be kept if keep-alive or HTTP/2
                if (lws_http_transaction_completed(wsi)) {
                    return -1;
                }
            } else {
                lws_callback_on_writable(wsi);
            }
            return 0;
        }
    case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
        // 17: called when a client connects to the server at network level
        // 'in' contains the connection socket's descriptor.
        {
            prefix = "[FILTER_NETWORK_CONNECTION]";
            LOG_DEBUG("%s: fd='%ld'", prefix, (intptr_t)in);

            // Return non-zero to terminate the connection immediately
            return 0;
        }
    case LWS_CALLBACK_FILTER_HTTP_CONNECTION:
        // 18:  request has been received and parsed from the client,
        // but the response is not sent yet
        {
            prefix = "[FILTER_HTTP_CONNECTION]";
            LOG_DEBUG("%s: URI='%s' (%p)", prefix, (char *)in, in);

            // Return non-zero to terminate the connection immediately
            return 0;
        }
    case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED:
        // 19:
        {
            prefix = NULL; // "[SERVER_NEW_CLIENT_INSTANTIATED]";
            break;
        }
    case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS:
        // 21
        {
            prefix = "[OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS]";
            break;
        }
    case LWS_CALLBACK_PROTOCOL_INIT:
        // 27:
        {
            prefix = "[PROTOCOL_INIT]";
            break;
        }
    case LWS_CALLBACK_PROTOCOL_DESTROY: {
        prefix = "[PROTOCOL_DESTROY]";
        break;
    }
    case LWS_CALLBACK_WSI_CREATE:
        // 29:
        {
            prefix = NULL; // "[WSI_CREATE]";
            break;
        }
    case LWS_CALLBACK_WSI_DESTROY:
        // 30
        {
            prefix = NULL; // "[WSI_DESTROY]";
            break;
        }
    case LWS_CALLBACK_GET_THREAD_ID:
        // 31:
        {
            prefix = NULL; // "[GET_THREAD_ID]";
            break;
        }
    case LWS_CALLBACK_ADD_POLL_FD:
        // 32: a socket needs to be added to the polling loop
        // 'in' points to a struct lws_pollargs
        {
            prefix = NULL; // "[ADD_POLL_FD]";
            break;
        }
    case LWS_CALLBACK_DEL_POLL_FD:
        // 33: socket descriptor needs to be removed from an external polling
        // array 'in' is again the struct lws_pollargs containing the fd member
        // to be removed
        {
            prefix = NULL; // "[DEL_POLL_FD]";
            break;
        }
    case LWS_CALLBACK_CHANGE_MODE_POLL_FD:
        // 34: lws wants to modify the events for a connection
        // 'in' is the struct lws_pollargs with the fd to change
        {
            prefix = NULL; // "[CHANGE_MODE_POLL_FD]";
            break;
        }
    case LWS_CALLBACK_LOCK_POLL:
        // 35:
        {
            prefix = NULL; // "[LOCK_POLL]";
            break;
        }
    case LWS_CALLBACK_UNLOCK_POLL:
        // 36:
        {
            prefix = NULL; // "[UNLOCK_POLL]";
            break;
        }
    case LWS_CALLBACK_HTTP_BIND_PROTOCOL:
        // 49
        {
            prefix = NULL; // "[HTTP_BIND_PROTOCOL]";
            break;
        }
    case LWS_CALLBACK_HTTP_DROP_PROTOCOL:
        // 50
        {
            prefix = "[HTTP_DROP_PROTOCOL]";
            if (pss->spa) {
                LOG_DEBUG("%s: drop SPA %p", prefix, pss->spa);
                lws_spa_destroy(pss->spa);
                pss->spa = NULL;
            }
            prefix = NULL;
            break;
        }
    case LWS_CALLBACK_CHECK_ACCESS_RIGHTS:
        // 51
        {
            prefix = "[CHECK_ACCESS_RIGHTS]";
            break;
        }
    case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
        // 71:
        {
            prefix = "[EVENT_WAIT_CANCELLED]";
            break;
        }
    default:
        break;
    }
    {
        int ret = lws_callback_http_dummy(wsi, reason, user, in, len);
        if (prefix) {
            LOG_DEBUG("%s(%d): return %d", prefix, reason, ret);
        }
        return ret;
    }
}

int
httpd_main(struct worker *wk, void *config)
{
    assert(wk->pid == 0); // to be called from a child process
    int err;
    struct httpd_state httpd_state = { .config = config };
    struct httpd_config *httpd_config = config;

    const char name[] = "http";
    const struct lws_protocols protocol = {
        .name = name,
        .callback = http_callback,
        .per_session_data_size = sizeof(struct per_session),
        .rx_buffer_size = 0,
        .id = 0,
        .user = NULL,
        .tx_packet_size = 0,
    };
    const struct lws_protocols *pprotocols[] = { &protocol, NULL };
    const struct lws_http_mount http_mount = {
        /* serve "HOST:PORT/[PATH_BASE]/..." */
        .mountpoint = httpd_config->path_base,
        .protocol = name,
        .origin_protocol = LWSMPRO_CALLBACK, /* -> http_callback() */
        .mountpoint_len = sizeof(name) - 1,
    };

    logger_set_prefix("H:%#x\t", getpid());
    LOG_DEBUG("started child process %#x, path_base=%s", getpid(),
      httpd_config->path_base);
    {
        struct lws_context_creation_info info = {
            .iface = NULL, // listen to all interfaces
            //. extensions = NULL; // for LWS_ROLE_WS
            .options = (LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT |
              LWS_SERVER_OPTION_EXPLICIT_VHOSTS |
              LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE),
            .port = httpd_config->port,
            .pprotocols = pprotocols,
            .mounts = &http_mount,
            .vhost_name = "http",
            .user = &httpd_state,
        };

        struct lws_context *context = lws_create_context(&info);
        if (!context) {
            lwsl_err("lws init failed\n");
            err = EINVAL; // FIXME
        } else {
            if (lws_create_vhost(context, &info)) {
                LOG_INFO("stared with port=%d", httpd_config->port);
                err = loop(wk, context);
            } else {
                LOG_ERROR("failed to init vhost '%s'?", info.vhost_name);
                err = EINVAL; // FIXME
            }
            lws_context_destroy(context);
        }
    }

    LOG_DEBUG("cleanup HTTPd states, err= %d", err);
    close(wk->sock); // let parent know the socket has been closed
    wk->sock = -1;
    wk->state = WORKER_STATE_NONE;

    return err;
}
