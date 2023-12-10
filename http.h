#pragma once

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

typedef struct {
  Str key, value;
} Header;

// TODO: headers.
typedef struct {
  Method method;
  int error;
  Str hostname;
  Str path;
  Str params;
  Str body;
} Request;

// TODO: headers.
typedef struct {
  u16 status;
  pg_pad(6);
  Str body;
} Response;

// TODO: Store headers.
static bool parse_headers(Read_cursor *cursor) {
  while (!read_cursor_is_at_end(*cursor)) {
    const Str key = read_cursor_match_until_excl_char(cursor, ':');
    if (str_is_empty(key)) {
      return true;
    }

    pg_assert(read_cursor_match(cursor, str_from_c(":")));

    const Str value = read_cursor_match_until_excl(cursor, str_from_c("\r\n"));
    if (str_is_empty(value)) {
      return false;
    }
    pg_assert(read_cursor_match(cursor, str_from_c("\r\n")));

    // TODO: Left-trim value.
  }

  return true;
}

static Request parse_request(Read_result read_res) {
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

  if (!read_cursor_match(&cursor, str_from_c(" HTTP/1.1\r\n"))) {
    return (Request){.error = true};
  }

  if (!parse_headers(&cursor)) {
    return (Request){.error = true};
  }

  if (!read_cursor_match(&cursor, str_from_c("\r\n"))) {
    return (Request){.error = true};
  }

  return req;
}
