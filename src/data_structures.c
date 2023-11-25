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
inline type *type##ListMakeRoom(type ## List *list, size_t count){\
	if (list->used+count > list->total){\
		if (!list->total) list->total = 1;\
		while (list->used+count > list->total) list->total *= 2;\
		list->elements = realloc_or_die(list->elements,list->total*sizeof(type));\
	}\
	return list->elements+list->used;\
}\
inline void type##ListMakeRoomAtIndex(type ## List *list, size_t index, size_t count){\
	type##ListMakeRoom(list,count);\
	if (index != list->used) memmove(list->elements+index+count,list->elements+index,(list->used-index)*sizeof(type));\
}\
inline void type##ListInsert(type ## List *list, size_t index, type *elements, size_t count){\
	type##ListMakeRoomAtIndex(list,index,count);\
	memcpy(list->elements+index,elements,count*sizeof(type));\
	list->used += count;\
}\
inline void type##ListAppend(type ## List *list, type *elements, size_t count){\
	type##ListMakeRoom(list,count);\
	memcpy(list->elements+list->used,elements,count*sizeof(type));\
	list->used += count;\
}

size_t fnv1a(size_t keylen, char *key)
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

#define LINKED_HASHLIST_INIT(list)\
	(list)->total = 0;\
	(list)->used = 0;\
	(list)->buckets = 0;\
	(list)->first = 0;\
	(list)->last = 0;
#define LINKED_HASHLIST_FREE(list)\
	if ((list)->total){\
		free((list)->buckets);\
		LINKED_HASHLIST_INIT(list);\
	}
#define LINKED_HASHLIST_IMPLEMENTATION(type)\
typedef struct type##LinkedHashListBucket {\
	struct type##LinkedHashListBucket *prev, *next;\
	size_t keylen;\
	char *key;\
	type value;\
} type##LinkedHashListBucket;\
typedef struct type##LinkedHashList {\
	size_t total, used;\
	type##LinkedHashListBucket *buckets, *first, *last;\
} type##LinkedHashList;\
inline type##LinkedHashListBucket *type##LinkedHashListGetBucket(type##LinkedHashList *list, size_t keylen, char *key){\
	if (!list->total) return 0;\
	size_t index = fnv1a(keylen,key);\
	index %= list->total;\
	type##LinkedHashListBucket *lastTombstone = 0;\
	while (1){\
		type##LinkedHashListBucket *bucket = list->buckets+index;\
		if (bucket->keylen < 0) lastTombstone = bucket;\
		else if (!bucket->keylen) return lastTombstone ? lastTombstone : bucket;\
		else if (bucket->keylen == keylen && !memcmp(bucket->key,key,keylen)) return bucket;\
		index = (index + 1) % list->total;\
	}\
}\
inline type *type##LinkedHashListGet(type##LinkedHashList *list, size_t keylen, char *key){\
	type##LinkedHashListBucket *bucket = type##LinkedHashListGetBucket(list,keylen,key);\
	if (bucket && bucket->keylen > 0) return &bucket->value;\
	else return 0;\
}\
inline type *type##LinkedHashListNew(type##LinkedHashList *list, size_t keylen, char *key){\
	if (!list->total){\
		list->total = 8;\
		list->buckets = zalloc_or_die(8*sizeof(*list->buckets));\
	}\
	if (list->used+1 > (list->total*3)/4){\
		type##LinkedHashList newList;\
		newList.total = list->total * 2;\
		newList.used = list->used;\
		newList.buckets = zalloc_or_die(newList.total*sizeof(*newList.buckets));\
		for (type##LinkedHashListBucket *bucket = list->first; bucket;){\
			type##LinkedHashListBucket *newBucket = type##LinkedHashListGetBucket(&newList,bucket->keylen,bucket->key);\
			newBucket->keylen = bucket->keylen;\
			newBucket->key = bucket->key;\
			newBucket->value = bucket->value;\
			newBucket->next = 0;\
			if (bucket == list->first){\
				newBucket->prev = 0;\
				newList.first = newBucket;\
				newList.last = newBucket;\
			} else {\
				newBucket->prev = newList.last;\
				newList.last->next = newBucket;\
				newList.last = newBucket;\
			}\
			bucket = bucket->next;\
		}\
		free(list->buckets);\
		*list = newList;\
	}\
	type##LinkedHashListBucket *bucket = type##LinkedHashListGetBucket(list,keylen,key);\
	if (bucket->keylen <= 0){\
		list->used++;\
		bucket->keylen = keylen;\
		bucket->key = key;\
		bucket->next = 0;\
		if (list->last){\
			bucket->prev = list->last;\
			list->last->next = bucket;\
			list->last = bucket;\
		} else {\
			bucket->prev = 0;\
			list->first = bucket;\
			list->last = bucket;\
		}\
	}\
	return &bucket->value;\
}\
inline void type##LinkedHashListRemove(type##LinkedHashList *list, size_t keylen, char *key){\
	type##LinkedHashListBucket *bucket = type##LinkedHashListGetBucket(list,keylen,key);\
	if (bucket->keylen > 0){\
		if (bucket->prev){\
			if (bucket->next){\
				bucket->prev->next = bucket->next;\
				bucket->next->prev = bucket->prev;\
			} else {\
				bucket->prev->next = 0;\
				list->last = bucket->prev;\
			}\
		} else if (bucket->next){\
			list->first = bucket->next;\
			bucket->next->prev = 0;\
		}\
		bucket->keylen = -1;\
		list->used--;\
	}\
}