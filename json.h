#pragma once
#include "arena.h"
#include "cursor.h"
#include "str.h"

typedef enum {
  JSON_KIND_UNDEFINED,
  JSON_KIND_OBJECT,
  JSON_KIND_ARRAY,
  JSON_KIND_STRING,
  JSON_KIND_NUMBER,
  JSON_KIND_BOOL,
} Json_kind;

typedef struct Json Json;

typedef struct {

  u32 len;
} Json_array;

struct Json {
  Json_kind kind;
  union {
    double number;
    bool boolean;
    Str string;
    // TODO: Array, Object
  } v;
};

static Json *json_parse_number(Read_cursor *cursor, Arena *arena) {
  double num = 0;

  const double sign = read_cursor_match_char(cursor, '-') ? -1 : 1;

  // TODO: 1e3.
  while (!read_cursor_is_at_end(*cursor)) {
    const u8 c = read_cursor_next(cursor);
    if (!str_is_digit(c))
      break;

    num = num * 10 + (c - '0');
  }

  Json *j = arena_alloc(arena, sizeof(Json), _Alignof(Json), 1);
  *j = (Json){.kind = JSON_KIND_NUMBER, .v.number = (double)num * sign};
  return j;
}

static Json *json_parse_bool(Read_cursor *cursor, Arena *arena) {
  bool val = false;
  if (read_cursor_match(cursor, str_from_c("true")))
    val = true;
  else if (read_cursor_match(cursor, str_from_c("false")))
    val = false;
  else
    return NULL;

  Json *j = arena_alloc(arena, sizeof(Json), _Alignof(Json), 1);
  *j = (Json){.kind = JSON_KIND_BOOL, .v.boolean = val};
  return j;
}

static Json *json_parse(Read_cursor *cursor, Arena *arena) {
  while (!read_cursor_is_at_end(*cursor)) {
    const u8 c = read_cursor_peek(*cursor);

    if (str_is_digit(c) || c == '-') {
      return json_parse_number(cursor, arena);
    } else if (c == 't' || c == 'f') {
      return json_parse_bool(cursor, arena);
    } else {
      return NULL;
    }
  }
  return NULL;
}

static void test_json_parse() {
  {
    const Str in = str_from_c("xxx");
    u8 mem[256] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    Json *j = json_parse(&cursor, &arena);
    pg_assert(j == NULL);
  }
  {
    const Str in = str_from_c("123");
    u8 mem[256] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    Json *j = json_parse(&cursor, &arena);
    pg_assert(j != NULL);
    pg_assert(j->kind = JSON_KIND_NUMBER);
    pg_assert(j->v.number == 123);
  }
  {
    const Str in = str_from_c("-123");
    u8 mem[256] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    Json *j = json_parse(&cursor, &arena);
    pg_assert(j != NULL);
    pg_assert(j->kind = JSON_KIND_NUMBER);
    pg_assert(j->v.number == -123);
  }
  {
    const Str in = str_from_c("true");
    u8 mem[256] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    Json *j = json_parse(&cursor, &arena);
    pg_assert(j != NULL);
    pg_assert(j->kind = JSON_KIND_BOOL);
    pg_assert(j->v.boolean == true);
  }
  {
    const Str in = str_from_c("false");
    u8 mem[256] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    Json *j = json_parse(&cursor, &arena);
    pg_assert(j != NULL);
    pg_assert(j->kind = JSON_KIND_BOOL);
    pg_assert(j->v.boolean == false);
  }
}
