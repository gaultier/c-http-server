SRC := main.c sys.h http.h

CC = clang

# Assume clang for cross compilation.
MY_CFLAGS := -Wall -Wextra -std=c99 --target=armv6-linux-none -static -g3 -fuse-ld=lld -nostdlib -mfloat-abi=hard -march=armv6

main_debug: $(SRC)
	$(CC) $(MY_CFLAGS) main.c -o main -O0

main: $(SRC)
	$(CC) $(MY_CFLAGS) main.c -o main -Ofast -march=armv6

debug:
	qemu-arm -g 1234 ./main &
	gdb-multiarch -q --nh -ex 'set architecture arm' -ex 'file main' -ex 'target remote localhost:1234' -tui         


.PHONY: debug
