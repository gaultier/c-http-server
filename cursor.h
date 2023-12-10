#pragma once

#include "str.h"

typedef struct {
  Str s;
  usize pos;
} Read_cursor;

__attribute__((warn_unused_result)) static bool
read_cursor_is_at_end(Read_cursor self) {
  return self.pos >= self.s.len;
}

__attribute__((warn_unused_result)) static Str
read_cursor_remaining(Read_cursor self) {
  return str_advance(self.s, self.pos);
}

static bool read_cursor_match(Read_cursor *self, Str needle) {
  if (read_cursor_is_at_end(*self))
    return false;

  const Str remaining = read_cursor_remaining(*self);
  if (str_starts_with(remaining, needle)) {
    self->pos += needle.len;
    return true;
  }

  return false;
}

static Str read_cursor_match_until_excl(Read_cursor *self, Str needle) {
  const Str remaining = read_cursor_remaining(*self);
  const isize pos = str_find(remaining, needle);
  if (pos != -1) {
    self->pos += (usize)pos;
    return (Str){.data = remaining.data, .len = (usize)pos};
  }
  return (Str){0};
}

static Str read_cursor_match_until_excl_char(Read_cursor *self, u8 c) {
  const Str remaining = read_cursor_remaining(*self);
  Str_split_result split = str_split(remaining, c);
  if (split.found) {
    self->pos += split.found_pos;
    return split.left;
  }
  return (Str){0};
}
