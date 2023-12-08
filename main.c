#include "sys.h"

void _start() {
  const int sock = socket(AF_INET, SOCK_STREAM, 0);
  assert(sock != -1);
 // ssize_t write_n = write(1, "Hello", 5);
  exit(0);
}
