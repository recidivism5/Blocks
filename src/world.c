#include <world.h>

/*
Scaevolus' Region File Format explanation:
https://www.minecraftforum.net/forums/mapping-and-modding-java-edition/minecraft-mods/mods-discussion/1346703-mod-mcregion-v5-optimized-saves-1-2_02?comment=107

Further comment from Scaevolus:
https://www.reddit.com/r/Minecraft/comments/evzbs/comment/c1bg51k/?utm_source=share&utm_medium=web2x&context=3

Map format:
https://wiki.vg/Map_Format
*/

BlockType block_types[] = {
	{"air",{{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}}},
	{"bedrock",{{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}}},
	{"stone",{{1,0},{1,0},{1,0},{1,0},{1,0},{1,0}}},
	{"dirt",{{0,3},{0,3},{0,3},{0,3},{0,3},{0,3}}},
	{"grass",{{0,2},{0,2},{0,3},{0,1},{0,2},{0,2}}},
};

ChunkLinkedHashListBucket *ChunkLinkedHashListGet(ChunkLinkedHashList *list, ivec2 position){
	if (!list->total) return 0;
	size_t index = fnv_1a(sizeof(position),position) % list->total;
	ChunkLinkedHashListBucket *tombstone = 0;
	while (1){
		ChunkLinkedHashListBucket *b = list->buckets+index;
		if (b->chunk == TOMBSTONE) tombstone = b;
		else if (b->chunk == 0) return tombstone ? tombstone : b;
		else if (!memcmp(b->position,position,sizeof(position))) return b;
		index = (index + 1) % list->total;
	}
}

ChunkLinkedHashListBucket *ChunkLinkedHashListGetChecked(ChunkLinkedHashList *list, ivec2 position){
	ChunkLinkedHashListBucket *b = ChunkLinkedHashListGet(list,position);
	if (!b || b->chunk == 0 || b->chunk == TOMBSTONE) return 0;
	return b;
}

void ChunkLinkedHashListRemove(ChunkLinkedHashList *list, ChunkLinkedHashListBucket *b){
	b->chunk = TOMBSTONE;
	if (b->prev) b->prev->next = b->next;
	if (b->next) b->next->prev = b->prev;
	if (list->first == b) list->first = b->next;
	if (list->last == b) list->last = b->prev;
	list->used--;
	list->tombstones++;
}

ChunkLinkedHashListBucket *ChunkLinkedHashListNew(ChunkLinkedHashList *list, ivec2 position){
	if ((list->used+list->tombstones+1) > (list->total*3)/4){
		ChunkLinkedHashList newList;
		newList.total = 8;
		while (newList.total < (list->used*2)) newList.total *= 2; //make a new list with at least twice as many elements as list->used.
		newList.used = list->used;
		newList.tombstones = 0;
		newList.first = 0;
		newList.last = 0;
		newList.buckets = zalloc_or_die(newList.total*sizeof(*newList.buckets));
		for (ChunkLinkedHashListBucket *b = list->first; b; b = b->next){
			ChunkLinkedHashListBucket *nb = ChunkLinkedHashListGet(&newList,b->position);
			memcpy(nb->position,b->position,sizeof(b->position));
			nb->chunk = b->chunk;
			if (!newList.first){
				newList.first = nb;
				nb->prev = 0;
			} else {
				newList.last->next = nb;
				nb->prev = newList.last;
			}
			newList.last = nb;
			nb->next = 0;
		}
		if (list->buckets) free(list->buckets);
		*list = newList;
	}
	ChunkLinkedHashListBucket *b = ChunkLinkedHashListGet(list,position);
	memcpy(b->position,position,sizeof(position));
	if (!list->first){
		list->first = b;
		b->prev = 0;
	} else {
		list->last->next = b;
		b->prev = list->last;
	}
	list->last = b;
	b->next = 0;
	if (b->chunk == TOMBSTONE) list->tombstones--;
	list->used++;
	return b;
}

