#ifdef INCLUDED
#define INCLUDED 1
#else
#define INCLUDED 0
#endif
#pragma push_macro("INCLUDED")

#include "base.c"

#pragma pop_macro("INCLUDED")

#define LIST_INIT(list)\
	(list)->total = 0;\
	(list)->used = 0;\
	(list)->elements = 0;
#define LIST_FREE(list)\
	if ((list)->total){\
		free((list)->elements);\
		(list)->total = 0;\
		(list)->used = 0;\
		(list)->elements = 0;\
	}
#define LIST_IMPLEMENTATION(type)\
typedef struct type##List {\
	size_t total, used;\
	type *elements;\
} type##List;\
type *type##ListMakeRoom(type ## List *list, size_t count){\
	if (list->used+count > list->total){\
		if (!list->total) list->total = 1;\
		while (list->used+count > list->total) list->total *= 2;\
		list->elements = realloc_or_die(list->elements,list->total*sizeof(type));\
	}\
	return list->elements+list->used;\
}\
void type##ListMakeRoomAtIndex(type ## List *list, size_t index, size_t count){\
	type##ListMakeRoom(list,count);\
	if (index != list->used) memmove(list->elements+index+count,list->elements+index,(list->used-index)*sizeof(type));\
}\
void type##ListInsert(type ## List *list, size_t index, type *elements, size_t count){\
	type##ListMakeRoomAtIndex(list,index,count);\
	memcpy(list->elements+index,elements,count*sizeof(type));\
	list->used += count;\
}\
void type##ListAppend(type ## List *list, type *elements, size_t count){\
	type##ListMakeRoom(list,count);\
	memcpy(list->elements+list->used,elements,count*sizeof(type));\
	list->used += count;\
}
#define LIST_HEADER(type)\
typedef struct type##List {\
	size_t total, used;\
	type *elements;\
} type##List;\
type *type##ListMakeRoom(type ## List *list, size_t count);\
void type##ListMakeRoomAtIndex(type ## List *list, size_t index, size_t count);\
void type##ListInsert(type ## List *list, size_t index, type *elements, size_t count);\
void type##ListAppend(type ## List *list, type *elements, size_t count);

size_t fnv1a(int keylen, char *key)
#if INCLUDED == 0
{
	size_t index = 2166136261u;
	for (size_t i = 0; i < keylen; i++){
		index ^= key[i];
		index *= 16777619;
	}
	return index;
}
#else
;
#endif

typedef struct LinkedHashListBucket {
	struct LinkedHashListBucket *prev, *next, *prev_in_chain, *next_in_chain;
	int keylen;
	char *key;
	void *value;
} LinkedHashListBucket;

typedef struct LinkedHashList {
	size_t total, used;
	LinkedHashListBucket *buckets, *first, *last;
} LinkedHashList;

void LinkedHashListFree(LinkedHashList *list)
#if INCLUDED == 0
{
	for (LinkedHashListBucket *b = list->first; b; b = b->next){
		LinkedHashListBucket *nb = b->next_in_chain;
		while (nb){
			LinkedHashListBucket *xb = nb->next_in_chain;
			free(nb);
			nb = xb;
		}
	}
	free(list->buckets);
	memset(list,0,sizeof(list));
}
#else
;
#endif

LinkedHashListBucket *LinkedHashListGet(LinkedHashList *list, int keylen, char *key, bool make_new)
#if INCLUDED == 0
{
	if (!list->total){
		return 0;
	}
	LinkedHashListBucket *b = list->buckets + (fnv1a(keylen,key) % list->total);
	if (!b->keylen){
		if (make_new) goto MAKE;
		return 0;
	}
	while (1){
		if (b->keylen == keylen && !memcmp(b->key,key,keylen)){
			return b;
		}
		if (b->next_in_chain){
			b = b->next_in_chain;
		} else if (make_new){
			b->next_in_chain = malloc_or_die(sizeof(*b->next_in_chain));
			b->prev_in_chain = b;//this is not volatile
			b = b->next_in_chain;
			goto MAKE;
		} else {
			return 0;
		}
	}
MAKE:
	b->keylen = keylen;
	b->key = key;
	if (!list->first){
		list->first = b;
		list->last = b;
		b->prev = 0;
	} else {
		list->last->next = b;
		b->prev = list->last;
		list->last = b;
	}
	b->next = 0;
	b->next_in_chain = 0;
	b->value = 0;
	return b;
}
#else
;
#endif

LinkedHashListBucket *LinkedHashListNew(LinkedHashList *list, int keylen, char *key)
#if INCLUDED == 0
{
	if (!list->total){
		list->total = 8;
		list->buckets = zalloc_or_die(8*sizeof(*list->buckets));
	}
	if (list->used+1 > (list->total*3)/4){
		LinkedHashList newList;
		newList.total = list->total * 2;
		newList.used = list->used;
		newList.buckets = zalloc_or_die(newList.total*sizeof(*newList.buckets));
		for (LinkedHashListBucket *b = list->first; b; b = b->next){
			LinkedHashListBucket *nb = LinkedHashListGet(&newList,b->keylen,b->key,true);
			nb->value = b->value;
		}
		LinkedHashListFree(list);
		*list = newList;
	}
	list->used++;
	return LinkedHashListGet(list,keylen,key,true);
}
#else
;
#endif

void LinkedHashListRemove(LinkedHashList *list, LinkedHashListBucket *b)
#if INCLUDED == 0
{
	if (b->prev_in_chain){
		b->prev_in_chain->next_in_chain = b->next_in_chain;
	}
	if (b->prev){
		b->prev->next = b->next;
		if (b->next) b->next->prev = b->prev;
		else list->last = b->prev;
	} else if (b->next){
		list->first = b->next;
		b->next->prev = 0;
	}
	b->keylen = 0;
	list->used--;
}
#else
;
#endif