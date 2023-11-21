#ifdef INCLUDE_LEVEL
#define INCLUDE_LEVEL 1
#else
#define INCLUDE_LEVEL 0
#endif
#pragma push_macro("INCLUDE_LEVEL")

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "test2.c"

#pragma pop_macro("INCLUDE_LEVEL")

/*
randint
From: https://stackoverflow.com/a/822361
Generates uniform random integers in range [0,n).
*/
int randint(int n)
#if INCLUDE_LEVEL == 0
{
    if ((n - 1) == RAND_MAX){
        return rand();
    } else {
        // Chop off all of the values that would cause skew...
        int end = n * (RAND_MAX / n);
        // ... and ignore results from rand() that fall above that limit.
        // (Worst case the loop condition should succeed 50% of the time,
        // so we can expect to bail out of this loop pretty quickly.)
        int r;
        while ((r = rand()) >= end);
        return r % n;
    }
}
#else
;
#endif