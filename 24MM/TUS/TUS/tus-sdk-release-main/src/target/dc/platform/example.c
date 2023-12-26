// sample of platform-dependent implementations

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "platform.h"

#define LOG_(UTIL, FUNC, FMT, ...)                                             \
    do {                                                                       \
        assert((UTIL)->FUNC != NULL);                                          \
        ((UTIL)->FUNC)((UTIL), ("%32s:%03u %20s()\t" FMT), __FILE__, __LINE__, \
          __func__, __VA_ARGS__);                                              \
    } while (0)
#define LOG_HIDDEN(UTIL, FMT, ...) \
    do { /* do nothing */          \
    } while (0)
#define LOG_DEBUG(UTIL, FMT, ...) LOG_(UTIL, log_debug, FMT, __VA_ARGS__);
#define LOG_INFO(UTIL, FMT, ...) LOG_(UTIL, log_info, FMT, __VA_ARGS__);
#define LOG_ERROR(UTIL, FMT, ...) LOG_(UTIL, log_error, FMT, __VA_ARGS__);

struct tus_dc_platform platform = {
    .desc = "example implementaion",
};

struct userdata {
    const char *input_basedir;
    const char *output_basedir;
    int mode;
} config = {
    "/tmp", // input_basedir: where gather_environment() will look for
    "/tmp", // output_basedir: where propagate_environment() will make files
    0664,   // mode used to create a new file
};

static void
finalize(struct tus_dc_platform *platform)
{
    if (!platform)
        return;

    // has no internal state to destruct

    memset(platform, 0, sizeof(*platform));
}

static int
full_read(int fd, void *src, ssize_t limit, size_t *read_bytes)
{
    int ret = 0;
    // always re-read every data, as previouly read data may be overwritten
    ssize_t cur = 0;
    while (cur < limit) {
        ssize_t bytes = pread(fd, (char *)src + cur, limit - cur, cur);
        if (bytes == -1) {
            if (errno == EINTR) {
                continue;
            }
            ret = errno;
            break;
        } else if (bytes == 0) {
            // EOF
            break;
        }
        if (cur + bytes < cur || limit < cur + bytes) {
            // (shall not happen unless something has went wrong)
            // cannot trust errno
            ret = EIO;
            break;
        }
        cur += bytes;
    }
    if (!ret && read_bytes) {
        // report how many bytes that have actually been read
        *read_bytes = cur;
    }

    return ret;
}

static int
full_write(int fd, const void *src, ssize_t limit)
{
    int ret = 0;
    ssize_t left = limit;
    while (left > 0) {
        ssize_t written = write(fd, src, left);
        if (written == -1) {
            if (errno == EINTR) {
                // interrupted before any data was written
                continue;
            }
            ret = errno;
        } else if (written <= 0 || written > left) {
            // (shall not happen unless something has went wrong)
            // cannot trust errno
            ret = EIO;
            break;
        } else {
            // write() may success partially
            left -= written;
        }
    }
    return ret;
}

