SRC := main.c sys.h http.h

CC = clang

# Assume clang for cross compilation.
MY_CFLAGS_COMMON := -Wall -Wextra -Wpadded -Wconversion -std=c99 -static -g3

main: $(SRC)
	$(CC) $(MY_CFLAGS_COMMON) main.c -o main -Ofast

main_debug: $(SRC)
	$(CC) $(MY_CFLAGS_COMMON) main.c -o main_debug -O0

all: main main_debug

debug_main:
	qemu-arm -g 1234 ./main &
	gdb-multiarch -q --nh -ex 'set architecture arm' -ex 'file main' -ex 'target remote localhost:1234' -tui         
	pkill qemu

debug_main_debug:
	qemu-arm -g 1234 ./main_debug &
	gdb-multiarch -q --nh -ex 'set architecture arm' -ex 'file main_debug' -ex 'target remote localhost:1234' -tui         
	pkill qemu


.PHONY: debug all
