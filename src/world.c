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
	uint8_t block_ids[CHUNK_WIDTH*CHUNK_WIDTH*CHUNK_HEIGHT];
	uint8_t block_light_values[CHUNK_WIDTH*CHUNK_WIDTH*CHUNK_HEIGHT];
	GLuint vbo_id;
	int vertex_count;
} Chunk;
#if INCLUDED == 0
LINKED_HASHLIST_IMPLEMENTATION(Chunk)
#else
LINKED_HASHLIST_HEADER(Chunk)
#endif

#define BLOCK_AT(x,y,z) ((y)*CHUNK_WIDTH*CHUNK_WIDTH + (z)*CHUNK_WIDTH + (x))
void gen_chunk(Chunk *c)
#if INCLUDED == 0
{
	for (int y = 0; y < CHUNK_HEIGHT; y++){
		for (int z = 0; z < CHUNK_WIDTH; z++){
			for (int x = 0; x < CHUNK_WIDTH; x++){
				if (y < 31){
					c->block_ids[BLOCK_AT(x,y,z)] = BLOCK_DIRT;
				} else if (y == 31){
					c->block_ids[BLOCK_AT(x,y,z)] = BLOCK_GRASS;
				} else {
					c->block_ids[BLOCK_AT(x,y,z)] = BLOCK_AIR;
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
	tvl->used += 6;
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

void mesh_chunk(Chunk *c)
#if INCLUDED == 0
{
	TextureColorVertexList tvl = {0};
	for (int y = 0; y < CHUNK_HEIGHT; y++){
		for (int z = 0; z < CHUNK_WIDTH; z++){
			for (int x = 0; x < CHUNK_WIDTH; x++){
				BlockId id = c->block_ids[BLOCK_AT(x,y,z)];
				if (id){
					BlockType *bt = block_types + id;
					if (x == 0 || !c->block_ids[BLOCK_AT(x-1,y,z)]){
						append_block_face(&tvl,x,y,z,0,bt);
					}
					if (x == (CHUNK_WIDTH-1) || !c->block_ids[BLOCK_AT(x+1,y,z)]){
						append_block_face(&tvl,x,y,z,1,bt);
					}
					if (y == 0 || !c->block_ids[BLOCK_AT(x,y-1,z)]){
						append_block_face(&tvl,x,y,z,2,bt);
					}
					if (y == (CHUNK_HEIGHT-1) || !c->block_ids[BLOCK_AT(x,y+1,z)]){
						append_block_face(&tvl,x,y,z,3,bt);
					}
					if (z == 0 || !c->block_ids[BLOCK_AT(x,y,z-1)]){
						append_block_face(&tvl,x,y,z,4,bt);
					}
					if (z == (CHUNK_WIDTH-1) || !c->block_ids[BLOCK_AT(x,y,z+1)]){
						append_block_face(&tvl,x,y,z,5,bt);
					}
				}
			}
		}
	}
	if (!c->vbo_id){
		glGenBuffers(1,&c->vbo_id);
	}
	glBindBuffer(GL_ARRAY_BUFFER,c->vbo_id);
	glBufferData(GL_ARRAY_BUFFER,tvl.used*sizeof(*tvl.elements),tvl.elements,GL_STATIC_DRAW);
	c->vertex_count = tvl.used;
	if (tvl.elements){
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
#if INCLUDED == 0
LINKED_HASHLIST_IMPLEMENTATION(Region)
#else
LINKED_HASHLIST_HEADER(Region)
#endif

typedef struct {
	ChunkLinkedHashList chunks;
	RegionLinkedHashList regions;
} World;