#include "arena.h"
#include "json.h"
#include "str.h"

int main(int argc, char *argv[]) {
  pg_assert(argc == 2);

  Arena arena = arena_new(4 * MiB, NULL);
  Read_result read_res = ut_file_read_all(argv[1], &arena);
  pg_assert(!read_res.error);

  Read_cursor cursor = {.s = read_res.content};
  Json *j = json_parse(&cursor, &arena);
  Str s = json_format(j, &arena);
  pg_unused(s);
}