#define MSG_MAX 64
static int
propagate_environment(const struct tus_dc_platform *platform, const char *key,
  const char *value)
{
    const struct tus_dc_platform_utility *util = platform->util;
    const struct userdata *config = platform->userdata;

    LOG_INFO(util, "%s -> %s", key, value);

    size_t left = strnlen(value, MSG_MAX);
    if (left >= MSG_MAX) {
        return ENAMETOOLONG;
    }

    const char *dirpath = config->output_basedir;
    assert(dirpath != NULL);

    int ret;
    int dirfd = open(dirpath, O_RDONLY | O_DIRECTORY);
    if (dirfd != -1) {
        // note: assuming key won't contain '/'
        const char *filename = key;
        int fd = openat(dirfd, filename, O_CREAT | O_WRONLY | O_APPEND,
          config->mode);
        if (fd != -1) {
            // ToDO: prefer LOCK_NB?
            if (flock(fd, LOCK_EX) == 0) {
                ret = full_write(fd, value, left);
                if (!ret) {
                    char timestamp[32] = {};
                    struct tm tm;
                    {
                        time_t t;
                        time(&t);
                        gmtime_r(&t, &tm);
                    }
                    // prefer UTC (do not trust TZ)
                    strftime(timestamp, sizeof(timestamp) - 1,
                      "\t@\t%Y-%m-%d %H:%M:%S UTC\n", &tm);
                    ret = full_write(fd, timestamp, strlen(timestamp));
                }
                (void)fdatasync(fd);
                flock(fd, LOCK_UN);
            } else {
                ret = errno;
                LOG_ERROR(util, "!! failed to open '%s' for append-writing %d",
                  filename, ret);
            }
            close(fd);
        } else {
            ret = errno;
            LOG_ERROR(util, "!! failed to open dir '%s' %d", dirpath, ret);
        }
        close(dirfd);
    } else {

        ret = errno;
    }
    return ret;
}

static int
gather_environment_initialize(const struct tus_dc_platform_utility *util,
  int dirfd, const char *filename, int mode, int *new_fd)
{
    int ret;
    *new_fd = -1;
    // create a new file which shall be written from a external system
    if (dirfd <= 0) {
        return ENOTDIR;
    }
    do {
        int fd = openat(dirfd, filename, O_RDONLY | O_CREAT | O_EXCL, mode);
        if (fd != -1) {
            LOG_INFO(util, "created '%s'", filename);
            ret = 0;
            *new_fd = fd;
            break;
        }
        ret = errno;
        if (ret == EEXIST) { // O_EXCL
            if (unlinkat(dirfd, filename, 0) == -1) {
                ret = errno;
                if (ret == ENOENT) {
                    LOG_INFO(util, "someone has unlinked '%s', rece condition?",
                      filename);
                    ret = 0; // retry
                } else {
                    LOG_ERROR(util, "failed to unlink existing '%s' (%d)",
                      filename, ret);
                }
            } else {
                LOG_INFO(util, "unlinked existing '%s'", filename);
                ret = 0; // retry
            }
        } else if (ret) {
            LOG_ERROR(util, "failed to create '%s' (%d)", filename, ret);
        } else {
            // shall not happen
            LOG_ERROR(util, "unexpected failure while creating '%s'", filename);
            ret = EIO;
        }
    } while (ret == 0);

    return ret;
}

static void
gather_environment_finalize(const struct tus_dc_platform_utility *util,
  int dirfd, const char *filename)
{
    if (unlinkat(dirfd, filename, 0) == -1) {
        if (errno == ENOENT) {
            LOG_INFO(util, "someone has unlinked '%s', rece condition?",
              filename);
        } else {
            int ret = errno;
            LOG_ERROR(util, "failed to unlink '%s' (%d)", filename, ret);
        }
    } else {
        LOG_INFO(util, "unlinked existing '%s'", filename);
    }
}

