CC = clang
CFLAGS = -I../include -g -Wall -DNA_DEBUG
LDFLAGS = -L../build -lnanoalloc -Wl,-rpath=../build

all: test1 test2 test3 calloc smoke

test1: test1.c
	$(CC) $(CFLAGS) test1.c -o test1.out $(LDFLAGS)


test2: test2.c
	$(CC) $(CFLAGS) test2.c -o test2.out $(LDFLAGS)

test3: test3.c
		$(CC) $(CFLAGS) test3.c -o test3.out $(LDFLAGS)	

calloc: calloc.c
		$(CC) $(CFLAGS) calloc.c -o calloc.out $(LDFLAGS)	

smoke: smoke.c
		$(CC) $(CFLAGS) smoke.c -o smoke.out $(LDFLAGS)	

clean:
	rm -rf test1.out
	rm -rf test2.out
	rm -rf test3.out
	rm -rf calloc.out
	rm -rf smoke.out
