#pragma once

#include <world.h>

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

MMBB *MMBBListMakeRoom(MMBBList *list, size_t count);

void aabb_to_mmbb(AABB *a, MMBB *m);

void expand_mmbb(MMBB *m, vec3 v);

void mmbb_get_center(MMBB *m, vec3 c);

TSTRUCT(Player){
	AABB aabb;
	vec3 head_euler;
};

void init_player(Player *p, vec3 position, vec3 head_euler);

void move_aabb(World *w, AABB *a, double dt);

void get_player_head_position(Player *p, vec3 pos);