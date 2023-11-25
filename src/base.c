#ifdef INCLUDED
#define INCLUDED 1
#else
#define INCLUDED 0
#endif
#pragma push_macro("INCLUDED")

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
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