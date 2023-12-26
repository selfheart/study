/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#include <assert.h>
#include <curl/curl.h>
#include <errno.h>
#include <string.h>
#ifdef __QNX__
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif
#include <unistd.h>

#include "fetch.h"
#include "logger.h"

struct to_fd {
    int fd;
    size_t base;
};

struct to_buffer {
    void *buffer;
    size_t base;
    size_t limit;
};

static size_t
write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    struct to_fd *dst = (struct to_fd *)userdata;
    const int fd = dst->fd;
    assert(fd != -1);

    const size_t realsize = size * nmemb;
    LOG_DEBUG("fd=%d: %zu @ %zd\n", fd, realsize, dst->base);
    assert(realsize + dst->base >= dst->base);

    ssize_t ret = pwrite(fd, ptr, realsize, (off_t)(dst->base));
    if (ret < 0 || (size_t)ret != realsize) {
        // ToDO: use CURL_WRITEFUNC_PAUSE for ENOSPC?
        return CURLE_WRITE_ERROR;
    }
    dst->base += realsize;
    return ret;
}

static int
fetch_to_fd(CURL *c, const char *url, size_t head, off_t tail,
  struct to_fd *dst)
{
    CURLcode ret;
    char range[64] = {}; // "X-Y" requires around 20 + 1 + 20 bytes

    LOG_DEBUG("url='%s' range=(head %zd, tail %zd) -> fd %d, base=%zd\n", url,
      head, tail, dst->fd, dst->base);

    ret = curl_easy_setopt(c, CURLOPT_URL, url);
    if (ret != CURLE_OK)
        return -1;

    // ToDo: set CURLOPT_HEADERFUNCTION ?

    ret = curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, write_callback);
    if (ret != CURLE_OK)
        return -1;
    ret = curl_easy_setopt(c, CURLOPT_WRITEDATA, dst);
    if (ret != CURLE_OK)
        return -1;
    if (head != 0 || tail != -1) {
        if (tail == -1) {
            snprintf(range, sizeof(range) - 1, "%zu-", head);
        } else {
            // note: HTTP range is inclusive, so 'tail' needs '-1'
            // i.e. requesting '0--0' will return 1 byte.
            if ((tail < 0) || ((size_t)tail < head)) {
                return -1;
            }
            snprintf(range, sizeof(range) - 1, "%zu-%zu", head,
              (size_t)tail - 1);
        }
        LOG_DEBUG("set HTTP RANGE header: '%s'", range);
        ret = curl_easy_setopt(c, CURLOPT_RANGE, range);
        if (ret != CURLE_OK)
            return -1;
    }
    ret = curl_easy_perform(c);
    if (ret != CURLE_OK)
        return -1;

    return 0;
}

static size_t
write_buffer_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    struct to_buffer *dst = (struct to_buffer *)userdata;

    const size_t realsize = size * nmemb;
    LOG_DEBUG("to=%p: %zu @ %zd\n", dst->buffer, realsize, dst->base);
    assert(realsize + dst->base >= dst->base);
    if (realsize + dst->base > dst->limit)
        return CURLE_WRITE_ERROR;

    memcpy((char *)(dst->buffer) + dst->base, ptr, realsize);
    dst->base += realsize;
    return realsize;
}

static int
fetch_to_buffer(CURL *c, const char *url, struct to_buffer *dst)
{
    CURLcode ret;

    ret = curl_easy_setopt(c, CURLOPT_URL, url);
    if (ret != CURLE_OK)
        return -1;

    // ToDo: set CURLOPT_HEADERFUNCTION ?

    ret = curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, write_buffer_callback);
    if (ret != CURLE_OK)
        return -1;
    ret = curl_easy_setopt(c, CURLOPT_WRITEDATA, dst);
    if (ret != CURLE_OK)
        return -1;
    ret = curl_easy_perform(c);
    if (ret != CURLE_OK)
        return -1;

    return 0;
}

int
http_fetch_to_buffer(const char *url, void *buffer, size_t limit)
{
    int ret;
    CURL *c = curl_easy_init();
    if (c) {
        struct to_buffer dst = { .buffer = buffer, .base = 0, .limit = limit };
        ret = fetch_to_buffer(c, url, &dst);
        curl_easy_cleanup(c);
    } else {
        ret = EINVAL;
    }
    return ret;
}

int
http_init(void)
{
    CURLcode ret = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (ret != CURLE_OK) {
        LOG_ERROR("faield to initialized CURL %d\n", ret);
        return -1;
    }
    return (0);
}

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