void gen_chunk(ChunkLinkedHashListBucket *b){
	Chunk *c = b->chunk;
#define HEIGHTMAP_WIDTH (CHUNK_WIDTH+1)
#define HEIGHT_AT(x,z) ((z)*HEIGHTMAP_WIDTH+(x))
	ivec2 cbase = {b->position[0]*CHUNK_WIDTH,b->position[1]*CHUNK_WIDTH};
	static float height_map[HEIGHTMAP_WIDTH*HEIGHTMAP_WIDTH];
	for (int z = 0; z < HEIGHTMAP_WIDTH; z+=4){
		for (int x = 0; x < HEIGHTMAP_WIDTH; x+=4){
			height_map[HEIGHT_AT(x,z)] = 0.4f+0.2f*fractal_perlin_noise_2d(cbase[0]+x,cbase[1]+z,0.005f,8);
		}
	}
	for (int z = 0; z < CHUNK_WIDTH; z++){
		for (int x = 0; x < CHUNK_WIDTH; x++){
			int min[2] = {(x/4)*4,(z/4)*4};
			int max[2] = {min[0]+4,min[1]+4};
			float t[2] = {
				(float)(x%4)/4.0f,
				(float)(z%4)/4.0f
			};
			height_map[HEIGHT_AT(x,z)] =
				glm_lerp(
					glm_lerp(height_map[HEIGHT_AT(min[0],min[1])],height_map[HEIGHT_AT(max[0],min[1])],t[0]),
					glm_lerp(height_map[HEIGHT_AT(min[0],max[1])],height_map[HEIGHT_AT(max[0],max[1])],t[0]),
					t[1]
				);
		}
	}
#define DENSITYMAP_WIDTH (CHUNK_WIDTH+1)
#define DENSITYMAP_HEIGHT (CHUNK_HEIGHT+1)
#define DENSITY_AT(x,y,z) ((y)*DENSITYMAP_WIDTH*DENSITYMAP_WIDTH + (z)*DENSITYMAP_WIDTH + (x))
	static float density_map[DENSITYMAP_WIDTH*DENSITYMAP_WIDTH*DENSITYMAP_HEIGHT];
	for (int y = 0; y < DENSITYMAP_HEIGHT; y+=4){
		for (int z = 0; z < DENSITYMAP_WIDTH; z+=4){
			for (int x = 0; x < DENSITYMAP_WIDTH; x+=4){
				density_map[DENSITY_AT(x,y,z)] = fractal_perlin_noise_3d(cbase[0]+x,y,cbase[1]+z,0.005f,8) + 1.4f*(height_map[HEIGHT_AT(x,z)]-(float)y/CHUNK_HEIGHT);
			}
		}
	}
	for (int y = 0; y < CHUNK_HEIGHT; y++){
		for (int z = 0; z < CHUNK_WIDTH; z++){
			for (int x = 0; x < CHUNK_WIDTH; x++){
				int min[3] = {(x/4)*4,(y/4)*4,(z/4)*4};
				int max[3] = {min[0]+4,min[1]+4,min[2]+4};
				float t[3] = {
					(float)(x%4)/4.0f,
					(float)(y%4)/4.0f,
					(float)(z%4)/4.0f
				};
				density_map[DENSITY_AT(x,y,z)] =
					glm_lerp(
						glm_lerp(
							glm_lerp(density_map[DENSITY_AT(min[0],min[1],min[2])],density_map[DENSITY_AT(max[0],min[1],min[2])],t[0]),
							glm_lerp(density_map[DENSITY_AT(min[0],min[1],max[2])],density_map[DENSITY_AT(max[0],min[1],max[2])],t[0]),
							t[2]
						),
						glm_lerp(
							glm_lerp(density_map[DENSITY_AT(min[0],max[1],min[2])],density_map[DENSITY_AT(max[0],max[1],min[2])],t[0]),
							glm_lerp(density_map[DENSITY_AT(min[0],max[1],max[2])],density_map[DENSITY_AT(max[0],max[1],max[2])],t[0]),
							t[2]
						),
						t[1]
					);
			}
		}
	}
	for (int y = 0; y < CHUNK_HEIGHT; y++){
		for (int z = 0; z < CHUNK_WIDTH; z++){
			for (int x = 0; x < CHUNK_WIDTH; x++){
				float s = density_map[DENSITY_AT(x,y,z)];
				if (s > 0){
					bool block_above = density_map[DENSITY_AT(x,y+1,z)] > 0;
					c->blocks[BLOCK_AT(x,y,z)].id = block_above ? BLOCK_DIRT : BLOCK_GRASS;
				} else {
					c->blocks[BLOCK_AT(x,y,z)].id = BLOCK_AIR;
				}
			}
		}
	}
#undef HEIGHTMAP_WIDTH
#undef HEIGHT_AT
#undef DENSITYMAP_WIDTH
#undef DENTIYMAP_HEIGHT
#undef DENSITY_AT
}

vec3 cube_verts[] = {
	0,1,0, 0,0,0, 0,0,1, 0,0,1, 0,1,1, 0,1,0,
	1,1,1, 1,0,1, 1,0,0, 1,0,0, 1,1,0, 1,1,1,

	0,0,0, 1,0,0, 1,0,1, 1,0,1, 0,0,1, 0,0,0,
	1,1,0, 0,1,0, 0,1,1, 0,1,1, 1,1,1, 1,1,0,

	1,1,0, 1,0,0, 0,0,0, 0,0,0, 0,1,0, 1,1,0,
	0,1,1, 0,0,1, 1,0,1, 1,0,1, 1,1,1, 0,1,1,
};

