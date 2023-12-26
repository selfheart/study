/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

enum log_lv {
    LOG_LV_CRITICAL = 0,
    LOG_LV_ERROR = 1,
    LOG_LV_WARN = 2,
    LOG_LV_INFO = 3,
    LOG_LV_DEBUG = 4,
    LOG_LV_HIDDEN = 5, // 'to be hidden for normal build'
};

void logger_set_prefix(const char *fmt, ...)
  __attribute__((format(printf, 1, 2)));

#ifdef va_arg
void log_valist_raw(int lv, const char *fmt, va_list ap);
#endif

void log_valist(int lv, const char *fmt, ...)
  __attribute__((format(printf, 2, 3)));

// note: "..." may not be empty (to avoid using gnu's '##__VA_ARGS__')
#define LOG__VALIST(LV, FMT, ...)                                              \
    do {                                                                       \
        log_valist(LV, "%32s:%03u %20s()\t" FMT, __FILE__, __LINE__, __func__, \
          __VA_ARGS__);                                                        \
    } while (0)

#define LOG_CRITICAL(FMT, ...) LOG__VALIST(LOG_LV_CRITICAL, FMT, __VA_ARGS__)
#define LOG_ERROR(FMT, ...) \
    LOG__VALIST(LOG_LV_ERROR, "!!ERROR!!\t" FMT, __VA_ARGS__)
#define LOG_WARN(FMT, ...) LOG__VALIST(LOG_LV_WARN, FMT, __VA_ARGS__)
#define LOG_INFO(FMT, ...) LOG__VALIST(LOG_LV_INFO, FMT, __VA_ARGS__)
#define LOG_DEBUG(FMT, ...) LOG__VALIST(LOG_LV_DEBUG, FMT, __VA_ARGS__)
#define LOG_HIDDEN(FMT, ...) LOG__VALIST(LOG_LV_HIDDEN, FMT, __VA_ARGS__)

#ifdef __cplusplus
}
#endif // __cplusplus
