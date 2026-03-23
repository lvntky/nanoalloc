CC = clang
CFLAGS = -I../include -g -Wall
LDFLAGS = -L../build -lnanoalloc -Wl,-rpath=../build

all: test1 test2

test1: test1.c
	$(CC) $(CFLAGS) test1.c -o test1.out $(LDFLAGS)


test2: test2.c
	$(CC) $(CFLAGS) test2.c -o test2.out $(LDFLAGS)

clean:
	rm -rf test1
	rm -rf test2
