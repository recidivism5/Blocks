#include <player.h>

void init_player(Player *p, vec3 position, vec3 head_euler){
	p->aabb.half_extents[0] = 0.25f;
	p->aabb.half_extents[1] = 0.9f;
	p->aabb.half_extents[2] = 0.25f;
	glm_vec3_copy(position,p->aabb.position);
	glm_vec3_copy(head_euler,p->head_euler);
}

void move_aabb_against_chunks(ChunkLinkedHashList *chunks, AABB *a, double dt){
	vec3 d;
	glm_vec3_scale(a->velocity,dt,d);
	vec3 d_initial;
	glm_vec3_copy(d,d_initial);
	MMBB m;
	aabb_to_mmbb(a,&m);
	MMBB exp_m = m;
	expand_mmbb(&exp_m,d);
	ivec3 block_min, block_max;
	for (int i = 0; i < 3; i++){
		block_min[i] = floorf(exp_m.min[i]);
		block_max[i] = floorf(exp_m.max[i]);
	}
	MMBBList mbl = {0};
	for (int y = block_min[1]; y <= block_max[1]; y++){
		for (int z = block_min[2]; z <= block_max[2]; z++){
			for (int x = block_min[0]; x <= block_max[0]; x++){
				Block *b = get_block(chunks,(ivec3){x,y,z});
				if (b && b->id){
					MMBB *bm = MMBBListMakeRoom(&mbl,1);
					bm->min[0] = x;
					bm->min[1] = y;
					bm->min[2] = z;
					bm->max[0] = x+1;
					bm->max[1] = y+1;
					bm->max[2] = z+1;
				}
			}
		}
	}
	for (MMBB *bm = mbl.elements; bm < mbl.elements+mbl.used; bm++){
		if (m.min[0] < bm->max[0] && bm->min[0] < m.max[0] && 
			m.min[2] < bm->max[2] && bm->min[2] < m.max[2]){
			if (d[1] < 0 && bm->max[1] <= m.min[1]){
				float nd = bm->max[1] - m.min[1];
				if (d[1] < nd){
					d[1] = nd + 0.001f;
				}
			} else if (d[1] > 0 && m.max[1] <= bm->min[1]){
				float nd = bm->min[1] - m.max[1];
				if (nd < d[1]){
					d[1] = nd - 0.001f;
				}
			}
		}
	}
	m.min[1] += d[1];
	m.max[1] += d[1];
	for (MMBB *bm = mbl.elements; bm < mbl.elements+mbl.used; bm++){
		if (m.min[0] < bm->max[0] && bm->min[0] < m.max[0] &&
			m.min[1] < bm->max[1] && bm->min[1] < m.max[1]){
			if (d[2] < 0 && bm->max[2] <= m.min[2]){
				float nd = bm->max[2] - m.min[2];
				if (d[2] < nd){
					d[2] = nd + 0.001f;
				}
			} else if (d[2] > 0 && m.max[2] <= bm->min[2]){
				float nd = bm->min[2] - m.max[2];
				if (nd < d[2]){
					d[2] = nd - 0.001f;
				}
			}
		}
	}
	m.min[2] += d[2];
	m.max[2] += d[2];
	for (MMBB *bm = mbl.elements; bm < mbl.elements+mbl.used; bm++){
		if (m.min[1] < bm->max[1] && bm->min[1] < m.max[1] &&
			m.min[2] < bm->max[2] && bm->min[2] < m.max[2]){
			if (d[0] < 0 && bm->max[0] <= m.min[0]){
				float nd = bm->max[0] - m.min[0];
				if (d[0] < nd){
					d[0] = nd + 0.001f;
				}
			} else if (d[0] > 0 && m.max[0] <= bm->min[0]){
				float nd = bm->min[0] - m.max[0];
				if (nd < d[0]){
					d[0] = nd - 0.001f;
				}
			}
		}
	}
	m.min[0] += d[0];
	m.max[0] += d[0];
	if (mbl.used){
		free(mbl.elements);
	}
	mmbb_get_center(&m,a->position);
	if (d[0] != d_initial[0]){
		a->velocity[0] = 0.0f;
	}
	if (d[1] != d_initial[1]){
		a->velocity[1] = 0.0f;
		if (d_initial[1] < 0.0f){
			a->on_ground = true;
		}
	} else {
		a->on_ground = false;
	}
	if (d[2] != d_initial[2]){
		a->velocity[2] = 0.0f;
	}
}

void get_player_head_position(Player* p, vec3 pos){
	glm_vec3_copy(p->aabb.position,pos);
	pos[1] += 1.62f - 0.9f;
}
