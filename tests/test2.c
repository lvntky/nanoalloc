#include <stdio.h>
#include <nanoalloc.h>

int main(int argc, char *argv[])
{
	int *arr = malloc(5 * sizeof(int));

	for (int i = 0; i < 5; i++) {
	    
		arr[i] = i * 10;
	}

	free(arr);
    return 0;
}

