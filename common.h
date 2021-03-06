#pragma once

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define i64 int64_t
#define u64 uint64_t
#define i32 int32_t
#define u32 uint32_t
#define u16 uint16_t
#define u8 uint8_t
#define usize size_t

typedef enum {
    RES_OK,
    RES_NONE,
    RES_UNEXPECTED_TOKEN,
    RES_INVALID_SOURCE_FILE_NAME,
    RES_SOURCE_FILE_READ_FAILED,
    RES_ASM_FILE_READ_FAILED,
    RES_FAILED_AS,
    RES_FAILED_LD,
    RES_NON_MATCHING_TYPES,
    RES_EXPECTED_PRIMARY,
    RES_UNKNOWN_VAR,
    RES_ASSIGNING_VAL,
    RES_NEED_AT_LEAST_ONE_STMT,
    RES_MISSING_PARAM,
    RES_ERR,
} mkt_res_t;

static bool is_tty = false;

#define STR(s) #s

#define CHECK(a, cond, b, fmt)                                              \
    do {                                                                    \
        if (!((a)cond(b))) {                                                \
            fprintf(stderr,                                                 \
                    __FILE__ ":%d:CHECK failed: %s " STR(                   \
                        cond) " %s i.e.: " fmt " " STR(cond) " " fmt        \
                                                             " is false\n", \
                    __LINE__, STR(a), STR(b), a, b);                        \
            assert(0);                                                      \
        }                                                                   \
    } while (0)

#define UNIMPLEMENTED()                                            \
    do {                                                           \
        fprintf(stderr, __FILE__ ":%d:unimplemented\n", __LINE__); \
        exit(1);                                                   \
    } while (0)

#define UNREACHABLE()                                            \
    do {                                                         \
        fprintf(stderr, __FILE__ ":%d:unreachable\n", __LINE__); \
        exit(1);                                                 \
    } while (0)

#define IGNORE(x)  \
    do {           \
        (void)(x); \
    } while (0)

#if WITH_LOGS == 0
#define log_debug(fmt, ...) \
    do {                    \
        IGNORE(fmt);        \
    } while (0)
#define log_debug_with_indent(indent, fmt, ...) \
    do {                                        \
        IGNORE(indent);                         \
        IGNORE(fmt);                            \
    } while (0)
#else
#define log_debug(fmt, ...)                                                \
    do {                                                                   \
        fprintf(stderr, "[debug] %s:%s:%d: " fmt "\n", __FILE__, __func__, \
                __LINE__, __VA_ARGS__);                                    \
    } while (0)
#define log_debug_with_indent(indent, fmt, ...)                              \
    do {                                                                     \
        fprintf(stderr, "[debug] %s:%s:%d: ", __FILE__, __func__, __LINE__); \
        for (i32 i = 0; i < indent; i++) fprintf(stderr, " ");               \
        fprintf(stderr, fmt "\n", __VA_ARGS__);                              \
    } while (0)
#endif

typedef enum {
    COL_RESET,
    COL_GRAY,
    COL_RED,
    COL_GREEN,
    COL_COUNT,
} mkt_color_t;

static const char mkt_colors[2][COL_COUNT][14] = {
    // is_tty == true
    [true] = {[COL_RESET] = "\x1b[0m",
              [COL_GRAY] = "\x1b[38;5;250m",
              [COL_RED] = "\x1b[31m",
              [COL_GREEN] = "\x1b[32m"}};

#define TRY_OK(expr)                   \
    do {                               \
        mkt_res_t res = expr;          \
        if (res != RES_OK) return res; \
    } while (0)

#define TRY_NONE(expr)                   \
    do {                                 \
        mkt_res_t res = expr;            \
        if (res != RES_NONE) return res; \
    } while (0)

#define ARR_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
