#include <nanoalloc.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
	char *s = (char *)malloc(sizeof(argv[1]));
	strcpy(s, argv[1]);

	char fs[10];

	printf("text: %s\n", s);
	printf("text: %s\n", fs);

	return 0;
}
