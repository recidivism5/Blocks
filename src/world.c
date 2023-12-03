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
	{"air",true,{{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}}},
	{"bedrock",false,{{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}}},
	{"stone",false,{{1,0},{1,0},{1,0},{1,0},{1,0},{1,0}}},
	{"dirt",false,{{0,3},{0,3},{0,3},{0,3},{0,3},{0,3}}},
	{"grass",false,{{0,2},{0,2},{0,3},{0,1},{0,2},{0,2}}},
	{"log",false,{{1,3},{1,3},{1,2},{1,2},{1,3},{1,3}}},
	{"glass",true,{{8,0},{8,0},{8,0},{8,0},{8,0},{8,0}}},
	{"red_glass",true,{{8,1},{8,1},{8,1},{8,1},{8,1},{8,1}}},
	{"green_glass",true,{{8,2},{8,2},{8,2},{8,2},{8,2},{8,2}}},
	{"blue_glass",true,{{8,3},{8,3},{8,3},{8,3},{8,3},{8,3}}},
	{"brick",false,{{10,0},{10,0},{10,0},{10,0},{10,0},{10,0}}},
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

GPUMesh block_outline;
void init_block_outline(){
	uint32_t c = RGBA(0,0,0,127);
	ColorVertex v[] = {
		0,1,0,c, 0,0,0,c,
		0,0,0,c, 0,0,1,c,
		0,0,1,c, 0,1,1,c,
		0,1,1,c, 0,1,0,c,

		1,1,0,c, 1,0,0,c,
		1,0,0,c, 1,0,1,c,
		1,0,1,c, 1,1,1,c,
		1,1,1,c, 1,1,0,c,

		0,1,0,c, 1,1,0,c,
		0,0,0,c, 1,0,0,c,
		0,0,1,c, 1,0,1,c,
		0,1,1,c, 1,1,1,c,
	};
	gpu_mesh_from_color_verts(&block_outline,v,COUNT(v));
}

float ambient_light_coefficients[6] = {0.6f,0.6f,0.5f,1.0f,0.8f,0.8f};

float light_coefficients[16];

void init_light_coefficients(){
	for (int i = 0; i < COUNT(light_coefficients); i++){
		float a = i/15.0f;
		light_coefficients[i] = a / ((1-a) * 3 + 1);
	}
}

void append_block_face(TextureColorVertexList *tvl, ivec3 pos, int face_id, BlockType *bt){
	TextureColorVertex *v = TextureColorVertexListMakeRoom(tvl,6);
	uint8_t ambient = ambient_light_coefficients[face_id] * 255;
	uint32_t color = RGBA(ambient,ambient,ambient,255);
	for (int i = 0; i < 6; i++){
		glm_vec3_add((vec3){pos[0],pos[1],pos[2]},cube_verts[face_id*6+i],v[i].position);
		uint8_t ambient = ambient_light_coefficients[face_id] * 255;
		v[i].color = color;
	}
	float w = 1.0f/16.0f;

	v[0].texcoord[0] = w*bt->faces[face_id][0];
	v[0].texcoord[1] = w*(16-bt->faces[face_id][1]);

	v[1].texcoord[0] = v[0].texcoord[0];
	v[1].texcoord[1] = v[0].texcoord[1] - w;

	v[2].texcoord[0] = v[0].texcoord[0] + w;
	v[2].texcoord[1] = v[1].texcoord[1];

	v[3].texcoord[0] = v[2].texcoord[0];
	v[3].texcoord[1] = v[2].texcoord[1];

	v[4].texcoord[0] = v[2].texcoord[0];
	v[4].texcoord[1] = v[0].texcoord[1];

	v[5].texcoord[0] = v[0].texcoord[0];
	v[5].texcoord[1] = v[0].texcoord[1];
}