static lua_Integer
get_optional_luaint(lua_State *L, const char *key, lua_Integer fallback)
{
    // to be called with a table on the stack's top
    lua_Integer ret;

    lua_getfield(L, -1, key);
    switch (lua_type(L, -1)) {
    case LUA_TNUMBER:
        ret = lua_tointeger(L, -1);
        break;
    case LUA_TNIL:
        // omitted, use default value
        ret = fallback;
        LOG_DEBUG("no '%s' was given, default to %ld", key, (long)ret);
        break;
    default:
        // luaL_error() actually will not return
        lua_pop(L, 1);
        return luaL_error(L, "'%s' shall be a number", key);
    }
    lua_pop(L, 1);
    return ret;
}

static int
fetch_from_lua(lua_State *L)
{
    int ret = 0;
    const int args = lua_gettop(L);

    size_t head = 0;
    size_t woffset = 0;
    off_t tail = -1;

    if (args != 3) {
        LOG_ERROR("number of args was %d, shall be 3", args);
        return EINVAL;
    }

    // optional configs may be passed as arg#3
    if (lua_type(L, 3) == LUA_TTABLE) {
        lua_Integer lua_int;
        // "offset" is where to start reading from the URL
        lua_int = get_optional_luaint(L, "offset", 0);
        if ((0 <= lua_int) && (lua_int <= SSIZE_MAX)) {
            // limit valid size by ssize_t for pwrite()
            head = lua_int;
            LOG_DEBUG("set head=%ld", (long)head);
        } else {
            LOG_ERROR("invalid offset %zd", (ssize_t)lua_int);
            return EINVAL;
        }

        // "size" is the amount of data to be fetched from the URL
        // ('-1' will be interpreted as "rest of all")
        lua_int = get_optional_luaint(L, "size", -1);
        if (-1 == lua_int) {
            // size == -1 will be interpreted as 'rest of all'
            tail = -1;
        } else if ((0 <= lua_int) && (lua_int <= SSIZE_MAX)) {
            if (head > ((size_t)lua_int + head)) {
                // overflow
                LOG_ERROR("invalid size %zd from offset %zu", (ssize_t)lua_int,
                  head);
                return EINVAL;
            }
            tail = lua_int + head;
            LOG_DEBUG("set tail=%zu", (ssize_t)tail);
        } else {
            LOG_ERROR("invalid size %zd", (ssize_t)lua_int);
            return EINVAL;
        }

        // "woffset" is where to start writing fetched data in the file
        // (may be set to non-zero to update a file incrementally)
        lua_int = get_optional_luaint(L, "woffset", head);
        if ((0 <= lua_int) && (lua_int <= SSIZE_MAX)) {
            woffset = lua_int;
            LOG_DEBUG("set woffset=%zd", woffset);
        } else {
            LOG_ERROR("invalid woffset %zu", (size_t)lua_int);
            return EINVAL;
        }
    } else {
        if (lua_type(L, 3) != LUA_TNIL) {
            LOG_ERROR("arg#3 shall be a table, got %d", lua_type(L, 3));
            return EINVAL;
        }
    }

    // arg#1: url (as str)
    if (lua_type(L, 1) != LUA_TSTRING) {
        LOG_ERROR("args#1 shall be a URL string, got %d", lua_type(L, 1));
        return EINVAL;
    }
    const char *url = lua_tostring(L, 1);

    // arg#2: url (as str)
    switch (lua_type(L, 2)) {
    case LUA_TSTRING: {
        // write data from the URL to a file
        int flags = O_CREAT | O_RDWR; // ToDo: set O_EXCL if requested
        int mode = 0660;
        const char *dst = lua_tostring(L, 2);
        int fd = open(dst, flags, mode);
        if (fd != -1) {
            LOG_DEBUG("downloaing to '%s'", dst);
            CURL *c = curl_easy_init();
            if (c) {
                struct to_fd arg = { .fd = fd, .base = woffset };
                ret = fetch_to_fd(c, url, head, tail, &arg);
                if (ret) {
                    LOG_ERROR("download error %d", ret);
                }
                curl_easy_cleanup(c);
            } else {
                ret = ENOMEM;
                LOG_ERROR("failed to init cURL, return %d", ret);
            }
            close(fd);
        } else {
            int e = errno;
            LOG_ERROR("failed to open(%s): %s", dst, strerror(e));
            return e;
        }
        break;
    }
    default:
        // ToDo: may allow to using mkstemp / downloading to memory
        LOG_ERROR("arg2 shall be a path string, got %d", lua_type(L, 2));
        ret = EINVAL;
        break;
    }

    LOG_DEBUG("ret %d", ret);

    if (ret != 0) {
        /* returns an error obj */
        lua_pushinteger(L, ret);
        return 1;
    } else {
        return 0; /* no error */
    }
}

int
http_push_fecher_to_lua(lua_State *L)
{
    // ToDOL it's failible?
    lua_pushcfunction(L, fetch_from_lua);
    return 0;
}
