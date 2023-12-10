#include "http.h"
#include "sys.h"

#define fd_puts(fd, msg)                                                       \
  do {                                                                         \
    write(fd, msg, sizeof(msg));                                               \
  } while (0)

static Response handler(Request req) { return (Response){.status = 200}; }

static void worker(int client_socket) {
  Request req = {0}; // TODO
  const Response res = handler(req);
  write(client_socket, "Hello!", 6);

  return;
}

int main() {
  const int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  assert(server_socket >= 0);

  const uint16_t port = 4096;
  const uint16_t net_port = (uint16_t)__builtin_bswap16(port);

  const struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_port = net_port,
  };
  assert(bind(server_socket, (void *)&addr, sizeof(addr)) == 0);

  // Will anyways probably be capped to 128 by the kernel depending on the
  // version (see man page).
  const int backlog = 4096;
  assert(listen(server_socket, backlog) == 0);
  // ssize_t write_n = write(1, "Hello", 5);

  fd_puts(FD_STDERR, "Listening to: 0.0.0.0:4096\n");

  for (;;) {
    const int client_socket = accept(server_socket, NULL, 0);
    if (client_socket <= 0) {
      fd_puts(FD_STDERR, "Failed to accept(2)\n");
      continue;
    }

    const int pid = fork();
    if (pid == -1) {
      fd_puts(FD_STDERR, "Failed to fork(2)\n");
      close(client_socket);
      continue;
    }

    if (pid == 0) { // Child.
      worker(client_socket);
      exit(0);
    } else { // Parent.
      close(client_socket);
    }
  }
}

#ifdef FREESTANDING
void _start() { exit(main()); }
#endif