// check states of external systems for 'key' and fill 'buffer'
static int
gather_environment(const struct tus_dc_platform *platform,
  const struct timespec *previous, const char *key, void *buffer, size_t limit,
  size_t *filled)
{
    const struct tus_dc_platform_utility *util = platform->util;
    const struct userdata *config = platform->userdata;
    const char *dirpath = config->input_basedir;
    assert(dirpath != NULL);

    if (filled)
        *filled = 0;

    int ret;
    int dirfd = open(dirpath, O_RDONLY | O_DIRECTORY);
    if (dirfd != -1) {
        // note: assuming the 'key' won't contain '/'
        const char *filename = key;

        if (!buffer) {
            LOG_INFO(util, "nowhere to read into, finalizing '%s'", filename);
            gather_environment_finalize(util, dirfd, filename);
            (void)close(dirfd);
            return 0; // finalization cannot fail
        }

        int fd = -1;
        if (!previous) {
            // 'just started checking'
            ret = gather_environment_initialize(util, dirfd, filename,
              config->mode, &fd);
            LOG_INFO(util, "started to check '%s' (%d)", filename, ret);
            // continue (and try to read if ret == 0)
        } else {
            fd = openat(dirfd, filename, O_RDONLY);
            if (fd == -1) {
                // ensure ret != 0
                ret = (errno ? errno : EIO);
            } else {
                ret = 0;
            }
        }

        if (ret == 0) {
            // try to read a file to the 'buffer'
            assert(fd != -1);
            // ToDO: prefer LOCK_NB?
            if (flock(fd, LOCK_SH) == 0) {
                size_t bytes;
                ret = full_read(fd, buffer, limit, &bytes);
                if (ret == 0) {
                    LOG_HIDDEN(util, "read %zd '%.4s'", bytes, (char *)buffer);
                    if (filled)
                        *filled = bytes;
                }
                flock(fd, LOCK_UN);
            } else {
                ret = errno;
                LOG_ERROR(util, "!! failed to lock '%s' for reading (%d)",
                  filename, ret);
            }
            close(fd);
        } else {
            LOG_ERROR(util, "!! failed to open '%s' for reading (%d)", filename,
              ret);
        }
        (void)close(dirfd);
    } else {
        ret = errno;
        LOG_ERROR(util, "!! failed to open dir '%s' %d", dirpath, ret);
    }

    return ret;
}

static const char *default_config_dir = "./version";
static int
get_version(const struct tus_dc_platform *platform,
  int variation_code, // enum dc_version_variation
  int context_code,   // enum dc_version_context
  const char *key, void *buffer, size_t limit, size_t *filled)
{
    const struct tus_dc_platform_utility *util = platform->util;
    //    const struct userdata *config = platform->userdata;
    const char *dirpath = default_config_dir;
    assert(dirpath != NULL);

    int ret = EINVAL;
    if (filled)
        *filled = 0;

    switch (context_code) {
    case DC_VERSION_CONTEXT_ANY:
        // no special context
        switch (variation_code) {
        case DC_VERSION_VARIATION_CURRENT:
            LOG_DEBUG(util, "valid selector: context=%d, variation=%d",
              context_code, variation_code);
            ret = 0;
            break;
        }
        break;
    default:
        break;
    }
    if (ret) {
        LOG_ERROR(util, "invalid selector(%d): context=%d, variation=%d", ret,
          context_code, variation_code);
        return ret;
    }

    int dirfd = open(dirpath, O_RDONLY | O_DIRECTORY);
    if (dirfd == -1 && errno == ENOENT) {
        ret = mkdir(dirpath, 0755);
        if (ret != 0) {
            int err = errno ? errno : EIO;
            LOG_ERROR(util, "failed to mkdir(%s) %d", dirpath, err);
            return err;
        }
        dirfd = open(dirpath, O_RDONLY | O_DIRECTORY);
        LOG_INFO(util, "mkdir(%s) -> dirfd=%d", dirpath, dirfd);
    }
    if (dirfd != -1) {
        // note: assuming the 'key' won't contain '/'
        const char *filename = key;

        int fd = -1;
        fd = openat(dirfd, filename, O_RDONLY);
        if (fd == -1) {
            // ensure ret != 0
            ret = (errno ? errno : EIO);
            LOG_ERROR(util, "!! failed to open '%s' for reading (%d)", filename,
              ret);
        } else {
            // assuming the file is fully governed by TUS, and no locking is
            // needed
            size_t bytes;
            ret = full_read(fd, buffer, limit, &bytes);
            if (ret == 0) {
                LOG_HIDDEN(util, "read %zd '%.4s'", bytes, (char *)buffer);
                if (filled)
                    *filled = bytes;
            }
            close(fd);
        }
        (void)close(dirfd);
    } else {
        ret = errno;
        LOG_ERROR(util, "!! failed to open dir '%s' %d", dirpath, ret);
    }

    return ret;
}

