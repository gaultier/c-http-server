SRC := main.c sys.h http.h

CC = clang

# Assume clang for cross compilation.
MY_CFLAGS := -Wall -Wextra -std=c99 --target=armv6-linux-none -static -g3 -fuse-ld=lld -nostdlib -mfloat-abi=hard

main: $(SRC)
	$(CC) $(MY_CFLAGS) main.c -o main -Ofast -march=armv6