void mesh_chunk(ChunkLinkedHashList *list, ChunkLinkedHashListBucket *b){
	/*
	Lighting:
	Max range of a single skylight is a chunk height column expanded out by 15 in all directions.
	Aka a CHUNK_HEIGHT * 31 * 31 volume.
	Rounding up, a CHUNK_HEIGHT * 32 * 32 length ring buffer is plenty for light propagation.
	You could potentially keep an MMBB for your lighting update and only remesh intersecting chunks.
	Propagate skylight, then propagate neighbor walls.
	static BlockPositionPair bpp_ring_buffer[CHUNK_HEIGHT*32*32]; //actual capacity = CHUNK_HEIGHT*32*32 - 1
	*/
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

	if (c->transparent_verts.used){
		free(c->transparent_verts.elements);
		memset(&c->transparent_verts,0,sizeof(c->transparent_verts));
	}
	TextureColorVertexList opaque_vl = {0};
	ivec3 bp;
	for (bp[1] = 0; bp[1] < CHUNK_HEIGHT; bp[1]++){
		for (bp[2] = 0; bp[2] < CHUNK_WIDTH; bp[2]++){
			for (bp[0] = 0; bp[0] < CHUNK_WIDTH; bp[0]++){
				BlockId id = c->blocks[BLOCK_AT(bp[0],bp[1],bp[2])].id;
				if (id){
					BlockType *bt = block_types + id;
					if ((bp[0] == 0 && neighbors[0] &&
						block_types[neighbors[0]->blocks[BLOCK_AT(CHUNK_WIDTH-1,bp[1],bp[2])].id].transparent) ||
						(bp[0] > 0 && block_types[c->blocks[BLOCK_AT(bp[0]-1,bp[1],bp[2])].id].transparent)){
						append_block_face(bt->transparent ? &c->transparent_verts : &opaque_vl,bp,0,bt);
					}
					if ((bp[0] == (CHUNK_WIDTH-1) && neighbors[1] &&
						block_types[neighbors[1]->blocks[BLOCK_AT(0,bp[1],bp[2])].id].transparent) ||
						(bp[0] < (CHUNK_WIDTH-1) && block_types[c->blocks[BLOCK_AT(bp[0]+1,bp[1],bp[2])].id].transparent)){
						append_block_face(bt->transparent ? &c->transparent_verts : &opaque_vl,bp,1,bt);
					}
					if (bp[1] == 0 || block_types[c->blocks[BLOCK_AT(bp[0],bp[1]-1,bp[2])].id].transparent){
						append_block_face(bt->transparent ? &c->transparent_verts : &opaque_vl,bp,2,bt);
					}
					if (bp[1] == (CHUNK_HEIGHT-1) || block_types[c->blocks[BLOCK_AT(bp[0],bp[1]+1,bp[2])].id].transparent){
						append_block_face(bt->transparent ? &c->transparent_verts : &opaque_vl,bp,3,bt);
					}
					if ((bp[2] == 0 && neighbors[2] &&
						block_types[neighbors[2]->blocks[BLOCK_AT(bp[0],bp[1],CHUNK_WIDTH-1)].id].transparent) ||
						(bp[2] > 0 && block_types[c->blocks[BLOCK_AT(bp[0],bp[1],bp[2]-1)].id].transparent)){
						append_block_face(bt->transparent ? &c->transparent_verts : &opaque_vl,bp,4,bt);
					}
					if ((bp[2] == (CHUNK_WIDTH-1) && neighbors[3]
						&& block_types[neighbors[3]->blocks[BLOCK_AT(bp[0],bp[1],0)].id].transparent) ||
						(bp[2] < (CHUNK_WIDTH-1) && block_types[c->blocks[BLOCK_AT(bp[0],bp[1],bp[2]+1)].id].transparent)){
						append_block_face(bt->transparent ? &c->transparent_verts : &opaque_vl,bp,5,bt);
					}
				}
			}
		}
	}
	if (c->opaque_verts.vertex_count){
		delete_gpu_mesh(&c->opaque_verts);
	}
	if (opaque_vl.used){
		gpu_mesh_from_texture_color_verts(&c->opaque_verts,opaque_vl.elements,opaque_vl.used);
		free(opaque_vl.elements);
	}
}

