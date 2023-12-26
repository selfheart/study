#pragma once

#include <stdint.h>

struct ctx; // for PAL, 'struct ctx' is an opaque data

struct tus_dc_updater_type_id {
    char id[16]; // fixme: tentative
};
struct tus_dc_update_target_id {
    char id[256]; // fixme: tentative
};
struct tus_dc_updater_call_context;
struct tus_dc_platform_utility;

struct tus_dc_updater_function {
    const char *name;
    // shal lreturn numbher of ouput[] that filled, or nagative error number
    int (*func)(const struct tus_dc_platform_utility *util,
      struct tus_dc_updater_call_context *ctx);
};

// keep info to instantiate a custom updater on DC Lua env
struct tus_dc_platform_utility;
struct tus_dc_updater {
    struct tus_dc_updater_type_id id;
    struct tus_dc_update_target_id target_id;

    void *userdata; // for PAL. not touched by TUS

    const struct tus_dc_updater_function *functions;
    size_t num_functions;
};

// features exposed for each implementation, provided from TUS DC runtime
struct tus_dc_platform;
struct tus_dc_platform_utility {
    struct ctx *ctx;
    void (*log_error)(const struct tus_dc_platform_utility *util,
      const char *format, ...) __attribute__((format(printf, 2, 3)));
    void (*log_info)(const struct tus_dc_platform_utility *util,
      const char *format, ...) __attribute__((format(printf, 2, 3)));
    void (*log_debug)(const struct tus_dc_platform_utility *util,
      const char *format, ...) __attribute__((format(printf, 2, 3)));

    struct {
#define TUS_DC_PAL_INVALID_ASYNC_TAG (0)
        int (*generate_tag)(const struct tus_dc_platform_utility *util);
        int (*set_results)(const struct tus_dc_platform_utility *util, int tag,
          int code);
    } async;

    struct {
        // to be used from updater implementations
        // note: 'arg_index' is C_style (counted from 0)
        int (*compare_updater_type)(const struct tus_dc_updater_type_id *lhs,
          const struct tus_dc_updater_type_id *rhs, void *arg);
        int (*count_arguments)(struct tus_dc_updater_call_context *call_ctx);
        int (*get_arg_as_int64)(struct tus_dc_updater_call_context *call_ctx,
          int arg_index, int64_t *val);
        int (*copy_arg_data)(struct tus_dc_updater_call_context *call_ctx,
          int arg_index, void *buffer, size_t limit);
        int (*add_result_int64)(struct tus_dc_updater_call_context *call_ctx,
          int64_t result);
    } updater;

    // ToDo: add more utility functions needed to implement target-sprcific
    // parts
};

enum dc_version_variation {
    DC_VERSION_VARIATION_CURRENT = 0, //
    DC_VERSION_VARIATION__LIMIT
};

enum dc_version_context {
    DC_VERSION_CONTEXT_ANY = 0, //
    DC_VERSION_CONTEXT__LIMIT
};

enum tus_dc_pal_notification {
    TUS_DC_PAL_NOTIFICATION_UNKNOWN = 0,
    TUS_DC_PAL_NOTIFICATION_SCRIPT_STARTED,
    TUS_DC_PAL_NOTIFICATION_SCRIPT_STOPPED,
    TUS_DC_PAL_NOTIFICATION__LIMIT
};

struct tus_dc_platform {
    // description used for logging
    // to be set by tus_dc_platform_initialize()
    char desc[64];

    // not used by TUS runtime
    void *userdata;

    // functions to be implemented for each target platform
    // (to be set by a tus_dc_platform_initialize() implementation)

    void (*notify)(const struct tus_dc_platform *platform,
      enum tus_dc_pal_notification what, const void *aux, size_t aux_size);
    void (*finalize)(struct tus_dc_platform *platform);

    // poll external states that may update DC FSM env
    int (*gather_environment)(const struct tus_dc_platform *platform,
      const struct timespec *previous, const char *key, void *buffer,
      size_t limit, size_t *filled);
    // called to notify DC FSM env updates to externals
    int (*propagate_environment)(const struct tus_dc_platform *platform,
      const char *key, const char *value);

    // called from dc:get_version()
    int (*get_version)(const struct tus_dc_platform *platform,
      int variation_code, // enum dc_version_variation
      int context_code,   // enum dc_version_context
      const char *key, void *buffer, size_t limit, size_t *filled);
    // called from dc:update_version()
    int (*update_version)(const struct tus_dc_platform *platform,
      int variation_code, // enum dc_version_variation
      int context_code,   // enum dc_version_context
      const char *key, const void *value, size_t size);

    // create an updater instance
    int (*get_updater)(const struct tus_dc_platform *platform,
      const struct tus_dc_updater_type_id *updater_type,
      const struct tus_dc_update_target_id *target_id,
      struct tus_dc_updater *updater);
    // called when updater has gone
    int (*free_updater)(const struct tus_dc_platform *platform,
      struct tus_dc_updater *updater);

    // set by runtime before calling tus_dc_platform_initialize().
    // tus_dc_platform_initialize() may use 'util', but shall not modify it.
    const struct tus_dc_platform_utility *util;
};

// to be implemented for a specific platform
// will be called with *platform
// (platform->util is already populated on call, other members are 0-filled /
// NULL) expected to fill '*platform' and return 0 on success
int tus_dc_platform_initialize(struct tus_dc_platform *platform);
