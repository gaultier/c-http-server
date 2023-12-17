#pragma once

#include "arena.h"
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

static bool read_cursor_match(Read_cursor *_Nonnull self, Str needle) {
  if (read_cursor_is_at_end(*self))
    return false;

  const Str remaining = read_cursor_remaining(*self);
  if (str_starts_with(remaining, needle)) {
    self->pos += needle.len;
    return true;
  }

  return false;
}

static Str read_cursor_match_until_excl(Read_cursor *_Nonnull self,
                                        Str needle) {
  const Str remaining = read_cursor_remaining(*self);
  const isize pos = str_find(remaining, needle);
  if (pos != -1) {
    self->pos += (usize)pos;
    return (Str){.data = remaining.data, .len = (usize)pos};
  }
  return (Str){0};
}

static Str read_cursor_match_until_excl_char(Read_cursor *_Nonnull self, u8 c) {
  const Str remaining = read_cursor_remaining(*self);
  Str_split_result split = str_split(remaining, c);
  if (split.found) {
    self->pos += split.found_pos;
    return split.left;
  }
  return (Str){0};
}

static u8 read_cursor_peek(Read_cursor self) {
  return read_cursor_is_at_end(self) ? 0 : self.s.data[self.pos];
}

static u8 read_cursor_next(Read_cursor *_Nonnull self) {
  return read_cursor_is_at_end(*self) ? 0 : self->s.data[self->pos++];
}

static bool read_cursor_match_char_oneof(Read_cursor *_Nonnull self,
                                         Str needles, u8 *_Nonnull matched) {
  if (read_cursor_is_at_end(*self))
    return false;

  const u8 c = read_cursor_peek(*self);
  for (u64 i = 0; i < needles.len; i++) {
    if (c == needles.data[i]) {
      *matched = c;
      read_cursor_next(self);
      return true;
    }
  }

  return false;
}

static bool read_cursor_match_char(Read_cursor *_Nonnull self, u8 c) {
  return read_cursor_match_char_oneof(self, (Str){.data = &c, .len = 1},
                                      &(u8){0});
}

static void read_cursor_skip_many_spaces(Read_cursor *_Nonnull self) {
  while (!read_cursor_is_at_end(*self)) {
    const u8 c = read_cursor_peek(*self);
    if (char_is_space(c)) {
      read_cursor_next(self);
    } else {
      return;
    }
  }
}

__attribute__((warn_unused_result)) static Unicode_character
read_cursor_utf8_rune(Read_cursor *_Nonnull self) {
  if (read_cursor_is_at_end(*self))
    return (Unicode_character){0};

  const u8 c1 = read_cursor_next(self);

  switch (utf8_rune_announced_length(c1)) {
  case 0:
    return (Unicode_character){0};
  case 1:
    return (Unicode_character){.data = {c1}, .len = 1};
  case 2: {
    if (read_cursor_is_at_end(*self))
      return (Unicode_character){0};

    const u8 c2 = read_cursor_next(self);
    if (!utf8_is_continuation_byte(c2))
      return (Unicode_character){0};

    return (Unicode_character){.data = {c1, c2}, .len = 2};
  }
  case 3: {
    if (read_cursor_is_at_end(*self))
      return (Unicode_character){0};

    const u8 c2 = read_cursor_next(self);
    if (!utf8_is_continuation_byte(c2))
      return (Unicode_character){0};

    if (read_cursor_is_at_end(*self))
      return (Unicode_character){0};

    const u8 c3 = read_cursor_next(self);
    if (!utf8_is_continuation_byte(c2))
      return (Unicode_character){0};

    return (Unicode_character){.data = {c1, c2, c3}, .len = 3};
  }
  case 4: {
    if (read_cursor_is_at_end(*self))
      return (Unicode_character){0};

    const u8 c2 = read_cursor_next(self);
    if (!utf8_is_continuation_byte(c2))
      return (Unicode_character){0};

    if (read_cursor_is_at_end(*self))
      return (Unicode_character){0};

    const u8 c3 = read_cursor_next(self);
    if (!utf8_is_continuation_byte(c2))
      return (Unicode_character){0};

    if (read_cursor_is_at_end(*self))
      return (Unicode_character){0};

    const u8 c4 = read_cursor_next(self);
    if (!utf8_is_continuation_byte(c2))
      return (Unicode_character){0};

    return (Unicode_character){.data = {c1, c2, c3, c4}, .len = 4};
  }
  default:
    return (Unicode_character){0};
  }
}
