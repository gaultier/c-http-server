#pragma once

typedef int ssize_t;
typedef unsigned size_t;
typedef unsigned char uint8_t;
typedef unsigned uint32_t;
typedef unsigned short uint16_t;

/* Avoid use of r7 in asm constraints when producing thumb code,
 * since it's reserved as frame pointer and might not be supported. */
#define __ASM____R7__
#define __asm_syscall(...)                                                     \
  do {                                                                         \
    __asm__ __volatile__("mov %1,r7 ; mov r7,%2 ; svc 0 ; mov r7,%1"           \
                         : "=r"(r0), "=&r"((int){0})                           \
                         : __VA_ARGS__                                         \
                         : "memory");                                          \
    return r0;                                                                 \
  } while (0)

/* For thumb2, we can allow 8-bit immediate syscall numbers, saving a
 * register in the above dance around r7. Does not work for thumb1 where
 * only movs, not mov, supports immediates, and we can't use movs because
 * it doesn't support high regs. */
#ifdef __thumb2__
#define R7_OPERAND "rI"(r7)
#else
#define R7_OPERAND "r"(r7)
#endif

static inline long __syscall0(long n) {
  register long r7 __ASM____R7__ = n;
  register long r0 __asm__("r0");
  __asm_syscall(R7_OPERAND);
}

static inline long __syscall1(long n, long a) {
  register long r7 __ASM____R7__ = n;
  register long r0 __asm__("r0") = a;
  __asm_syscall(R7_OPERAND, "0"(r0));
}

static inline long __syscall2(long n, long a, long b) {
  register long r7 __ASM____R7__ = n;
  register long r0 __asm__("r0") = a;
  register long r1 __asm__("r1") = b;
  __asm_syscall(R7_OPERAND, "0"(r0), "r"(r1));
}

static inline long __syscall3(long n, long a, long b, long c) {
  register long r7 __ASM____R7__ = n;
  register long r0 __asm__("r0") = a;
  register long r1 __asm__("r1") = b;
  register long r2 __asm__("r2") = c;
  __asm_syscall(R7_OPERAND, "0"(r0), "r"(r1), "r"(r2));
}

static inline long __syscall4(long n, long a, long b, long c, long d) {
  register long r7 __ASM____R7__ = n;
  register long r0 __asm__("r0") = a;
  register long r1 __asm__("r1") = b;
  register long r2 __asm__("r2") = c;
  register long r3 __asm__("r3") = d;
  __asm_syscall(R7_OPERAND, "0"(r0), "r"(r1), "r"(r2), "r"(r3));
}

static inline long __syscall5(long n, long a, long b, long c, long d, long e) {
  register long r7 __ASM____R7__ = n;
  register long r0 __asm__("r0") = a;
  register long r1 __asm__("r1") = b;
  register long r2 __asm__("r2") = c;
  register long r3 __asm__("r3") = d;
  register long r4 __asm__("r4") = e;
  __asm_syscall(R7_OPERAND, "0"(r0), "r"(r1), "r"(r2), "r"(r3), "r"(r4));
}

static inline long __syscall6(long n, long a, long b, long c, long d, long e,
                              long f) {
  register long r7 __ASM____R7__ = n;
  register long r0 __asm__("r0") = a;
  register long r1 __asm__("r1") = b;
  register long r2 __asm__("r2") = c;
  register long r3 __asm__("r3") = d;
  register long r4 __asm__("r4") = e;
  register long r5 __asm__("r5") = f;
  __asm_syscall(R7_OPERAND, "0"(r0), "r"(r1), "r"(r2), "r"(r3), "r"(r4),
                "r"(r5));
}

#define SYS_WRITE 4
static ssize_t write(int fd, const void *buf, size_t count) {
  return (ssize_t)__syscall3(SYS_WRITE, fd, (long)buf, (long)count);
}

#define SYS_READ 3
static ssize_t read(int fd, void *buf, size_t count) {
  return (ssize_t)__syscall3(SYS_READ, fd, (long)buf, (long)count);
}

#define SYS_EXIT 1
static void exit(int status) {
  for (;;)
    __syscall1(SYS_EXIT, status);
}

#define SYS_KILL 37
static int kill(int pid, int sig) { return __syscall2(SYS_KILL, pid, sig); }

#define SYS_SIGABRT 6
static void abort(void) {
  for (;;)
    kill(0, SYS_SIGABRT);
}

#define assert(cond) (cond) ? (void)0 : abort()

static void *memset(void *s, int c, size_t n) {
  for (size_t i = 0; i < n; i++)
    ((uint8_t *)s)[i] = (uint8_t)c;
  return s;
}

static void *memcpy(void *restrict dst, const void *restrict src, size_t n) {
  for (size_t i = 0; i < n; i++)
    ((uint8_t *)dst)[i] = ((uint8_t *)src)[i];
  return dst;
}

#define SYS_SOCKET 281
#define AF_INET 2
#define SOCK_STREAM 1
static int socket(int domain, int type, int protocol) {
  return __syscall6(SYS_SOCKET, domain, type, protocol, 0, 0, 0);
}

#define SYS_BIND 282
struct sockaddr {
  uint16_t sin_family, sin_port;
  uint32_t sin_addr;
  uint8_t sin_zero[8];
};

static int bind(int sockfd, const struct sockaddr *addr, uint32_t addrlen) {
  return __syscall6(SYS_BIND, sockfd, (long)addr, (long)addrlen, 0, 0, 0);
}

#define SYS_LISTEN 284
static int listen(int sockfd, int backlog) {
  return __syscall6(SYS_LISTEN, sockfd, backlog, 0, 0, 0, 0);
}

#define SYS_ACCEPT 285
static int accept(int sockfd, struct sockaddr *restrict addr,
                  uint32_t addrlen) {
  return __syscall6(SYS_ACCEPT, sockfd, (long)addr, (long)addrlen, 0, 0, 0);
}

#define NULL 0
#define stdin 0
#define stdout 1
#define stderr 2

#define SYS_FORK 2
static int fork(void) { return __syscall0(SYS_FORK); }

#define SYS_CLOSE 6
static int close(int fd) { return __syscall1(SYS_CLOSE, fd); }
