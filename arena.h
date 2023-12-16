#pragma once

#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

typedef uint64_t u64;
typedef int64_t i64;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint8_t u8;
typedef int8_t i8;
typedef size_t usize;
typedef ssize_t isize;

#define KiB 1024UL
#define MiB ((KiB) * 1024UL)

// ------------------- Logs

static bool cli_log_verbose = false;
#define LOG(fmt, ...)                                                          \
  if (cli_log_verbose)                                                         \
  fprintf(stderr, fmt "\n", __VA_ARGS__)

// ----------- Utility macros

// Check that __COUNTER__ is defined and that __COUNTER__ increases by 1
// every time it is expanded. X + 1 == X + 0 is used in case X is defined to be
// empty. If X is empty the expression becomes (+1 == +0).
#if defined(__COUNTER__) && (__COUNTER__ + 1 == __COUNTER__ + 0)
#define PG_PRIVATE_UNIQUE_ID __COUNTER__
#else
#define PG_PRIVATE_UNIQUE_ID __LINE__
#endif

// Helpers for generating unique variable names
#define pg_private_name(n) pg_private_concat(n, PG_PRIVATE_UNIQUE_ID)
#define pg_private_concat(a, b) pg_private_concat2(a, b)
#define pg_private_concat2(a, b) a##b
#define pg_pad(n) u8 pg_private_name(_padding)[n]

#define pg_unused(x) (void)(x)

#define pg_assert(condition) assert(condition)

#define pg_max(a, b) (((a) > (b)) ? (a) : (b))

#define PG_DBL_EPSILON (double)2.2e-16

#define carray_count(a) (sizeof(a) / sizeof((a)[0]))

static u64 pg_pow_u64(u64 a, u64 b) {
  if (b == 0)
    return 1;
  if (b == 1)
    return a;
  else
    return a * pg_pow_u64(a, b - 1);
}

// --------------------------- Arena

typedef struct Mem_profile Mem_profile;
typedef struct {
  u8 *_Nonnull start;
  u8 *_Nonnull end;
  Mem_profile *_Nullable profile;
} Arena;

__attribute__((warn_unused_result)) static u32
arena_offset_from_end(void *_Nonnull ptr, Arena a) {
  pg_assert((u8 *)ptr <= a.end);

  const usize offset = (usize)(a.end - (u8 *)ptr);
  pg_assert(offset <= UINT32_MAX);
  // Ensure that the last bits are clear to store the flags.
  return (u32)offset;
}

__attribute__((warn_unused_result)) static Arena
arena_new(usize cap, Mem_profile *_Nullable profile) {
  u8 *const mem = mmap(NULL, cap, PROT_READ | PROT_WRITE,
                       MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  pg_assert(mem);

  Arena arena = {
      .profile = profile,
      .start = mem,
      .end = mem + cap,
  };
  return arena;
}

__attribute__((warn_unused_result)) static Arena
arena_from_mem(u8 *_Nonnull mem, usize mem_len) {
  return (Arena){
      .start = mem,
      .end = mem + mem_len,
  };
}

static void mem_profile_record_alloc(Mem_profile *_Nonnull profile,
                                     usize objects_count, usize bytes_count);

__attribute__((warn_unused_result))
__attribute((malloc, alloc_size(2, 4), alloc_align(3))) static void
    *_Nonnull arena_alloc(Arena *_Nonnull a, size_t size, size_t align,
                          size_t count) {
  pg_assert(a->start <= a->end);
  pg_assert(size > 0);
  pg_assert(align == 1 || align == 2 || align == 4 || align == 8);

  const usize available = (usize)a->end - (usize)a->start;
  const usize padding = -(usize)a->start & (align - 1);

  // Ignore overflow for now.
  size_t offset = padding + size * count;
  if (available < offset) {
    fprintf(stderr,
            "Out of memory: available=%lu "
            "allocation_size=%lu\n",
            available, offset);
    abort();
  }

  u8 *const res = a->start + padding;
  pg_assert(res + count * size <= a->end);
  memset(res, 0, size * count);

  a->start += offset;
  pg_assert(a->start <= a->end);

  if (a->profile) {
    mem_profile_record_alloc((Mem_profile *_Nonnull)a->profile, count, offset);
  }

  return (void *)res;
}

__attribute__((warn_unused_result)) static bool
arena_is_ptr_last_allocation(const Arena *_Nonnull arena,
                             const void *_Nullable ptr, u64 size) {
  if (!ptr)
    return false;

  pg_assert(size > 0);

  const u8 *const ptr_u8 = (const u8 *)ptr;
  pg_assert(ptr_u8 <= arena->start);
  pg_assert(ptr_u8 + size <= arena->end);

  return ptr_u8 + size == arena->start;
}
