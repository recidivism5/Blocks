#pragma once

#include <zlib.h>
#include <tinydir.h>

#include <renderer.h>
#include <perlin_noise.h>
#include <aabb.h>

TSTRUCT(BlockType){
	char *name;
	bool transparent;
	int faces[6][2];
};

BlockType block_types[];

typedef enum {
	BLOCK_AIR,
	BLOCK_BEDROCK,
	BLOCK_STONE,
	BLOCK_DIRT,
	BLOCK_GRASS,
	BLOCK_LOG,
	BLOCK_GLASS,
	BLOCK_RED_GLASS,
	BLOCK_GREEN_GLASS,
	BLOCK_BLUE_GLASS,
	BLOCK_BRICK,
} BlockId;

#define CHUNK_WIDTH 16
#define CHUNK_HEIGHT 256
#define BLOCK_AT(x,y,z) ((y)*CHUNK_WIDTH*CHUNK_WIDTH + (z)*CHUNK_WIDTH + (x))

TSTRUCT(Block){
	uint8_t id, light[3];
};

#define SKYLIGHT(v) ((v)>>4)
#define BLOCKLIGHT(v) ((v)&0x0f)

TSTRUCT(Chunk){
	bool neighbors_exist[4];
	Block blocks[CHUNK_WIDTH*CHUNK_WIDTH*CHUNK_HEIGHT];
	TextureColorVertexList transparent_verts;
	GPUMesh opaque_verts;
};

TSTRUCT(ChunkLinkedHashListBucket){
	ChunkLinkedHashListBucket *prev, *next;
	ivec2 position;
	Chunk *chunk;
};

TSTRUCT(ChunkLinkedHashList){
	size_t total, used, tombstones;
	ChunkLinkedHashListBucket *buckets, *first, *last;
};

#define TOMBSTONE UINTPTR_MAX

ChunkLinkedHashListBucket *ChunkLinkedHashListGet(ChunkLinkedHashList *list, ivec2 position);

ChunkLinkedHashListBucket *ChunkLinkedHashListGetChecked(ChunkLinkedHashList *list, ivec2 position);

void ChunkLinkedHashListRemove(ChunkLinkedHashList *list, ChunkLinkedHashListBucket *b);

ChunkLinkedHashListBucket *ChunkLinkedHashListNew(ChunkLinkedHashList *list, ivec2 position);

void gen_chunk(ChunkLinkedHashListBucket *b);

extern vec3 cube_verts[];

extern GPUMesh block_outline;

void init_block_outline();

void init_light_coefficients();

void append_block_face(TextureColorVertexList *tvl, ivec3 pos, int face_id, Block *neighbor, BlockType *bt);

void light_chunk(ChunkLinkedHashList *chunks, ChunkLinkedHashListBucket *bucket);

void mesh_chunk(ChunkLinkedHashList *list, ChunkLinkedHashListBucket *b);

TSTRUCT(Region){
	FILE *file_ptr;
	struct {
		int sector_number;
		int sector_count;
	} chunk_positions[32*32];
	int sector_count;
	bool *sector_usage;
};

TSTRUCT(World){
	ChunkLinkedHashList chunks;
};

void world_pos_to_chunk_pos(vec3 world_pos, ivec2 chunk_pos);

Block *get_block(ChunkLinkedHashList *chunks, ivec3 pos);

bool set_block(ChunkLinkedHashList *chunks, ivec3 pos, int id);

TSTRUCT(BlockRayCastResult){
	Block *block;
	ivec3 block_pos;
	ivec3 face_normal;
	float t;
};

void cast_ray_into_blocks(ChunkLinkedHashList *chunks, vec3 origin, vec3 ray, BlockRayCastResult *result);