SRC := main.c http.h array.h arena.h str.h

# Assume clang for cross compilation.
MY_CFLAGS_COMMON := -Wall -Wextra -Wpadded -Wconversion -Wno-gnu-alignof-expression -Wno-unused-function -std=c99 -g3

all: main main_debug main_debug_san

main: $(SRC)
	$(CC) $(MY_CFLAGS_COMMON) $(CFLAGS) main.c -o main -Ofast -static

main_debug: $(SRC)
	$(CC) $(MY_CFLAGS_COMMON) $(CFLAGS) main.c -o main_debug -O0

main_debug_san: $(SRC)
	$(CC) $(MY_CFLAGS_COMMON) $(CFLAGS) -fsanitize=address,undefined main.c -o main_debug_san -O0

.PHONY: all
