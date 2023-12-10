#pragma once

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
  Str path;
  Str body;
} Request;

// TODO: headers.
typedef struct {
  u16 status;
  Str body;
} Response;
