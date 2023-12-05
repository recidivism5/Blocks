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
	if ((list->used+list->tombstones+1) > (list->total*3)/4){ // 3/4 load limit, see benchmark/HashTableBenchmarkResults.txt
		ChunkLinkedHashList newList;
		newList.total = 16; // same as used resize factor
		while (newList.total < (list->used*16)) newList.total *= 2; // 16x used resize, see benchmark/HashTableBenchmarkResults.txt
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
	ivec2 cbase = {b->position[0]*CHUNK_WIDTH,b->position[1]*CHUNK_WIDTH};
	static float heightmap[3][3];
	for (int x = 0; x < 3; x++){
		for (int z = 0; z < 3; z++){
			heightmap[x][z] = 0.5f+0.25f*fractal_perlin_noise_2d(cbase[0]+x*8,cbase[1]+z*8,0.005f,4);
		}
	}
	static float densitymap[3][CHUNK_HEIGHT/8+1][3];
	float invh = 1.0f / CHUNK_HEIGHT;
	for (int x = 0; x < 3; x++){
		for (int y = 0; y < (CHUNK_HEIGHT/8+1); y++){
			for (int z = 0; z < 3; z++){
				densitymap[x][y][z] = fractal_perlin_noise_3d(cbase[0]+x*8,y*8,cbase[1]+z*8,0.005f,8);
			}
		}
	}
	float inv8 = 1.0f/8.0f;
	for (int y = 0; y < CHUNK_HEIGHT; y++){
		for (int z = 0; z < CHUNK_WIDTH; z++){
			for (int x = 0; x < CHUNK_WIDTH; x++){
				int xd = x/8, yd = y/8, zd = z/8;
				float xi = (x%8)*inv8, yi = (y%8)*inv8, zi = (z%8)*inv8;
				float s = 
					glm_lerp(
						glm_lerp(
							glm_lerp(densitymap[xd][yd][zd],densitymap[xd+1][yd][zd],xi),
							glm_lerp(densitymap[xd][yd+1][zd],densitymap[xd+1][yd+1][zd],xi),
							yi
						),
						glm_lerp(
							glm_lerp(densitymap[xd][yd][zd+1],densitymap[xd+1][yd][zd+1],xi),
							glm_lerp(densitymap[xd][yd+1][zd+1],densitymap[xd+1][yd+1][zd+1],xi),
							yi
						),
						zi
					)
					+ 1.5f * (glm_lerp(
						glm_lerp(heightmap[xd][zd],heightmap[xd+1][zd],xi),
						glm_lerp(heightmap[xd][zd+1],heightmap[xd+1][zd+1],xi),
						zi
					) - y*invh);
				if (s > 0.0f){
					c->blocks[BLOCK_AT(x,y,z)].id = BLOCK_DIRT;
				} else {
					c->blocks[BLOCK_AT(x,y,z)].id = BLOCK_AIR;
				}
			}
		}
	}
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

void append_block_face(TextureColorVertexList *tvl, ivec3 pos, int face_id, Block *neighbor, BlockType *bt){
	TextureColorVertex *v = TextureColorVertexListMakeRoom(tvl,6);
	float ambient = ambient_light_coefficients[face_id];
	uint32_t color;
	if (0 && neighbor){
		uint8_t *c = &color;
		c[0] = 255*(ambient*light_coefficients[MAX(SKYLIGHT(neighbor->light[0]),BLOCKLIGHT(neighbor->light[0]))]);
		c[1] = 255*(ambient*light_coefficients[MAX(SKYLIGHT(neighbor->light[1]),BLOCKLIGHT(neighbor->light[1]))]);
		c[2] = 255*(ambient*light_coefficients[MAX(SKYLIGHT(neighbor->light[2]),BLOCKLIGHT(neighbor->light[2]))]);
		c[3] = 255;
	} else {
		color = RGBA(
			(int)(255*ambient),
			(int)(255*ambient),
			(int)(255*ambient),
			255
		);
	}
	vec3 fpos = {pos[0],pos[1],pos[2]};
	for (int i = 0; i < 6; i++){
		glm_vec3_add(fpos,cube_verts[face_id*6+i],v[i].position);
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

TSTRUCT(LightPropRingBuffer){
	uint16_t positions[CHUNK_HEIGHT*CHUNK_WIDTH*CHUNK_WIDTH];
	uint16_t write_index, read_index;
};

void light_chunk(ChunkLinkedHashList *chunks, ChunkLinkedHashListBucket *bucket){
	/*
	Lighting:
	Propagate all light sources in chunk,
	then propagate walls into neighboring chunks,
	then propagate neighboring walls into this chunk

	https://web.archive.org/web/20150910104528/https://www.seedofandromeda.com/blogs/29-fast-flood-fill-lighting-in-a-blocky-voxel-game-pt-1
	https://web.archive.org/web/20150910103315/https://www.seedofandromeda.com/blogs/30-fast-flood-fill-lighting-in-a-blocky-voxel-game-pt-2
	https://0fps.net/2018/02/21/voxel-lighting/
	https://gamedev.stackexchange.com/questions/91926/how-can-i-build-minecraft-style-light-propagation-without-recursive-functions

	Ambient occlusion: https://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/
	*/
}

void mesh_chunk(ChunkLinkedHashList *chunks, ChunkLinkedHashListBucket *bucket){
	/*
	benchmark:
	meshing:
		do each edge, excluding corners
		do each corner
		do middle

		vs current method
	*/
	Chunk *c = bucket->chunk;
	ivec3 bp;

	ChunkLinkedHashListBucket *neighbor_buckets[4] = {
		ChunkLinkedHashListGetChecked(chunks,(ivec2){bucket->position[0]-1,bucket->position[1]}),
		ChunkLinkedHashListGetChecked(chunks,(ivec2){bucket->position[0]+1,bucket->position[1]}),
		ChunkLinkedHashListGetChecked(chunks,(ivec2){bucket->position[0],bucket->position[1]-1}),
		ChunkLinkedHashListGetChecked(chunks,(ivec2){bucket->position[0],bucket->position[1]+1})
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
	if (neighbors[0] && !neighbors[0]->neighbors_exist[1]) mesh_chunk(chunks,neighbor_buckets[0]);
	if (neighbors[1] && !neighbors[1]->neighbors_exist[0]) mesh_chunk(chunks,neighbor_buckets[1]);
	if (neighbors[2] && !neighbors[2]->neighbors_exist[3]) mesh_chunk(chunks,neighbor_buckets[2]);
	if (neighbors[3] && !neighbors[3]->neighbors_exist[2]) mesh_chunk(chunks,neighbor_buckets[3]);

	if (c->transparent_verts.used){
		free(c->transparent_verts.elements);
		memset(&c->transparent_verts,0,sizeof(c->transparent_verts));
	}
	TextureColorVertexList opaque_vl = {0};

	for (bp[1] = 0; bp[1] < CHUNK_HEIGHT; bp[1]++){
		for (bp[2] = 0; bp[2] < CHUNK_WIDTH; bp[2]++){
			for (bp[0] = 0; bp[0] < CHUNK_WIDTH; bp[0]++){
				BlockId id = c->blocks[BLOCK_AT(bp[0],bp[1],bp[2])].id;
				if (id){
					BlockType *bt = block_types + id;
					Block *neighbor_block;

					if (bp[0] == 0){
						if (neighbors[0]){
							neighbor_block = neighbors[0]->blocks+BLOCK_AT(CHUNK_WIDTH-1,bp[1],bp[2]);
						} else {
							goto L0;
						}
					} else {
						neighbor_block = c->blocks+BLOCK_AT(bp[0]-1,bp[1],bp[2]);
					}
					if (block_types[neighbor_block->id].transparent){
						append_block_face(bt->transparent ? &c->transparent_verts : &opaque_vl,bp,0,neighbor_block,bt);
					}
					L0:

					if (bp[0] == (CHUNK_WIDTH-1)){
						if (neighbors[1]){
							neighbor_block = neighbors[1]->blocks+BLOCK_AT(0,bp[1],bp[2]);
						} else {
							goto L1;
						}
					} else {
						neighbor_block = c->blocks+BLOCK_AT(bp[0]+1,bp[1],bp[2]);
					}
					if (block_types[neighbor_block->id].transparent){
						append_block_face(bt->transparent ? &c->transparent_verts : &opaque_vl,bp,1,neighbor_block,bt);
					}
					L1:

					if (bp[1] > 0){
						neighbor_block = c->blocks+BLOCK_AT(bp[0],bp[1]-1,bp[2]);
						if (block_types[neighbor_block->id].transparent){
							append_block_face(bt->transparent ? &c->transparent_verts : &opaque_vl,bp,2,neighbor_block,bt);
						}
					} else {
						append_block_face(bt->transparent ? &c->transparent_verts : &opaque_vl,bp,2,0,bt);
					}

					if (bp[1] < (CHUNK_HEIGHT-1)){
						neighbor_block = c->blocks+BLOCK_AT(bp[0],bp[1]+1,bp[2]);
						if (block_types[neighbor_block->id].transparent){
							append_block_face(bt->transparent ? &c->transparent_verts : &opaque_vl,bp,3,neighbor_block,bt);
						}
					} else {
						append_block_face(bt->transparent ? &c->transparent_verts : &opaque_vl,bp,3,0,bt);
					}

					if (bp[2] == 0){
						if (neighbors[2]){
							neighbor_block = neighbors[2]->blocks+BLOCK_AT(bp[0],bp[1],CHUNK_WIDTH-1);
						} else {
							goto L2;
						}
					} else {
						neighbor_block = c->blocks+BLOCK_AT(bp[0],bp[1],bp[2]-1);
					}
					if (block_types[neighbor_block->id].transparent){
						append_block_face(bt->transparent ? &c->transparent_verts : &opaque_vl,bp,4,neighbor_block,bt);
					}
					L2:

					if (bp[2] == (CHUNK_WIDTH-1)){
						if (neighbors[3]){
							neighbor_block = neighbors[3]->blocks+BLOCK_AT(bp[0],bp[1],0);
						} else {
							goto L3;
						}
					} else {
						neighbor_block = c->blocks+BLOCK_AT(bp[0],bp[1],bp[2]+1);
					}
					if (block_types[neighbor_block->id].transparent){
						append_block_face(bt->transparent ? &c->transparent_verts : &opaque_vl,bp,5,neighbor_block,bt);
					}
					L3:;
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

Block *get_block(ChunkLinkedHashList *chunks, ivec3 pos){
	if (pos[1] < 0 || pos[1] > (CHUNK_HEIGHT-1)){
		return 0;
	}
	ChunkLinkedHashListBucket *b = ChunkLinkedHashListGetChecked(chunks,(ivec2){(pos[0]+(pos[0]<0))/CHUNK_WIDTH-(pos[0]<0),(pos[2]+(pos[2]<0))/CHUNK_WIDTH-(pos[2]<0)});
	if (b){
		return b->chunk->blocks + BLOCK_AT(modulo(pos[0],CHUNK_WIDTH),pos[1],modulo(pos[2],CHUNK_WIDTH));
	} else {
		return 0;
	}
}

bool set_block(ChunkLinkedHashList *chunks, ivec3 pos, int id){
	if (pos[1] < 0 || pos[1] > (CHUNK_HEIGHT-1)){
		return false;
	}
	int cx = (pos[0]+(pos[0]<0))/CHUNK_WIDTH-(pos[0]<0);
	int cz = (pos[2]+(pos[2]<0))/CHUNK_WIDTH-(pos[2]<0);
	ChunkLinkedHashListBucket *b = ChunkLinkedHashListGetChecked(chunks,(ivec2){cx,cz});
	if (b){
		Block *block = b->chunk->blocks + BLOCK_AT(modulo(pos[0],CHUNK_WIDTH),pos[1],modulo(pos[2],CHUNK_WIDTH));
		block->id = id;
		mesh_chunk(chunks,b);
		ChunkLinkedHashListBucket *neighbors[] = {
			ChunkLinkedHashListGetChecked(chunks,(ivec2){cx-1,cz}),
			ChunkLinkedHashListGetChecked(chunks,(ivec2){cx+1,cz}),
			ChunkLinkedHashListGetChecked(chunks,(ivec2){cx,cz-1}),
			ChunkLinkedHashListGetChecked(chunks,(ivec2){cx,cz+1}),
		};
		for (int i = 0; i < 4; i++){
			if (neighbors[i]){
				mesh_chunk(chunks,neighbors[i]);
			}
		}
	} else {
		return 0;
	}
}

void cast_ray_into_blocks(ChunkLinkedHashList *chunks, vec3 origin, vec3 ray, BlockRayCastResult *result){
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
		result->block = get_block(chunks,result->block_pos);
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