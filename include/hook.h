#ifndef HOOK_H_
#define HOOK_H_

void *na_malloc(size_t size);
void na_free(void *ptr);
void *na_realloc(void *ptr, size_t size);
void *na_calloc(size_t nmemb, size_t size);
void *na_memalign(size_t alignment, size_t size);
int na_posix_memalign(void **memptr, size_t alignment, size_t size);

#endif //HOOK_H_
