#ifdef INCLUDE_LEVEL
#define INCLUDE_LEVEL 1
#else
#define INCLUDE_LEVEL 0
#endif
#pragma push_macro("INCLUDE_LEVEL")

#include <ctype.h>

#pragma pop_macro("INCLUDE_LEVEL")

void StringToLower(size_t len, char *str)
#if INCLUDE_LEVEL == 0
{
    for (size_t i = 0; i < len; i++){
        str[i] = tolower(str[i]);
    }
}
#else
    ;
#endif