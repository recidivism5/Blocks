#pragma once
#if defined __INTELLISENSE__
#undef INCLUDED
#endif
#ifdef INCLUDED
#define INCLUDED 1
#else
#define INCLUDED 0
#endif
#pragma push_macro("INCLUDED")

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <stdint.h>
#include <zlib.h>
#include <tinydir.h>
#include <cglm/cglm.h>

#include "renderer.c"

#pragma pop_macro("INCLUDED")

/*
Scaevolus' Region File Format explanation:
https://www.minecraftforum.net/forums/mapping-and-modding-java-edition/minecraft-mods/mods-discussion/1346703-mod-mcregion-v5-optimized-saves-1-2_02?comment=107

Further comment from Scaevolus:
https://www.reddit.com/r/Minecraft/comments/evzbs/comment/c1bg51k/?utm_source=share&utm_medium=web2x&context=3

Map format:
https://wiki.vg/Map_Format
*/
typedef struct {
	char *name;
	int faces[6][2];
} BlockType;

#if INCLUDED == 0
BlockType block_types[] = {
	{"air",{{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}}},
	{"bedrock",{{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}}},
	{"stone",{{1,0},{1,0},{1,0},{1,0},{1,0},{1,0}}},
	{"dirt",{{0,3},{0,3},{0,3},{0,3},{0,3},{0,3}}},
	{"grass",{{0,2},{0,2},{0,3},{0,1},{0,2},{0,2}}},
};
#else
extern BlockType block_types[];
#endif

typedef enum {
	BLOCK_AIR,
	BLOCK_BEDROCK,
	BLOCK_STONE,
	BLOCK_DIRT,
	BLOCK_GRASS
} BlockId;

#define CHUNK_WIDTH 16
#define CHUNK_HEIGHT 256
typedef struct {
	uint8_t id, r, g, b;
} Block;
typedef struct {
	bool neighbors_exist[4];
	Block blocks[CHUNK_WIDTH*CHUNK_WIDTH*CHUNK_HEIGHT];
	GLuint vbo_id;
	int vertex_count;
} Chunk;

typedef struct ChunkLinkedHashListBucket {
	struct ChunkLinkedHashListBucket *prev, *next;
	ivec2 position;
	Chunk *chunk;
} ChunkLinkedHashListBucket;

typedef struct {
	size_t total, used, tombstones;
	ChunkLinkedHashListBucket *buckets, *first, *last;
} ChunkLinkedHashList;

#define TOMBSTONE UINTPTR_MAX

