#include "sys.h"

void _start() {
  uint8_t buf[256] = {0}; 
  const ssize_t read_n = read(0, buf, sizeof(buf));
  write(1, "Hello!", 6);
  exit(0);
}
