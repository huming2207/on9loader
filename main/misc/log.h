#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void logger_init();

#ifdef DISABLE_LOG
#define ON9_LOG(text, ...) \
    do {                    \
    } while(0)             \

#define ON9_LOGN(text, ...) \
    do {                    \
    } while(0)             \

#else
#include <stdio.h>
#include <printf.h>

#if defined(__cplusplus) && (__cplusplus >  201703L)
#define ON9_LOG(text, ...) \
    do {                    \
        printf_(text __VA_OPT__(,) __VA_ARGS__);                        \
    } while(0)              \

#define ON9_LOGN(text, ...) \
    do {                    \
        printf_(text "\r\n" __VA_OPT__(,) __VA_ARGS__);                        \
    } while(0)              \


#else

#define ON9_LOG(text, ...) \
    do {                    \
        printf_(text, ##__VA_ARGS__);                        \
    } while(0)              \

#define ON9_LOGN(text, ...) \
    do {                    \
        printf_(text "\r\n", ##__VA_ARGS__);                        \
    } while(0)              \

#endif

#endif

#ifdef __cplusplus
}
#endif