vec3 block_outline[] = {
	0,1,0, 0,0,0,
	0,0,0, 0,0,1,
	0,0,1, 0,1,1,
	0,1,1, 0,1,0,

	1,1,0, 1,0,0,
	1,0,0, 1,0,1,
	1,0,1, 1,1,1,
	1,1,1, 1,1,0,

	0,1,0, 1,1,0,
	0,0,0, 1,0,0,
	0,0,1, 1,0,1,
	0,1,1, 1,1,1,
};

float ambient_light_coefficients[6] = {0.6f,0.6f,0.5f,1.0f,0.8f,0.8f};

void append_block_face(TextureColorVertexList *tvl, int x, int y, int z, int face_id, BlockType *bt){
	TextureColorVertex *v = TextureColorVertexListMakeRoom(tvl,6);
	uint8_t ambient = ambient_light_coefficients[face_id] * 255;
	uint32_t color = RGBA(ambient,ambient,ambient,255);
	for (int i = 0; i < 6; i++){
		v[i].x = x + cube_verts[face_id*6+i][0];
		v[i].y = y + cube_verts[face_id*6+i][1];
		v[i].z = z + cube_verts[face_id*6+i][2];

		uint8_t ambient = ambient_light_coefficients[face_id] * 255;
		v[i].color = color;
	}
	float w = 1.0f/16.0f;

	v[0].u = w*bt->faces[face_id][0];
	v[0].v = w*(16-bt->faces[face_id][1]);

	v[1].u = v[0].u;
	v[1].v = v[0].v - w;

	v[2].u = v[0].u + w;
	v[2].v = v[1].v;

	v[3].u = v[2].u;
	v[3].v = v[2].v;

	v[4].u = v[2].u;
	v[4].v = v[0].v;

	v[5].u = v[0].u;
	v[5].v = v[0].v;
}

void mesh_chunk(ChunkLinkedHashList *list, ChunkLinkedHashListBucket *b){
	Chunk *c = b->chunk;
	ChunkLinkedHashListBucket *neighbor_buckets[4] = {
		ChunkLinkedHashListGetChecked(list,(ivec2){b->position[0]-1,b->position[1]}),
		ChunkLinkedHashListGetChecked(list,(ivec2){b->position[0]+1,b->position[1]}),
		ChunkLinkedHashListGetChecked(list,(ivec2){b->position[0],b->position[1]-1}),
		ChunkLinkedHashListGetChecked(list,(ivec2){b->position[0],b->position[1]+1})
	};
	Chunk *neighbors[4];
	for (int i = 0; i < 4; i++){
		if (neighbor_buckets[i]){
			neighbors[i] = neighbor_buckets[i]->chunk;
			c->neighbors_exist[i] = true;
		} else {
			neighbors[i] = 0;
			c->neighbors_exist[i] = false;
		}
	}
	if (neighbors[0] && !neighbors[0]->neighbors_exist[1]) mesh_chunk(list,neighbor_buckets[0]);
	if (neighbors[1] && !neighbors[1]->neighbors_exist[0]) mesh_chunk(list,neighbor_buckets[1]);
	if (neighbors[2] && !neighbors[2]->neighbors_exist[3]) mesh_chunk(list,neighbor_buckets[2]);
	if (neighbors[3] && !neighbors[3]->neighbors_exist[2]) mesh_chunk(list,neighbor_buckets[3]);
	TextureColorVertexList tvl = {0};
	for (int y = 0; y < CHUNK_HEIGHT; y++){
		for (int z = 0; z < CHUNK_WIDTH; z++){
			for (int x = 0; x < CHUNK_WIDTH; x++){
				BlockId id = c->blocks[BLOCK_AT(x,y,z)].id;
				if (id){
					BlockType *bt = block_types + id;
					if ((x == 0 && neighbors[0] && !neighbors[0]->blocks[BLOCK_AT(CHUNK_WIDTH-1,y,z)].id) || (x > 0 && !c->blocks[BLOCK_AT(x-1,y,z)].id)){
						append_block_face(&tvl,x,y,z,0,bt);
					}
					if ((x == (CHUNK_WIDTH-1) && neighbors[1] && !neighbors[1]->blocks[BLOCK_AT(0,y,z)].id) || (x < (CHUNK_WIDTH-1) && !c->blocks[BLOCK_AT(x+1,y,z)].id)){
						append_block_face(&tvl,x,y,z,1,bt);
					}
					if (y == 0 || !c->blocks[BLOCK_AT(x,y-1,z)].id){
						append_block_face(&tvl,x,y,z,2,bt);
					}
					if (y == (CHUNK_HEIGHT-1) || !c->blocks[BLOCK_AT(x,y+1,z)].id){
						append_block_face(&tvl,x,y,z,3,bt);
					}
					if ((z == 0 && neighbors[2] && !neighbors[2]->blocks[BLOCK_AT(x,y,CHUNK_WIDTH-1)].id) || (z > 0 && !c->blocks[BLOCK_AT(x,y,z-1)].id)){
						append_block_face(&tvl,x,y,z,4,bt);
					}
					if ((z == (CHUNK_WIDTH-1) && neighbors[3] && !neighbors[3]->blocks[BLOCK_AT(x,y,0)].id) || (z < (CHUNK_WIDTH-1) && !c->blocks[BLOCK_AT(x,y,z+1)].id)){
						append_block_face(&tvl,x,y,z,5,bt);
					}
				}
			}
		}
	}
	if (c->mesh.vertex_count){
		delete_gpu_mesh(&c->mesh);
	}
	if (tvl.used){
		gpu_mesh_from_texture_color_verts(&c->mesh,tvl.elements,tvl.used);
		free(tvl.elements);
	}
}