static int
update_version(const struct tus_dc_platform *platform,
  int variation_code, // enum dc_version_variation
  int context_code,   // enum dc_version_context
  const char *key, const void *value, size_t size)
{
    const struct tus_dc_platform_utility *util = platform->util;
    const struct userdata *config = platform->userdata;
    const char *dirpath = default_config_dir;
    assert(dirpath != NULL);

    int ret = EINVAL;

    switch (context_code) {
    case DC_VERSION_CONTEXT_ANY:
        // no special context
        switch (variation_code) {
        case DC_VERSION_VARIATION_CURRENT:
            LOG_DEBUG(util, "valid selector: context=%d, variation=%d",
              context_code, variation_code);
            ret = 0;
            break;
        }
        break;
    default:
        break;
    }
    if (ret) {
        LOG_ERROR(util, "invalid selector(%d): context=%d, variation=%d", ret,
          context_code, variation_code);
        return ret;
    }

    int dirfd = open(dirpath, O_RDONLY | O_DIRECTORY);
    if (dirfd == -1 && errno == ENOENT) {
        ret = mkdir(dirpath, 0755);
        if (ret != 0) {
            int err = errno ? errno : EIO;
            LOG_ERROR(util, "failed to mkdir(%s) %d", dirpath, err);
            return err;
        }
        dirfd = open(dirpath, O_RDONLY | O_DIRECTORY);
        LOG_INFO(util, "mkdir(%s) -> dirfd=%d", dirpath, dirfd);
    }
    if (dirfd != -1) {
        // note: assuming the 'key' won't contain '/'
        const char *filename = key;

        int fd = -1;
        fd = openat(dirfd, filename, O_WRONLY | O_CREAT, config->mode);
        if (fd == -1) {
            // ensure ret != 0
            ret = (errno ? errno : EIO);
            LOG_ERROR(util, "!! failed to open '%s' for writing (%d)", filename,
              ret);
        } else {
            // assuming the file is fully governed by TUS, and no locking is
            // needed
            ret = full_write(fd, value, size);
            if (ret == 0) {
                LOG_DEBUG(util, "written %zd to '%s'/'%s'", size, dirpath,
                  filename);
            }
            close(fd);
        }
        (void)close(dirfd);
    } else {
        ret = errno;
        LOG_ERROR(util, "!! failed to open dir '%s' %d", dirpath, ret);
    }

    return ret;
}

static int
updater_example1_op_test1(const struct tus_dc_platform_utility *util,
  struct tus_dc_updater_call_context *call_ctx)
{
    LOG_INFO(util, "- util=%p, call_ctx=%p", util, call_ctx);

    assert(util->updater.count_arguments != NULL);
    const int num_args = (util->updater.count_arguments)(call_ctx);
    LOG_INFO(util, "- number of Lua args = %d", num_args);

    { // try to get 1st arg
        int64_t arg_0;
        int err = (util->updater.get_arg_as_int64)(call_ctx, 0, &arg_0);
        if (err == 0) {
            LOG_INFO(util, "arg#0 is int %lld", (long long)arg_0);
        } else {
            LOG_ERROR(util, "arg#0 is not a integer? %d", err);
        }
    }
    { // try to get 2nd arg
        char buf[32] = {};
        int err = (util->updater.copy_arg_data)(call_ctx, 1, buf, sizeof(buf));
        if (err == 0) {
            LOG_INFO(util, "arg#1 is string '%s'", buf);
        } else {
            LOG_ERROR(util, "arg#1 is not a string? %d", err);
        }
    }

