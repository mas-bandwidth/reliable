UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S), Linux)
	CC=gcc
	TARGET=libreliable-io.so
else
	CC=clang
	TARGET=libreliable-io.dylib
endif

$(TARGET): reliable.o
	$(CC) -shared -o $@ $^

reliable.o: reliable.c
	$(CC) $< -ffast-math -O3 -msse2 -Wall -Wextra -fPIC -c -g -o $@
	
.PHONY: clean

clean:
	rm *.o $(TARGET)
