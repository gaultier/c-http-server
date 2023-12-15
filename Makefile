SRC := main.c http.h array.h arena.h str.h

# Assume clang for cross compilation.
MY_CFLAGS_COMMON := $(shell tr < compile_flags.txt '\n' ' ') -g3

CC := clang

all: main main_debug main_debug_san

main: $(SRC)
	$(CC) $(MY_CFLAGS_COMMON) $(CFLAGS) $< -o $@ -Ofast -static

main_debug: $(SRC)
	$(CC) $(MY_CFLAGS_COMMON) $(CFLAGS) $< -o $@ -O0

main_debug_san: $(SRC)
	$(CC) $(MY_CFLAGS_COMMON) $(CFLAGS) -fsanitize=address,undefined $< -o $@ -O0

.PHONY: all
