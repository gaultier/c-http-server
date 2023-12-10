SRC := main.c sys.h http.h array.h arena.h str.h

# Assume clang for cross compilation.
MY_CFLAGS_COMMON := -Wall -Wextra -Wpadded -Wconversion -std=c99 -g3

all: main main_debug main_debug_san

main: $(SRC)
	$(CC) $(MY_CFLAGS_COMMON) $(CFLAGS) main.c -o main -Ofast -static

main_debug: $(SRC)
	$(CC) $(MY_CFLAGS_COMMON) $(CFLAGS) main.c -o main_debug -O0

main_debug_san: $(SRC)
	$(CC) $(MY_CFLAGS_COMMON) $(CFLAGS) -fsanitize=address,undefined main.c -o main_debug_san -O0

debug_main:
	qemu-arm -g 1234 ./main &
	gdb-multiarch -q --nh -ex 'set architecture arm' -ex 'file main' -ex 'target remote localhost:1234' -tui         
	pkill qemu

debug_main_debug:
	qemu-arm -g 1234 ./main_debug &
	gdb-multiarch -q --nh -ex 'set architecture arm' -ex 'file main_debug' -ex 'target remote localhost:1234' -tui         
	pkill qemu


.PHONY: debug all