Block *get_block(World *w, int x, int y, int z){
	if (y < 0 || y > (CHUNK_HEIGHT-1)){
		return 0;
	}
	ChunkLinkedHashListBucket *b = ChunkLinkedHashListGetChecked(&w->chunks,(ivec2){(x+(x<0))/CHUNK_WIDTH-(x<0),(z+(z<0))/CHUNK_WIDTH-(z<0)});
	if (b){
		return b->chunk->blocks + BLOCK_AT(modulo(x,CHUNK_WIDTH),y,modulo(z,CHUNK_WIDTH));
	} else {
		return 0;
	}
}

Block *block_raycast(World *w, vec3 origin, vec3 ray, float *t, ivec3 block_pos, ivec3 face_normal){
	block_pos[0] = floorf(origin[0]);
	block_pos[1] = floorf(origin[1]);
	block_pos[2] = floorf(origin[2]);
	vec3 da;
	for (int i = 0; i < 3; i++){
		if (ray[i] < 0){
			da[i] = ((float)block_pos[i]-origin[i]) / ray[i];
		} else {
			da[i] = ((float)block_pos[i]+1.0f-origin[i]) / ray[i];	
		}
	}
	*t = 0;
	int index = 0;
	while (*t <= 1.0f){
		Block *b = get_block(w,block_pos[0],block_pos[1],block_pos[2]);
		if (b && b->id){
			glm_ivec3_zero(face_normal);
			face_normal[index] = ray[index] < 0 ? 1 : -1;
			return b;
		}
		float d = HUGE_VALF;
		index = 0;
		for (int i = 0; i < 3; i++){
			if (da[i] < d){
				index = i;
				d = da[i];
			}
		}
		block_pos[index] += ray[index] < 0 ? -1 : 1;
		*t += da[index];
		glm_vec3_subs(da,d,da);
		da[index] = fabsf(1.0f / ray[index]);
	}
	return 0;
}

/*u8 raycast(FVec3 origin, FVec3 direction, float *vScale, IVec3 *blockPos, IVec3 *face){
	IVec3 gp = {floorf(origin.x),floorf(origin.y),floorf(origin.z)};
	FVec3 da;
	FOR(i,3){
		if (direction.arr[i] >= 0) da.arr[i] = ((float)gp.arr[i]+1.0f-origin.arr[i]) / direction.arr[i];
		else da.arr[i] = ((float)gp.arr[i]-origin.arr[i]) / direction.arr[i];
	}
	*vScale = 0;
	int index = 0;
	while (*vScale < 1){
		u8 b;
		Chunk *c;
		if ((b = getBlock(gp.x,gp.y,gp.z,&c).id) && c){
			*blockPos = gp;
			*face = (IVec3){0,0,0};
			face->arr[index] = direction.arr[index] < 0 ? 1 : -1;
			return b;
		}
		float d = HUGE_VAL;
		index = 0;
		FOR(i,3){
			if (da.arr[i] < d){
				index = i;
				d = da.arr[i];
			}
		}
		gp.arr[index] += direction.arr[index] < 0 ? -1 : 1;
		*vScale += da.arr[index];
		FOR(i,3) da.arr[i] -= d;
		da.arr[index] = fabsf(1.0f/direction.arr[index]);
	}
	return AIR;
}*/