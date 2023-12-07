#include "sys.h"

void _start() {
  uint8_t buf[256] = {0};
  const ssize_t read_n = read(0, buf, sizeof(buf));
  assert(read_n > 0);

  write(1, buf, read_n);
  exit(0);
}
