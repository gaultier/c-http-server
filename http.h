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

typedef struct {
  Method method;
  Str path;
  Str body;

} Request;
