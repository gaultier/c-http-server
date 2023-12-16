SRC := main.c http.h array.h arena.h str.h json.h

# Assume clang for cross compilation.
MY_CFLAGS_COMMON := $(shell tr < compile_flags.txt '\n' ' ') -g3

CC := clang

all: main main_debug main_debug_san

main: $(SRC)
	$(CC) $(MY_CFLAGS_COMMON) $(CFLAGS) $< -o $@ -Ofast -static -march=native

main_debug: $(SRC)
	$(CC) $(MY_CFLAGS_COMMON) $(CFLAGS) $< -o $@ -O0

main_debug_san: $(SRC)
	$(CC) $(MY_CFLAGS_COMMON) $(CFLAGS) $< -o $@ -O0 -fsanitize=address,undefined 

.PHONY: all
