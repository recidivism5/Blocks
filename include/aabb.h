#pragma once

#include <base.h>
#include <cglm/cglm.h>

TSTRUCT(AABB){
	vec3 half_extents;
	vec3 position;
	vec3 velocity;
	bool on_ground;
};

TSTRUCT(MMBB){
	vec3 min;
	vec3 max;
};

TSTRUCT(MMBBList){
	size_t total, used;
	MMBB *elements;
};

TSTRUCT(IMMBB){
	ivec3 min;
	ivec3 max;
};

MMBB *MMBBListMakeRoom(MMBBList *list, size_t count);

void aabb_to_mmbb(AABB *a, MMBB *m);

void expand_mmbb(MMBB *m, vec3 v);

void mmbb_get_center(MMBB *m, vec3 c);