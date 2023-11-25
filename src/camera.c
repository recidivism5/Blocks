#ifdef INCLUDED
#define INCLUDED 1
#else
#define INCLUDED 0
#endif
#pragma push_macro("INCLUDED")

#include <cglm/cglm.h>

#include "base.c"

#pragma pop_macro("INCLUDED")

typedef struct {
	vec3 position;
	vec3 euler;
	float fov_radians;
} Camera;