#include "arena.h"
#include "http.h"
#include "str.h"

#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

static Response handler(Request req, Arena *arena) {
  return (Response){
      .status = 200,
      .body = str_clone(req.path, arena),
  };
}

static void worker_signal_handler(int signo) {
  if (signo == SIGALRM)
    abort();
}

static void worker(int client_socket) {
  // Abort on SIGALRM
  const struct sigaction action = {.sa_handler = worker_signal_handler};
  assert(sigaction(SIGALRM, &action, NULL) != -1);

  // Send SIGALRM on timer expiration to implement worker timeout.
  const struct itimerval timer = {.it_value = {.tv_sec = 10}};
  pg_assert(setitimer(ITIMER_REAL, &timer, NULL) == 0);

  Arena arena = arena_new(4 * KiB, NULL);

  Str_builder in_buffer = sb_new(1 * KiB, &arena);
  const Read_result read_res =
      ut_read_from_fd_until(client_socket, in_buffer, str_from_c("\r\n\r\n"));
  const Request req = parse_request(read_res);
  if (req.error) {
    return;
  }

  const Response res = handler(req, &arena);
  const Str res_str = response_to_str(res, &arena);
  int _err = ut_write_all(client_socket, res_str);
  pg_unused(_err); // Nothing to do.

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

  fprintf(stderr, "Listening to: 0.0.0.0:4096\n");

  for (;;) {
    const int client_socket = accept(server_socket, NULL, 0);
    if (client_socket <= 0) {
      fprintf(stderr, "Failed to accept(2)\n");
      continue;
    }

    const int pid = fork();
    if (pid == -1) {
      fprintf(stderr, "Failed to fork(2)\n");
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
