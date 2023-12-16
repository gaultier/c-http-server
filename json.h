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

// TODO: Contemplate ways to shrink the size of this struct.
struct Json {
  Json_kind kind;
  pg_pad(4);
  union {
    double number;
    bool boolean;
    Str string;
    Json *children;
  } v;
  Json *next;
};

static Json *json_parse_number(Read_cursor *cursor, Arena *arena) {
  pg_assert(read_cursor_peek(*cursor) == '-' ||
            str_is_digit(read_cursor_peek(*cursor)));

  double num = 0;

  const double sign = read_cursor_match_char(cursor, '-') ? -1 : 1;

  // TODO: 1e3.
  while (!read_cursor_is_at_end(*cursor)) {
    const u8 c = read_cursor_peek(*cursor);
    if (!str_is_digit(c))
      break;

    num = num * 10 + (c - '0');
    read_cursor_next(cursor);
  }

  Json *j = arena_alloc(arena, sizeof(Json), _Alignof(Json), 1);
  *j = (Json){.kind = JSON_KIND_NUMBER, .v.number = (double)num * sign};
  return j;
}

static Json *json_parse_bool(Read_cursor *cursor, Arena *arena) {
  pg_assert(read_cursor_peek(*cursor) == 't' ||
            read_cursor_peek(*cursor) == 'f');

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

static Json *json_parse_string(Read_cursor *cursor, Arena *arena) {
  if (read_cursor_next(cursor) != '"')
    return NULL;

  Str s = {.data = read_cursor_remaining(*cursor).data};

  while (!read_cursor_is_at_end(*cursor)) {
    const u8 c = read_cursor_next(cursor);
    if (c == '"') {
      Json *j = arena_alloc(arena, sizeof(Json), _Alignof(Json), 1);
      *j = (Json){.kind = JSON_KIND_STRING, .v.string = s};
      return j;
    } else if (c == 92 /* Backslash */ && read_cursor_match_char(cursor, '"')) {
      s.len += 2;
    } else {
      s.len += 1;
    }
  }

  return NULL;
}

static Json *json_parse(Read_cursor *cursor, Arena *arena);

static Json *json_parse_array(Read_cursor *cursor, Arena *arena) {
  pg_assert(read_cursor_next(cursor) == '[');

  Json *j = arena_alloc(arena, sizeof(Json), _Alignof(Json), 1);
  *j = (Json){.kind = JSON_KIND_ARRAY};
  Json *it = NULL;

  while (!read_cursor_is_at_end(*cursor)) {
    const u8 c = read_cursor_peek(*cursor);
    if (c == ']') {
      read_cursor_next(cursor);
      return j;
    } else if (str_is_space(c)) {
      read_cursor_next(cursor);
    } else {
      Json *child = json_parse(cursor, arena);
      if (child == NULL)
        return NULL;

      const bool comma = read_cursor_match_char(cursor, ',');
      read_cursor_skip_many_spaces(cursor);

      if (comma && read_cursor_peek(*cursor) == ']') {
        return NULL;
      }

      j->v.children = j->v.children == NULL ? child : j->v.children;
      if (it == NULL) {
        it = child;
      } else {
        it = it->next = child;
      }
    }
  }

  return NULL;
}

static Json *json_parse_object(Read_cursor *cursor, Arena *arena) {
  pg_assert(read_cursor_next(cursor) == '{');

  Json *j = arena_alloc(arena, sizeof(Json), _Alignof(Json), 1);
  *j = (Json){.kind = JSON_KIND_OBJECT};
  Json *it = NULL;

  while (!read_cursor_is_at_end(*cursor)) {
    const u8 c = read_cursor_peek(*cursor);
    if (c == '}') {
      read_cursor_next(cursor);
      return j;
    } else if (str_is_space(c)) {
      read_cursor_next(cursor);
    } else {
      Json *key = json_parse_string(cursor, arena);
      if (key == NULL)
        return NULL;

      read_cursor_skip_many_spaces(cursor);

      if (!read_cursor_match_char(cursor, ':'))
        return NULL;

      Json *value = json_parse(cursor, arena);
      if (value == NULL)
        return NULL;

      const bool comma = read_cursor_match_char(cursor, ',');
      read_cursor_skip_many_spaces(cursor);

      if (comma && read_cursor_peek(*cursor) == '}')
        return NULL;

      j->v.children = j->v.children == NULL ? key : j->v.children;

      if (it == NULL) {
        it = key;
      } else {
        it = it->next = key;
      }
      it = it->next = value;
    }
  }

  return NULL;
}

static Json *json_parse(Read_cursor *cursor, Arena *arena) {
  while (!read_cursor_is_at_end(*cursor)) {
    const u8 c = read_cursor_peek(*cursor);

    if (str_is_digit(c) || c == '-') {
      return json_parse_number(cursor, arena);
    } else if (c == 't' || c == 'f') {
      return json_parse_bool(cursor, arena);
    } else if (c == '"') {
      return json_parse_string(cursor, arena);
    } else if (c == '[') {
      return json_parse_array(cursor, arena);
    } else if (c == '{') {
      return json_parse_object(cursor, arena);
    } else if (str_is_space(c)) {
      read_cursor_next(cursor);
    } else {
      return NULL;
    }
  }
  return NULL;
}

__attribute__((warn_unused_result)) static Str_builder
json_format_do(const Json *j, Str_builder sb, usize indent, Arena *arena) {
  if (j == NULL)
    return sb;

  switch (j->kind) {
  case JSON_KIND_UNDEFINED:
    pg_assert(0 && "unreachable");
  case JSON_KIND_BOOL:
    return sb_append(sb, str_from_c(j->v.boolean ? "true" : "false"), arena);
  case JSON_KIND_NUMBER:
    // TODO: Format double
    return sb_append_u64(sb, (u64)j->v.number, arena);
  case JSON_KIND_STRING:
    sb = sb_append_char(sb, '"', arena);
    sb = sb_append(sb, j->v.string, arena);
    return sb_append_char(sb, '"', arena);
  case JSON_KIND_ARRAY: {
    if (!j->v.children) {
      return sb_append(sb, str_from_c("[]"), arena);
    }

    sb = sb_append(sb, str_from_c("[\n"), arena);

    Json *it = j->v.children;
    while (it != NULL) {
      sb = sb_append_many(sb, ' ', indent + 2, arena);
      sb = json_format_do(it, sb, indent + 2, arena);
      it = it->next;

      if (it)
        sb = sb_append(sb, str_from_c(",\n"), arena);
    }

    sb = sb_append_char(sb, '\n', arena);
    sb = sb_append_many(sb, ' ', indent, arena);
    sb = sb_append(sb, str_from_c("]"), arena);
    return sb;
  }
  case JSON_KIND_OBJECT: {
    if (!j->v.children) {
      return sb_append(sb, str_from_c("{}"), arena);
    }

    sb = sb_append(sb, str_from_c("{\n"), arena);

    Json *it = j->v.children;
    while (it != NULL) {
      sb = sb_append_many(sb, ' ', indent + 2, arena);
      sb = json_format_do(it, sb, indent + 2, arena);

      it = it->next;

      sb = sb_append(sb, str_from_c(": "), arena);

      sb = json_format_do(it, sb, indent + 2, arena);
      it = it->next;

      if (it)
        sb = sb_append(sb, str_from_c(",\n"), arena);
    }
    sb = sb_append_char(sb, '\n', arena);
    sb = sb_append_many(sb, ' ', indent, arena);
    sb = sb_append(sb, str_from_c("}"), arena);
    return sb;
  }
  }
}

__attribute__((warn_unused_result)) static Str json_format(const Json *j,
                                                           Arena *arena) {
  if (j == NULL)
    return (Str){0};

  Str_builder sb = sb_new(1024, arena);
  return sb_build(json_format_do(j, sb, 0, arena));
}

static void test_json_parse(void) {
  {
    const Str in = str_from_c("xxx");
    u8 mem[256] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    const Json *const j = json_parse(&cursor, &arena);
    pg_assert(j == NULL);
  }
  {
    const Str in = str_from_c("123");
    u8 mem[256] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    const Json *const j = json_parse(&cursor, &arena);
    pg_assert(j != NULL);
    pg_assert(j->kind == JSON_KIND_NUMBER);
    pg_assert((u64)j->v.number == 123);
  }
  {
    const Str in = str_from_c("-123");
    u8 mem[256] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    const Json *const j = json_parse(&cursor, &arena);
    pg_assert(j != NULL);
    pg_assert(j->kind == JSON_KIND_NUMBER);
    pg_assert((i64)j->v.number == -123);
  }
  {
    const Str in = str_from_c("true");
    u8 mem[256] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    const Json *const j = json_parse(&cursor, &arena);
    pg_assert(j != NULL);
    pg_assert(j->kind == JSON_KIND_BOOL);
    pg_assert(j->v.boolean == true);
  }
  {
    const Str in = str_from_c("false");
    u8 mem[256] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    const Json *const j = json_parse(&cursor, &arena);
    pg_assert(j != NULL);
    pg_assert(j->kind == JSON_KIND_BOOL);
    pg_assert(j->v.boolean == false);
  }
  {
    const Str in = str_from_c("\"foo\"");
    u8 mem[256] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    const Json *const j = json_parse(&cursor, &arena);
    pg_assert(j != NULL);
    pg_assert(j->kind == JSON_KIND_STRING);
    pg_assert(str_eq_c(j->v.string, "foo"));
  }
  {
    const Str in = str_from_c("\"foo\\\"bar\"");
    u8 mem[256] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    const Json *const j = json_parse(&cursor, &arena);
    pg_assert(j != NULL);
    pg_assert(j->kind == JSON_KIND_STRING);
    pg_assert(str_eq_c(j->v.string, "foo\\\"bar"));
  }
  {
    const Str in = str_from_c("[]");
    u8 mem[256] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    const Json *const j = json_parse(&cursor, &arena);
    pg_assert(j != NULL);
    pg_assert(j->kind == JSON_KIND_ARRAY);
    pg_assert(j->v.children == NULL);
    pg_assert(j->v.children == NULL);
  }
  {
    const Str in = str_from_c(" [ 12 ] ");
    u8 mem[256] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    const Json *const j = json_parse(&cursor, &arena);
    pg_assert(j != NULL);
    pg_assert(j->kind == JSON_KIND_ARRAY);
    pg_assert(j->next == NULL);

    Json *child = j->v.children;
    pg_assert(child != NULL);
    pg_assert(child->kind == JSON_KIND_NUMBER);
    pg_assert((u64)child->v.number == 12);
    pg_assert(child->next == NULL);
  }
  {
    const Str in = str_from_c("[12, ]");
    u8 mem[256] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    const Json *const j = json_parse(&cursor, &arena);
    pg_assert(j == NULL);
  }
  {
    const Str in = str_from_c("[12, 3]");
    u8 mem[256] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    const Json *const j = json_parse(&cursor, &arena);
    pg_assert(j != NULL);
    pg_assert(j->kind == JSON_KIND_ARRAY);
    pg_assert(j->next == NULL);

    Json *first_child = j->v.children;
    pg_assert(first_child != NULL);
    pg_assert(first_child->kind == JSON_KIND_NUMBER);
    pg_assert((u64)first_child->v.number == 12);

    Json *second_child = first_child->next;
    pg_assert(second_child != NULL);
    pg_assert(second_child->kind == JSON_KIND_NUMBER);
    pg_assert((u64)second_child->v.number == 3);
    pg_assert(second_child->next == NULL);
  }
  {
    const Str in = str_from_c("{}");
    u8 mem[256] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    const Json *const j = json_parse(&cursor, &arena);
    pg_assert(j != NULL);
    pg_assert(j->kind == JSON_KIND_OBJECT);
    pg_assert(j->v.children == NULL);
    pg_assert(j->v.children == NULL);
  }
  {
    const Str in = str_from_c(" { \"abc\" : 12 } ");
    u8 mem[256] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    const Json *const j = json_parse(&cursor, &arena);
    pg_assert(j != NULL);
    pg_assert(j->kind == JSON_KIND_OBJECT);
    pg_assert(j->next == NULL);

    Json *key = j->v.children;
    pg_assert(key != NULL);
    pg_assert(key->kind == JSON_KIND_STRING);
    pg_assert(str_eq_c(key->v.string, "abc"));

    Json *value = key->next;
    pg_assert(value != NULL);
    pg_assert((u64)value->v.number == 12);
    pg_assert(value->next == NULL);
  }
  {
    const Str in = str_from_c("{ \"abc\" : 12, }");
    u8 mem[256] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    const Json *const j = json_parse(&cursor, &arena);
    pg_assert(j == NULL);
  }
  {
    const Str in = str_from_c("{ 13 : 12 }");
    u8 mem[256] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    const Json *const j = json_parse(&cursor, &arena);
    pg_assert(j == NULL);
  }
  {
    const Str in = str_from_c("{ : 12 }");
    u8 mem[256] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    const Json *const j = json_parse(&cursor, &arena);
    pg_assert(j == NULL);
  }
  {
    const Str in = str_from_c("{  12 }");
    u8 mem[256] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    const Json *const j = json_parse(&cursor, &arena);
    pg_assert(j == NULL);
  }
  {
    const Str in = str_from_c(
        "{ \"foo\": 12, \"bar\" : [true, false], \"baz\": \"hello\" }");
    u8 mem[1024] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    const Json *const j = json_parse(&cursor, &arena);
    pg_assert(j != NULL);
    pg_assert(j->kind == JSON_KIND_ARRAY);
    pg_assert(j->next == NULL);

    Json *first_key = j->v.children;
    pg_assert(first_key != NULL);
    pg_assert(first_key->kind == JSON_KIND_STRING);
    pg_assert(str_eq_c(first_key->v.string, "foo"));

    Json *first_value = first_key->next;
    pg_assert(first_value != NULL);
    pg_assert(first_value->kind == JSON_KIND_NUMBER);
    pg_assert((u64)first_value->v.number == 12);

    Json *second_key = first_value->next;
    pg_assert(second_key != NULL);
    pg_assert(second_key->kind == JSON_KIND_STRING);
    pg_assert(str_eq_c(second_key->v.string, "bar"));

    Json *second_value = second_key->next;
    pg_assert(second_value != NULL);
    pg_assert(second_value->kind == JSON_KIND_ARRAY);

    Json *first_array_element = second_value->v.children;
    pg_assert(first_array_element != NULL);
    pg_assert(first_array_element->kind == JSON_KIND_BOOL);
    pg_assert(first_array_element->v.boolean == true);

    Json *second_array_element = first_array_element->next;
    pg_assert(second_array_element != NULL);
    pg_assert(second_array_element->kind == JSON_KIND_BOOL);
    pg_assert(second_array_element->v.boolean == false);

    Json *third_key = second_value->next;
    pg_assert(third_key != NULL);
    pg_assert(third_key->kind == JSON_KIND_STRING);
    pg_assert(str_eq_c(third_key->v.string, "baz"));

    Json *third_value = third_key->next;
    pg_assert(third_value != NULL);
    pg_assert(third_value->kind == JSON_KIND_STRING);
    pg_assert(str_eq_c(third_value->v.string, "hello"));
  }
  {
    const Str in = str_from_c(
        "{ \"foo\": 12, \"bar\" : [true, false], \"baz\": \"hello\" }");
    u8 mem[4096] = {0};
    Arena arena = arena_from_mem(mem, sizeof(mem));
    Read_cursor cursor = {.s = in};

    const Json *const j = json_parse(&cursor, &arena);
    const Str out = json_format(j, &arena);

    pg_assert(str_eq_c(out, "{\n"
                            "  \"foo\": 12,\n"
                            "  \"bar\": [\n"
                            "    true,\n"
                            "    false\n"
                            "  ],\n"
                            "  \"baz\": \"hello\"\n"
                            "}"));
  }
}
