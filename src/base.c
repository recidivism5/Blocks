#ifdef INCLUDED
#define INCLUDED 1
#else
#define INCLUDED 0
#endif
#pragma push_macro("INCLUDED")

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <boxer/boxer.h>

#pragma pop_macro("INCLUDED")

#undef near //fuck deez macros lol
#undef far
#undef min
#undef max
#define COUNT(arr) (sizeof(arr)/sizeof(*arr))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define CLAMP(a,min,max) ((a) < (min) ? (min) : (a) > (max) ? (max) : (a))
#define SWAP(temp,a,b) (temp)=(a); (a)=(b); (b)=(temp)
#define LERP(a,b,t) ((a) + (t)*((b)-(a)))

void fatal_error(char *format, ...)
#if INCLUDED == 0
{
	va_list args;
	va_start(args,format);
	char msg[1024];
	vsnprintf(msg,COUNT(msg),format,args);
	boxerShow(msg,"Error",BoxerStyleError,BoxerButtonsQuit);
	va_end(args);
	exit(1);
}
#else
;
#endif

void *malloc_or_die(size_t size)
#if INCLUDED == 0
{
	void *p = malloc(size);
	if (!p) fatal_error("malloc failed.");
	return p;
}
#else
;
#endif

void *zalloc_or_die(size_t size)
#if INCLUDED == 0
{
	void *p = calloc(1,size);
	if (!p) fatal_error("zalloc failed.");
	return p;
}
#else
;
#endif

void *realloc_or_die(void *ptr, size_t size)
#if INCLUDED == 0
{
	void *p = realloc(ptr,size);
	if (!p) fatal_error("realloc failed.");
	return p;
}
#else
;
#endif

/*
rand_int
From: https://stackoverflow.com/a/822361
Generates uniform random integers in range [0,n).
*/
int rand_int(int n)
#if INCLUDED == 0
{
	if ((n - 1) == RAND_MAX){
		return rand();
	} else {
		// Chop off all of the values that would cause skew...
		int end = RAND_MAX / n; // truncate skew
		end *= n;
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

/*
rand_int_range
Generates uniform random integers in range [min,max],
where min >= 0 and max >= min.
*/
int rand_int_range(int min, int max)
#if INCLUDED == 0
{
	return rand_int(max-min+1) + min;
}
#else
;
#endif