void world_pos_to_chunk_pos(vec3 world_pos, ivec2 chunk_pos){
	int i = world_pos[0];
	int k = world_pos[2];
	chunk_pos[0] = (world_pos[0] < 0 ? -1 : 0) + i/CHUNK_WIDTH;
	chunk_pos[1] = (world_pos[2] < 0 ? -1 : 0) + k/CHUNK_WIDTH;
}

Block *get_block(World *w, ivec3 pos){
	if (pos[1] < 0 || pos[1] > (CHUNK_HEIGHT-1)){
		return 0;
	}
	ChunkLinkedHashListBucket *b = ChunkLinkedHashListGetChecked(&w->chunks,(ivec2){(pos[0]+(pos[0]<0))/CHUNK_WIDTH-(pos[0]<0),(pos[2]+(pos[2]<0))/CHUNK_WIDTH-(pos[2]<0)});
	if (b){
		return b->chunk->blocks + BLOCK_AT(modulo(pos[0],CHUNK_WIDTH),pos[1],modulo(pos[2],CHUNK_WIDTH));
	} else {
		return 0;
	}
}

bool set_block(World *w, ivec3 pos, int id){
	if (pos[1] < 0 || pos[1] > (CHUNK_HEIGHT-1)){
		return false;
	}
	int cx = (pos[0]+(pos[0]<0))/CHUNK_WIDTH-(pos[0]<0);
	int cz = (pos[2]+(pos[2]<0))/CHUNK_WIDTH-(pos[2]<0);
	ChunkLinkedHashListBucket *b = ChunkLinkedHashListGetChecked(&w->chunks,(ivec2){cx,cz});
	if (b){
		Block *block = b->chunk->blocks + BLOCK_AT(modulo(pos[0],CHUNK_WIDTH),pos[1],modulo(pos[2],CHUNK_WIDTH));
		block->id = id;
		mesh_chunk(&w->chunks,b);
		ChunkLinkedHashListBucket *neighbors[] = {
			ChunkLinkedHashListGetChecked(&w->chunks,(ivec2){cx-1,cz}),
			ChunkLinkedHashListGetChecked(&w->chunks,(ivec2){cx+1,cz}),
			ChunkLinkedHashListGetChecked(&w->chunks,(ivec2){cx,cz-1}),
			ChunkLinkedHashListGetChecked(&w->chunks,(ivec2){cx,cz+1}),
		};
		for (int i = 0; i < 4; i++){
			if (neighbors[i]){
				mesh_chunk(&w->chunks,neighbors[i]);
			}
		}
	} else {
		return 0;
	}
}

void cast_ray_into_blocks(World *w, vec3 origin, vec3 ray, BlockRayCastResult *result){
	result->block_pos[0] = floorf(origin[0]);
	result->block_pos[1] = floorf(origin[1]);
	result->block_pos[2] = floorf(origin[2]);
	vec3 da;
	for (int i = 0; i < 3; i++){
		if (ray[i] < 0){
			da[i] = ((float)result->block_pos[i]-origin[i]) / ray[i];
		} else {
			da[i] = ((float)result->block_pos[i]+1.0f-origin[i]) / ray[i];	
		}
	}
	result->t = 0;
	int index = 0;
	while (result->t <= 1.0f){
		result->block = get_block(w,result->block_pos);
		if (result->block && result->block->id){
			glm_ivec3_zero(result->face_normal);
			result->face_normal[index] = ray[index] < 0 ? 1 : -1;
			return;
		}
		float d = HUGE_VALF;
		index = 0;
		for (int i = 0; i < 3; i++){
			if (da[i] < d){
				index = i;
				d = da[i];
			}
		}
		result->block_pos[index] += ray[index] < 0 ? -1 : 1;
		result->t += da[index];
		glm_vec3_subs(da,d,da);
		da[index] = fabsf(1.0f / ray[index]);
	}
	result->block = 0;
}