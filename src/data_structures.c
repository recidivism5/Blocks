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
	int keylen;\
	char *key;\
	type value;\
} type##LinkedHashListBucket;\
typedef struct type##LinkedHashList {\
	size_t total, used;\
	type##LinkedHashListBucket *buckets, *first, *last;\
} type##LinkedHashList;\
type##LinkedHashListBucket *type##LinkedHashListGetBucket(type##LinkedHashList *list, int keylen, char *key){\
	if (!list->total) return 0;\
	size_t index = fnv1a(keylen,key);\
	index %= list->total;\
	size_t starting_index = index;\
	type##LinkedHashListBucket *tombstone = 0;\
	while (1){\
		type##LinkedHashListBucket *bucket = list->buckets+index;\
		if (bucket->keylen < 0 && !tombstone) tombstone = bucket;\
		else if (!bucket->keylen) return tombstone ? tombstone : bucket;\
		else if (bucket->keylen == keylen && !memcmp(bucket->key,key,keylen)) return bucket;\
		index = (index + 1) % list->total;\
		if (index == starting_index) return tombstone;\
	}\
}\
type *type##LinkedHashListGet(type##LinkedHashList *list, int keylen, char *key){\
	type##LinkedHashListBucket *bucket = type##LinkedHashListGetBucket(list,keylen,key);\
	if (bucket && bucket->keylen > 0) return &bucket->value;\
	else return 0;\
}\
type##LinkedHashListBucket *type##LinkedHashListNew(type##LinkedHashList *list, int keylen, char *key){\
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
	return bucket;\
}\
void type##LinkedHashListRemove(type##LinkedHashList *list, type##LinkedHashListBucket *bucket){\
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
}
#define LINKED_HASHLIST_HEADER(type)\
typedef struct type##LinkedHashListBucket {\
	struct type##LinkedHashListBucket *prev, *next;\
	int keylen;\
	char *key;\
	type value;\
} type##LinkedHashListBucket;\
typedef struct type##LinkedHashList {\
	size_t total, used;\
	type##LinkedHashListBucket *buckets, *first, *last;\
} type##LinkedHashList;\
type##LinkedHashListBucket *type##LinkedHashListGetBucket(type##LinkedHashList *list, int keylen, char *key);\
type *type##LinkedHashListGet(type##LinkedHashList *list, int keylen, char *key);\
type##LinkedHashListBucket *type##LinkedHashListNew(type##LinkedHashList *list, int keylen, char *key);\
void type##LinkedHashListRemove(type##LinkedHashList *list, type##LinkedHashListBucket *bucket);

#define FIXED_KEY_LINKED_HASHLIST_IMPLEMENTATION(type,fixed_keylen)\
typedef struct type##FixedKeyLinkedHashListBucket {\
	struct type##FixedKeyLinkedHashListBucket *prev, *next;\
	int keylen;\
	char key[fixed_keylen];\
	type value;\
} type##FixedKeyLinkedHashListBucket;\
typedef struct type##FixedKeyLinkedHashList {\
	size_t total, used;\
	type##FixedKeyLinkedHashListBucket *buckets, *first, *last;\
} type##FixedKeyLinkedHashList;\
type##FixedKeyLinkedHashListBucket *type##FixedKeyLinkedHashListGetBucket(type##FixedKeyLinkedHashList *list, int keylen, char *key){\
	if (!list->total) return 0;\
	size_t index = fnv1a(keylen,key);\
	index %= list->total;\
	size_t starting_index = index;\
	type##FixedKeyLinkedHashListBucket *tombstone = 0;\
	while (1){\
		type##FixedKeyLinkedHashListBucket *bucket = list->buckets+index;\
		if (bucket->keylen < 0 && !tombstone) tombstone = bucket;\
		else if (!bucket->keylen) return tombstone ? tombstone : bucket;\
		else if (bucket->keylen == keylen && !memcmp(bucket->key,key,keylen)) return bucket;\
		index = (index + 1) % list->total;\
		if (index == starting_index) return tombstone;\
	}\
}\
type *type##FixedKeyLinkedHashListGet(type##FixedKeyLinkedHashList *list, int keylen, char *key){\
	type##FixedKeyLinkedHashListBucket *bucket = type##FixedKeyLinkedHashListGetBucket(list,keylen,key);\
	if (bucket && bucket->keylen > 0) return &bucket->value;\
	else return 0;\
}\
type##FixedKeyLinkedHashListBucket *type##FixedKeyLinkedHashListNew(type##FixedKeyLinkedHashList *list, int keylen, char *key){\
	if (!list->total){\
		list->total = 8;\
		list->buckets = zalloc_or_die(8*sizeof(*list->buckets));\
	}\
	if (list->used+1 > (list->total*3)/4){\
		type##FixedKeyLinkedHashList newList;\
		newList.total = list->total * 2;\
		newList.used = list->used;\
		newList.buckets = zalloc_or_die(newList.total*sizeof(*newList.buckets));\
		for (type##FixedKeyLinkedHashListBucket *bucket = list->first; bucket;){\
			type##FixedKeyLinkedHashListBucket *newBucket = type##FixedKeyLinkedHashListGetBucket(&newList,bucket->keylen,bucket->key);\
			newBucket->keylen = bucket->keylen;\
			memcpy(newBucket->key,bucket->key,bucket->keylen);\
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
	type##FixedKeyLinkedHashListBucket *bucket = type##FixedKeyLinkedHashListGetBucket(list,keylen,key);\
	if (bucket->keylen <= 0){\
		list->used++;\
		bucket->keylen = keylen;\
		memcpy(bucket->key,key,keylen);\
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
	return bucket;\
}\
void type##FixedKeyLinkedHashListRemove(type##FixedKeyLinkedHashList *list, type##FixedKeyLinkedHashListBucket *bucket){\
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
}
#define FIXED_KEY_LINKED_HASHLIST_HEADER(type,fixed_keylen)\
typedef struct type##FixedKeyLinkedHashListBucket {\
	struct type##FixedKeyLinkedHashListBucket *prev, *next;\
	int keylen;\
	char key[fixed_keylen];\
	type value;\
} type##FixedKeyLinkedHashListBucket;\
typedef struct type##FixedKeyLinkedHashList {\
	size_t total, used;\
	type##FixedKeyLinkedHashListBucket *buckets, *first, *last;\
} type##FixedKeyLinkedHashList;\
type##FixedKeyLinkedHashListBucket *type##FixedKeyLinkedHashListGetBucket(type##FixedKeyLinkedHashList *list, int keylen, char *key);\
type *type##FixedKeyLinkedHashListGet(type##FixedKeyLinkedHashList *list, int keylen, char *key);\
type##FixedKeyLinkedHashListBucket *type##FixedKeyLinkedHashListNew(type##FixedKeyLinkedHashList *list, int keylen, char *key);\
void type##FixedKeyLinkedHashListRemove(type##FixedKeyLinkedHashList *list, type##FixedKeyLinkedHashListBucket *bucket);