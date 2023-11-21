#ifdef INCLUDE_LEVEL
#define INCLUDE_LEVEL 1
#else
#define INCLUDE_LEVEL 0
#endif
#pragma push_macro("INCLUDE_LEVEL")

#include "test1.c"

#pragma pop_macro("INCLUDE_LEVEL")

void main(){
	char joj[] = "THE FITNESS GRAM PACER TEST IS A %d STAGE AEROBIC CAPACITY TEST\n";
	StringToLower(strlen(joj),joj);
	srand(time(0));
	printf(joj,randint(10));
}