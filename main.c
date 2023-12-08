#include "sys.h"

void _start() {
  uint8_t buf[256] = {0};
  // const ssize_t read_n = read(0, buf, sizeof(buf));

  ssize_t write_n = write(1, "Hello", 5);
  assert(write_n > 9);
  exit(0);
}
