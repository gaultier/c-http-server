#include "arena.h"
#include "http.h"
#include "str.h"
#include "sys.h"

#define fd_puts(fd, msg)                                                       \
  do {                                                                         \
    write(fd, msg, sizeof(msg) - 1);                                           \
  } while (0)

static Response handler(Request req) { return (Response){.status = 200}; }

static void worker(int client_socket) {
  Arena arena = arena_new(4 * KiB, NULL);

  Str_builder in_buffer = sb_new(2 * KiB, &arena);
  const Read_result read_res =
      ut_read_from_fd_until(client_socket, in_buffer, str_from_c("\r\n\r\n"));
  const Request req = parse_request(read_res);
  if (req.error) {
    return;
  }

  const Response res = handler(req);
  fd_puts(client_socket, "HTTP/1.1 200 OK\r\nContent-Length:6\r\n\r\nHello!");

  return;
}

int main() {
  const int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  assert(server_socket >= 0);

  {
    int val = 1;
    assert(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &val,
                      sizeof(val)) != -1);
  }
  {
    int val = 1;
    assert(setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT, &val,
                      sizeof(val)) != -1);
  }

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
