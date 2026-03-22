gcc -I../include  main.c -o main -g -L../build -lnanoalloc -Wl,-rpath=../build