ChunkLinkedHashListBucket *ChunkLinkedHashListGet(ChunkLinkedHashList *list, ivec2 position)
#if INCLUDED == 0
{
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
#else
;
#endif

ChunkLinkedHashListBucket *ChunkLinkedHashListGetChecked(ChunkLinkedHashList *list, ivec2 position)
#if INCLUDED == 0
{
	ChunkLinkedHashListBucket *b = ChunkLinkedHashListGet(list,position);
	if (!b || b->chunk == 0 || b->chunk == TOMBSTONE) return 0;
	return b;
}
#else
;
#endif

void ChunkLinkedHashListRemove(ChunkLinkedHashList *list, ChunkLinkedHashListBucket *b)
#if INCLUDED == 0
{
	b->chunk = TOMBSTONE;
	if (b->prev) b->prev->next = b->next;
	if (b->next) b->next->prev = b->prev;
	if (list->first == b) list->first = b->next;
	if (list->last == b) list->last = b->prev;
	list->used--;
	list->tombstones++;
}
#else
;
#endif

ChunkLinkedHashListBucket *ChunkLinkedHashListNew(ChunkLinkedHashList *list, ivec2 position)
#if INCLUDED == 0
{
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
#else
;
#endif

#define BLOCK_AT(x,y,z) ((y)*CHUNK_WIDTH*CHUNK_WIDTH + (z)*CHUNK_WIDTH + (x))
void gen_chunk(Chunk *c)
#if INCLUDED == 0
{
	int h = rand_int(CHUNK_HEIGHT/8);
	for (int y = 0; y < CHUNK_HEIGHT; y++){
		for (int z = 0; z < CHUNK_WIDTH; z++){
			for (int x = 0; x < CHUNK_WIDTH; x++){
				if (y < h){
					c->blocks[BLOCK_AT(x,y,z)].id = BLOCK_DIRT;
				} else if (y == h){
					c->blocks[BLOCK_AT(x,y,z)].id = BLOCK_GRASS;
				} else {
					c->blocks[BLOCK_AT(x,y,z)].id = BLOCK_AIR;
				}
			}
		}
	}
}
#else
;
#endif

#if INCLUDED == 0
vec3 cube_verts[] = {
	0,1,0, 0,0,0, 0,0,1, 0,0,1, 0,1,1, 0,1,0,
	1,1,1, 1,0,1, 1,0,0, 1,0,0, 1,1,0, 1,1,1,

	0,0,0, 1,0,0, 1,0,1, 1,0,1, 0,0,1, 0,0,0,
	1,1,0, 0,1,0, 0,1,1, 0,1,1, 1,1,1, 1,1,0,

	1,1,0, 1,0,0, 0,0,0, 0,0,0, 0,1,0, 1,1,0,
	0,1,1, 0,0,1, 1,0,1, 1,0,1, 1,1,1, 0,1,1,
};
#else
extern vec3 cube_verts[];
#endif

void append_block_face(TextureColorVertexList *tvl, int x, int y, int z, int face_id, BlockType *bt)
#if INCLUDED == 0
{
	TextureColorVertex *v = TextureColorVertexListMakeRoom(tvl,6);
	for (int i = 0; i < 6; i++){
		v[i].x = x + cube_verts[face_id*6+i][0];
		v[i].y = y + cube_verts[face_id*6+i][1];
		v[i].z = z + cube_verts[face_id*6+i][2];

		v[i].color = 0xffffffff;
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
#else
;
#endif

void mesh_chunk(ChunkLinkedHashList *list, ChunkLinkedHashListBucket *b)
#if INCLUDED == 0
{
	Chunk *c = b->chunk;
	TextureColorVertexList tvl = {0};
	for (int y = 0; y < CHUNK_HEIGHT; y++){
		for (int z = 0; z < CHUNK_WIDTH; z++){
			for (int x = 0; x < CHUNK_WIDTH; x++){
				BlockId id = c->blocks[BLOCK_AT(x,y,z)].id;
				if (id){
					BlockType *bt = block_types + id;
					if (x == 0 || !c->blocks[BLOCK_AT(x-1,y,z)].id){
						append_block_face(&tvl,x,y,z,0,bt);
					}
					if (x == (CHUNK_WIDTH-1) || !c->blocks[BLOCK_AT(x+1,y,z)].id){
						append_block_face(&tvl,x,y,z,1,bt);
					}
					if (y == 0 || !c->blocks[BLOCK_AT(x,y-1,z)].id){
						append_block_face(&tvl,x,y,z,2,bt);
					}
					if (y == (CHUNK_HEIGHT-1) || !c->blocks[BLOCK_AT(x,y+1,z)].id){
						append_block_face(&tvl,x,y,z,3,bt);
					}
					if (z == 0 || !c->blocks[BLOCK_AT(x,y,z-1)].id){
						append_block_face(&tvl,x,y,z,4,bt);
					}
					if (z == (CHUNK_WIDTH-1) || !c->blocks[BLOCK_AT(x,y,z+1)].id){
						append_block_face(&tvl,x,y,z,5,bt);
					}
				}
			}
		}
	}
	if (tvl.elements){
		if (!c->vbo_id){
			glGenBuffers(1,&c->vbo_id);
		}
		glBindBuffer(GL_ARRAY_BUFFER,c->vbo_id);
		glBufferData(GL_ARRAY_BUFFER,tvl.used*sizeof(*tvl.elements),tvl.elements,GL_STATIC_DRAW);
		c->vertex_count = tvl.used;
		free(tvl.elements);
	}
}
#else
;
#endif

typedef struct {
	FILE *file_ptr;
	struct {
		int sector_number;
		int sector_count;
	} chunk_positions[32*32];
	int sector_count;
	bool *sector_usage;
} Region;

typedef struct {
	ChunkLinkedHashList chunks;
} World;

Block *get_block(World *w, int x, int y, int z)
#if INCLUDED == 0
{
	ChunkLinkedHashListBucket *b = ChunkLinkedHashListGetChecked(&w->chunks,(ivec2){(x+(x<0))/CHUNK_WIDTH-(x<0),(z+(z<0))/CHUNK_WIDTH-(z<0)});
	if (b){
		return b->chunk->blocks + BLOCK_AT(modulo(x,CHUNK_WIDTH),y,modulo(z,CHUNK_WIDTH));
	} else {
		return 0;
	}
}
#else
;
#endif