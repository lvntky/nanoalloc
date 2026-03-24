#include <stdio.h>
#include <nanoalloc.h>

int main(int argc, char *argv[])
{
	int *a = malloc(4096);
	a[0] = 1;

	free(a);

	int *b = malloc(1024);
	b[0] = 2;

	return 0;
}
