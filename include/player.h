#pragma once

#include <aabb.h>
#include <world.h>

TSTRUCT(Player){
	AABB aabb;
	vec3 head_euler;
};

void init_player(Player *p, vec3 position, vec3 head_euler);

void move_aabb_against_chunks(ChunkLinkedHashList *chunks, AABB *a, double dt);

void get_player_head_position(Player *p, vec3 pos);