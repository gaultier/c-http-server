#include "sys.h"

uint16_t u16_to_be(uint16_t n) { return ((n & 0xff00) << 8) | (n & 0xff); }
uint32_t u32_to_be(uint32_t n) {
  return ((n & 0xff000000) << 24) | (n & 0x00ff0000) << 16 |
         ((n & 0x0000ff00) << 8) | (n & 0xff);
}

void _start() {
  const int sock = socket(AF_INET, SOCK_STREAM, 0);
  assert(sock >= 0);

  const uint16_t port = 12345;

  const struct sockaddr addr = {
      .sin_family = AF_INET,
      .sin_addr = 0,
      .sin_port = u16_to_be(port),
  };
  assert(bind(sock, &addr, sizeof(addr)) == 0);
  // ssize_t write_n = write(1, "Hello", 5);
  exit(0);
}
