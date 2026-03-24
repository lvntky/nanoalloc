#include <stdio.h>
#include <nanoalloc.h>

int main(int argc, char *argv[])
{
	int *arr = malloc(5 * sizeof(int));

	for (int i = 0; i < 5; i++) {
		arr[i] = i + 1;
	}

	free(arr);

	int *two = malloc(5 * sizeof(int));

	
	for (int i = 0; i < 5; i++) {
		two[i] = i + 1;
	}

	free(two);
}
