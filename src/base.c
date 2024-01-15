#include <base.h>

void fatal_error(char *format, ...){
	va_list args;
	va_start(args,format);
	static char msg[1024];
	vsnprintf(msg,COUNT(msg),format,args);
	boxerShow(msg,"Error",BoxerStyleError,BoxerButtonsQuit);
	va_end(args);
	exit(1);
}

void *malloc_or_die(size_t size){
	void *p = malloc(size);
	if (!p) fatal_error("malloc failed.");
	return p;
}

void *zalloc_or_die(size_t size){
	void *p = calloc(1,size);
	if (!p) fatal_error("zalloc failed.");
	return p;
}

void *realloc_or_die(void *ptr, size_t size){
	void *p = realloc(ptr,size);
	if (!p) fatal_error("realloc failed.");
	return p;
}

int rand_int(int n){
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

int rand_int_range(int min, int max){
	return rand_int(max-min+1) + min;
}

uint32_t fnv_1a(int keylen, char *key){
	uint32_t index = 2166136261u;
	for (int i = 0; i < keylen; i++){
		index ^= key[i];
		index *= 16777619;
	}
	return index;
}

int modulo(int i, int m){
	return (i % m + m) % m;
}