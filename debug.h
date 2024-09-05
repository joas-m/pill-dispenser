#ifndef DEBUG_H
#define DEBUG_H

/**/
#define ENABLE_DEBUG_PRINTS
/**/

#ifdef ENABLE_DEBUG_PRINTS
#include <stdio.h>

#define DBG(format, ...)                                                       \
    do {                                                                       \
        printf(format, ##__VA_ARGS__);                                         \
    } while (0)
#else
#define DBG(format, ...)                                                       \
    do {                                                                       \
    } while (0)
#endif

#endif