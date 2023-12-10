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

static bool read_cursor_match_any(Read_cursor *self, Str *needles,
                                  usize needles_len) {
  for (usize i = 0; i < needles_len; i++) {
    if (read_cursor_match(self, needles[i])) {
      return true;
    }
  }
  return false;
}
