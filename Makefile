SRC := main.c sys.h http.h

CC = clang

# Assume clang for cross compilation.
MY_CFLAGS := -Wall -Wextra -Wpadded -Wconversion -std=c99 --target=armv6-linux-none -static -g3 -fuse-ld=lld -nostdlib -mfloat-abi=hard -march=armv6

main_debug: $(SRC)
	$(CC) $(MY_CFLAGS) main.c -o main_debug -O0

main: $(SRC)
	$(CC) $(MY_CFLAGS) main.c -o main -Ofast -march=armv6

debug_main:
	qemu-arm -g 1234 ./main &
	gdb-multiarch -q --nh -ex 'set architecture arm' -ex 'file main' -ex 'target remote localhost:1234' -tui         
	pkill qemu

debug_main_debug:
	qemu-arm -g 1234 ./main_debug &
	gdb-multiarch -q --nh -ex 'set architecture arm' -ex 'file main_debug' -ex 'target remote localhost:1234' -tui         
	pkill qemu


.PHONY: debug
