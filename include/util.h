#ifndef _UTIL_H
#define _UTIL_H 1

#include <assert.h>

#define _TODO() \
    do { \
        assert(!"TODO at " __FILE__ ":" __STR(__LINE__)); \
    } while(0)


#define __STR_HELPER(x) #x
#define __STR(x) __STR_HELPER(x)

#endif //_UTIL_H
