#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <windows.h>

#undef near //fuck deez macros lol
#undef far
#undef min
#undef max
#define COUNT(arr) (sizeof(arr)/sizeof(*arr))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define CLAMP(a,min,max) ((a) < (min) ? (min) : (a) > (max) ? (max) : (a))
#define SWAP(temp,a,b) (temp)=(a); (a)=(b); (b)=(temp)
#define COMPARE(a,b) (((a) > (b)) - ((a) < (b)))
#define RGBA(r,g,b,a) ((r) | ((g)<<8) | ((b)<<16) | ((a)<<24))
#define TSTRUCT(name)\
typedef struct name name;\
struct name

void fatal_error(char *format, ...){
	va_list args;
	va_start(args,format);
	char msg[1024];
	vsnprintf(msg,COUNT(msg),format,args);
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

typedef int ivec2[2];

TSTRUCT(ChunkLinkedHashListBucket){
	ChunkLinkedHashListBucket *prev, *next;
	ivec2 position;
	void *chunk;
};

TSTRUCT(ChunkLinkedHashList){
	size_t total, used, tombstones;
	ChunkLinkedHashListBucket *buckets, *first, *last;
};

#define TOMBSTONE UINTPTR_MAX

ChunkLinkedHashListBucket *ChunkLinkedHashListGet(ChunkLinkedHashList *list, ivec2 position){
	if (!list->total) return 0;
	size_t index = fnv_1a(sizeof(position),position) % list->total;
	ChunkLinkedHashListBucket *tombstone = 0;
	while (1){
		ChunkLinkedHashListBucket *b = list->buckets+index;
		if (b->chunk == TOMBSTONE) tombstone = b;
		else if (b->chunk == 0) return tombstone ? tombstone : b;
		else if (!memcmp(b->position,position,sizeof(position))) return b;
		index = (index + 1) % list->total;
	}
}

ChunkLinkedHashListBucket *ChunkLinkedHashListGetChecked(ChunkLinkedHashList *list, ivec2 position){
	ChunkLinkedHashListBucket *b = ChunkLinkedHashListGet(list,position);
	if (!b || b->chunk == 0 || b->chunk == TOMBSTONE) return 0;
	return b;
}

void ChunkLinkedHashListRemove(ChunkLinkedHashList *list, ChunkLinkedHashListBucket *b){
	b->chunk = TOMBSTONE;
	if (b->prev) b->prev->next = b->next;
	if (b->next) b->next->prev = b->prev;
	if (list->first == b) list->first = b->next;
	if (list->last == b) list->last = b->prev;
	list->used--;
	list->tombstones++;
}

ChunkLinkedHashListBucket *ChunkLinkedHashListNew(ChunkLinkedHashList *list, ivec2 position){
	if ((list->used+list->tombstones+1) > (list->total*3)/4){
		ChunkLinkedHashList newList;
		newList.total = 8;
		while (newList.total < (list->used*16)) newList.total *= 2;
		newList.used = list->used;
		newList.tombstones = 0;
		newList.first = 0;
		newList.last = 0;
		newList.buckets = zalloc_or_die(newList.total*sizeof(*newList.buckets));
		for (ChunkLinkedHashListBucket *b = list->first; b; b = b->next){
			ChunkLinkedHashListBucket *nb = ChunkLinkedHashListGet(&newList,b->position);
			memcpy(nb->position,b->position,sizeof(b->position));
			nb->chunk = b->chunk;
			if (!newList.first){
				newList.first = nb;
				nb->prev = 0;
			} else {
				newList.last->next = nb;
				nb->prev = newList.last;
			}
			newList.last = nb;
			nb->next = 0;
		}
		if (list->buckets) free(list->buckets);
		*list = newList;
	}
	ChunkLinkedHashListBucket *b = ChunkLinkedHashListGet(list,position);
	memcpy(b->position,position,sizeof(position));
	if (!list->first){
		list->first = b;
		b->prev = 0;
	} else {
		list->last->next = b;
		b->prev = list->last;
	}
	list->last = b;
	b->next = 0;
	if (b->chunk == TOMBSTONE) list->tombstones--;
	list->used++;
	return b;
}

void main(){
    LARGE_INTEGER frequency;
    LARGE_INTEGER start_time;
    LARGE_INTEGER end_time;
    double elapsed_time;

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start_time);

    ChunkLinkedHashList chl = {0};
    for (int i = 0; i < 1000000; i++){
        ChunkLinkedHashListBucket *b = ChunkLinkedHashListNew(&chl,(ivec2){i,i});
        b->chunk = i+1;
    }
    for (int i = 0; i < 1000000; i++){
        ChunkLinkedHashListBucket *b = ChunkLinkedHashListGet(&chl,(ivec2){i,i});
        void *val = b->chunk;
        ChunkLinkedHashListRemove(&chl,b);
        b = ChunkLinkedHashListNew(&chl,(ivec2){i+1,i});
        b->chunk = val;
    }

    QueryPerformanceCounter(&end_time);
    elapsed_time = ((double)(end_time.QuadPart - start_time.QuadPart) / frequency.QuadPart) * 1000000000.0;

    printf("Elapsed time: %f nanoseconds\n", elapsed_time);
}