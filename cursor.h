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
cursor_read_remaining(Read_cursor self) {
  return str_advance(self.s, self.pos);
}

static bool read_cursor_match(Read_cursor *self, Str needle) {
  if (read_cursor_is_at_end(*self))
    return false;

  const Str remaining = cursor_read_remaining(*self);
  if (str_eq(remaining, needle)) {
    self->s = str_advance(remaining, needle.len);
    return true;
  }

  return false;
}

static bool read_cursor_match_any(Read_cursor *self, const Str *needles,
                                  usize needles_len) {
  for (usize i = 0; i < needles_len; i++) {
    if (read_cursor_match(self, needles[i])) {
      return true;
    }
  }
  return false;
}

static Str read_cursor_match_until_excl(Read_cursor *self, u8 c) {
  const Str remaining = cursor_read_remaining(*self);
  for (usize i = 0; i < remaining.len; i++) {
    if (remaining.data[i] == c) {
      return str_advance(remaining, i);
    }
  }
  return (Str){0};
}
