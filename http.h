#pragma once

#include "cursor.h"
#include "str.h"

typedef enum {
  Get,
  Post,
  Put,
  Patch,
  Head,
  Delete,
} Method;

typedef struct {
  Str key, value;
} Header;

// TODO: headers.
typedef struct {
  Method method;
  int error;
  Str path;
  Str body;
} Request;

// TODO: headers.
typedef struct {
  u16 status;
  Str body;
} Response;

static Request parse_request(Read_result read_res) {
  if (read_res.error) {
    return (Request){.error = read_res.error};
  }
  Read_cursor cursor = {0};
}
