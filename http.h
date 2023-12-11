#pragma once

#include "arena.h"
#include "cursor.h"
#include "str.h"

typedef enum {
  HTTP_METHOD_GET,
  HTTP_METHOD_POST,
  HTTP_METHOD_PUT,
  HTTP_METHOD_PATCH,
  HTTP_METHOD_HEAD,
  HTTP_METHOD_DELETE,
  HTTP_METHOD_TRACE,
  HTTP_METHOD_CONNECT,
} Method;

typedef struct Header Header;
struct Header {
  Str key, value;
  Header *next;
};

// TODO: headers.
typedef struct {
  Method method;
  int error;
  Str path;
  Str params;
  Str body;
  Header *headers;
} Request;

// TODO: headers.
typedef struct {
  u16 status;
  pg_pad(6);
  Header *headers;
  Str body;
} Response;

// TODO: Store headers.
__attribute__((warn_unused_result)) static Request
parse_headers(Read_cursor *cursor, Request req, Arena *arena) {
  Header *it = NULL;

  while (!read_cursor_is_at_end(*cursor)) {
    const Str key = read_cursor_match_until_excl_char(cursor, ':');
    if (str_is_empty(key)) {
      return req;
    }

    pg_assert(read_cursor_match(cursor, str_from_c(":")));

    Str value = read_cursor_match_until_excl(cursor, str_from_c("\r\n"));
    if (str_is_empty(value)) {
      return (Request){.error = true};
    }
    value = str_trim_left(value, ' ');

    pg_assert(read_cursor_match(cursor, str_from_c("\r\n")));

    Header *header = arena_alloc(arena, sizeof(Header), _Alignof(Header), 1);
    *header = (Header){.key = key, .value = value};

    if (req.headers == NULL) {
      it = req.headers = header;
    }

    it = it->next = header;
  }

  return req;
}

__attribute__((warn_unused_result)) static Request
parse_request(Read_result read_res, Arena *arena) {
  if (read_res.error) {
    return (Request){.error = read_res.error};
  }

  Read_cursor cursor = {.s = read_res.content};
  Request req = {0};

  {
    if (read_cursor_match(&cursor, str_from_c("GET"))) {
      req.method = HTTP_METHOD_GET;
    } else if (read_cursor_match(&cursor, str_from_c("POST"))) {
      req.method = HTTP_METHOD_POST;
    } else if (read_cursor_match(&cursor, str_from_c("PATCH"))) {
      req.method = HTTP_METHOD_PATCH;
    } else if (read_cursor_match(&cursor, str_from_c("PUT"))) {
      req.method = HTTP_METHOD_PUT;
    } else if (read_cursor_match(&cursor, str_from_c("DELETE"))) {
      req.method = HTTP_METHOD_DELETE;
    } else if (read_cursor_match(&cursor, str_from_c("HEAD"))) {
      req.method = HTTP_METHOD_HEAD;
    } else if (read_cursor_match(&cursor, str_from_c("CONNECT"))) {
      req.method = HTTP_METHOD_CONNECT;
    } else if (read_cursor_match(&cursor, str_from_c("TRACE"))) {
      req.method = HTTP_METHOD_TRACE;
    } else {
      return (Request){.error = true};
    }
  }

  if (!read_cursor_match(&cursor, str_from_c(" "))) {
    return (Request){.error = true};
  }

  const Str url = read_cursor_match_until_excl_char(&cursor, ' ');
  if (str_is_empty(url)) {
    return (Request){.error = true};
  }
  // TODO: Parse url.
  req.path = req.params = url; // FIXME

  if (!read_cursor_match(&cursor, str_from_c(" HTTP/1.1\r\n"))) {
    return (Request){.error = true};
  }

  req = parse_headers(&cursor, req, arena);
  if (req.error) {
    return (Request){.error = true};
  }

  if (!read_cursor_match(&cursor, str_from_c("\r\n"))) {
    return (Request){.error = true};
  }

  return req;
}

__attribute__((warn_unused_result)) static Str status_to_str(u16 status) {
  switch (status) {
  case 200:
    return str_from_c("200 OK");
  case 404:
    return str_from_c("404 Not Found");
  case 500:
    return str_from_c("500 Server Error");
  default:
    pg_assert(0 && "todo");
  }
}

__attribute__((warn_unused_result)) static Str response_to_str(Response res,
                                                               Arena *arena) {
  Str_builder out = sb_new(1 * KiB, arena);
  {
    out = sb_append(out, str_from_c("HTTP/1.1 "), arena);
    out = sb_append(out, status_to_str(res.status), arena);
    out = sb_append(out, str_from_c("\r\n"), arena);
  }

  {
    out = sb_append(out, str_from_c("Content-Length:"), arena);
    out = sb_append_u64(out, res.body.len, arena);
    out = sb_append(out, str_from_c("\r\n"), arena);
  }

  out = sb_append(out, str_from_c("\r\n"), arena);
  out = sb_append(out, res.body, arena);

  return sb_build(out);
}