    { // async op start / queue 'done'
        uint32_t tag = (util->async.generate_tag)(util);
        LOG_INFO(util, "async.generate_tag -> %d", tag);
        if (tag != TUS_DC_PAL_INVALID_ASYNC_TAG) {
            // do this from another thread after some works have done
            int code = 0;
            int err = (util->async.set_results)(util, tag, code);
            LOG_INFO(util, "async.set_results -> %d", err);
        } else {
            LOG_ERROR(util, "failed to create async tag? %d", tag);
        }
    }

    { // prepare a value to retrun to Lua
        const int lua_retval = 1234;
        int err = (util->updater.add_result_int64)(call_ctx, lua_retval);
        if (err == 0) {
            LOG_INFO(util, "pushed '%d'", lua_retval);
        } else {
            LOG_ERROR(util, "failed to push retval? %d", err);
        }
    }
    return 0; // no error, use values in Lua stack as a (maybe multi) return
}

const struct tus_dc_updater_function updater_example1_functions[] = { {
  .name = "func1",
  .func = updater_example1_op_test1,
} };

static int
get_updater(const struct tus_dc_platform *platform,
  const struct tus_dc_updater_type_id *updater_type,
  const struct tus_dc_update_target_id *target_id,
  struct tus_dc_updater *updater)
{
    static const struct tus_dc_updater example1 = { .id = { "example1" },
        .functions = updater_example1_functions,
        .num_functions = sizeof(updater_example1_functions) /
          sizeof(updater_example1_functions[0]) };
    const struct tus_dc_platform_utility *util = platform->util;
    int err =
      ENOENT; // 'no updater which matchs updater_type/target_id was found'

    LOG_INFO(util, "requested: '%s', %s", updater_type->id,
      target_id ? target_id->id : "");

    if ((util->updater.compare_updater_type)(updater_type, &(example1.id),
          NULL) == 0) {
        LOG_INFO(util, "matched %s", example1.id.id);
        *updater = example1;
        if (target_id) {
            updater->target_id = *target_id;
        } else {
            // not bound to a specific target
        }
        LOG_INFO(util, " tid='%s'", updater->target_id.id);

        // note: may allocate and set updater->userdata here (if needed)

        return 0; // 'updated *updater'
    }

    if (err) {
        LOG_INFO(util, "err=%d", err);
    }
    return err;
}

static int
free_updater(const struct tus_dc_platform *platform,
  struct tus_dc_updater *updater)
{
    const struct tus_dc_platform_utility *util = platform->util;
    LOG_INFO(util, "!! free(%p)", updater);

    // nothing to actually free for now so far
    return 0;
}

static void
notify(const struct tus_dc_platform *platform,
  enum tus_dc_pal_notification what, const void *aux, size_t aux_size)
{
    const struct tus_dc_platform_utility *util = platform->util;
    switch (what) {
    case TUS_DC_PAL_NOTIFICATION_SCRIPT_STARTED:
        LOG_INFO(util, "TUS_DC_PAL_NOTIFICATION_SCRIPT_STARTED(%d)", what);
        break;
    case TUS_DC_PAL_NOTIFICATION_SCRIPT_STOPPED:
        LOG_INFO(util, "TUS_DC_PAL_NOTIFICATION_SCRIPT_STOPPED(%d)", what);
        break;
    default:
        LOG_INFO(util, "got unknown notification(%d), %p:%zd", what, aux,
          aux_size);
        break;
    }
}

int
tus_dc_platform_initialize(struct tus_dc_platform *platform)
{
    // platform->util should have already been set
    const struct tus_dc_platform_utility *util = platform->util;

    strncpy(platform->desc, "example implementaion of DC PAL",
      sizeof(platform->desc) - 1);
    platform->userdata = &config;

    platform->finalize = finalize;

    platform->gather_environment = gather_environment;
    platform->propagate_environment = propagate_environment;

    platform->get_version = get_version;
    platform->update_version = update_version;

    platform->get_updater = get_updater;
    platform->free_updater = free_updater;

    platform->notify = notify;

    LOG_INFO(util, "inifialized platform '%s'", platform->desc);

    return 0;
}
