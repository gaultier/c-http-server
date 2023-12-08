#include "sys.h"

uint16_t u16_to_be(uint16_t n) { return ((n & 0xff00) << 8) | (n & 0xff); }
uint32_t u32_to_be(uint32_t n) {
  return ((n & 0xff000000) << 24) | (n & 0x00ff0000) << 16 |
         ((n & 0x0000ff00) << 8) | (n & 0xff);
}

static void print_err(char *err) { write(stderr, err, sizeof(err)); }

void _start() {
  const int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  assert(server_socket >= 0);

  const uint16_t port = 12345;

  const struct sockaddr addr = {
      .sin_family = AF_INET,
      .sin_addr = 0,
      .sin_port = u16_to_be(port),
  };
  assert(bind(server_socket, &addr, sizeof(addr)) == 0);

  // Will anyways probably be capped to 128 by the kernel depending on the
  // version (see man page).
  const int backlog = 4096;
  assert(listen(server_socket, backlog) == 0);
  // ssize_t write_n = write(1, "Hello", 5);

  const int client_socket = accept(server_socket, NULL, 0);
  if (client_socket <= 0) {
    print_err("Failed to accept(2)\n");
  }
  exit(0);
}
