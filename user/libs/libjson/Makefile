CC   := x86_64-elf-gcc
AR   := x86_64-elf-ar
LIBC := ../../libc

CFLAGS := -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
          -fno-PIE -fno-pic -m64 -march=x86-64 -mno-red-zone -Wall -Wextra \
          -std=gnu11 -I$(LIBC)/include

all: libjson.a

libjson.a: json.o
	$(AR) rcs $@ $^

json.o: json.c json.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o libjson.a

.PHONY: all clean
