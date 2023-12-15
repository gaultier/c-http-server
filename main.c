#include "arena.h"
#include "cursor.h"
#include "http.h"
#include "json.h"
#include "str.h"

#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

static Response handler(Request req, Arena *arena) {
  Read_cursor cursor = {.s = req.body};
  Json *j = json_parse(&cursor, arena);
  if (!j) {
    return (Response){.status = 400};
  }

  const Str result = json_format(j, arena);
  return (Response){.status = 200, .body = result};
}

static void worker_signal_handler(int signo) {
  if (signo == SIGALRM)
    abort();
}

static void worker(int client_socket) {
  // Abort on SIGALRM
  const struct sigaction action = {.sa_handler = worker_signal_handler};
  pg_assert(sigaction(SIGALRM, &action, NULL) != -1);

  // Send SIGALRM on timer expiration to implement worker timeout.
  const struct itimerval timer = {.it_value = {.tv_sec = 10}};
  pg_assert(setitimer(ITIMER_REAL, &timer, NULL) == 0);

  Arena arena = arena_new(64 * KiB, NULL);

  Str_builder in_buffer = sb_new(1 * KiB, &arena);
  const Read_result read_res =
      ut_read_from_fd_until(client_socket, in_buffer, str_from_c("\r\n\r\n"));
  Request req = parse_request(read_res, &arena);
  if (req.error) {
    return;
  }

  // Read body.
  {
    const Header *content_length_header =
        http_find_header(req.headers, str_from_c("Content-Length"));
    if (content_length_header) {
      const usize announced_length = str_to_u64(content_length_header->value);
      const isize body_sep_pos =
          str_find(read_res.content, str_from_c("\r\n\r\n"));

      Str_builder body =
          sb_new(announced_length == 0 ? 16 * KiB : announced_length, &arena);
      if (body_sep_pos != -1) {
        pg_assert(body_sep_pos >= 4);
        Str body_already_read =
            str_advance(read_res.content, (usize)body_sep_pos + 4);

        pg_assert(body_already_read.len <= announced_length);

        body = sb_append(body, body_already_read, &arena);
      }

      const Read_result body_read_res = sb_read_to_fill(body, client_socket);
      pg_assert(!body_read_res.error);

      req.body = body_read_res.content;
    }
  }

  const Response res = handler(req, &arena);
  const Str res_str = response_to_str(res, &arena);
  int _err = ut_write_all(client_socket, res_str);
  pg_unused(_err); // Nothing to do.

  return;
}

int main(void) {
  const struct sigaction sa = {.sa_flags = SA_NOCLDWAIT};
  pg_assert(sigaction(SIGCHLD, &sa, NULL) != -1);

  const int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  pg_assert(server_socket >= 0);

  pg_assert(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1},
                       sizeof(int)) != -1);
#ifdef SO_REUSEPORT
  pg_assert(setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT, &(int){1},
                       sizeof(int)) != -1);
#endif

  const uint16_t port = 4096;
  const uint16_t net_port = (uint16_t)__builtin_bswap16(port);

  const struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_port = net_port,
  };
  pg_assert(bind(server_socket, (const void *)&addr, sizeof(addr)) == 0);

  // Will anyways probably be capped to 128 by the kernel depending on the
  // version (see man page).
  const int backlog = 4096;
  pg_assert(listen(server_socket, backlog) == 0);
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
