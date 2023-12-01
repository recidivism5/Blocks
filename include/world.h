#pragma once

#include <zlib.h>
#include <tinydir.h>

#include <renderer.h>
#include <perlin_noise.h>

typedef struct {
	char *name;
	int faces[6][2];
} BlockType;

BlockType block_types[];

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
	GPUMesh mesh;
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

ChunkLinkedHashListBucket *ChunkLinkedHashListGet(ChunkLinkedHashList *list, ivec2 position);

ChunkLinkedHashListBucket *ChunkLinkedHashListGetChecked(ChunkLinkedHashList *list, ivec2 position);

void ChunkLinkedHashListRemove(ChunkLinkedHashList *list, ChunkLinkedHashListBucket *b);

ChunkLinkedHashListBucket *ChunkLinkedHashListNew(ChunkLinkedHashList *list, ivec2 position);

#define BLOCK_AT(x,y,z) ((y)*CHUNK_WIDTH*CHUNK_WIDTH + (z)*CHUNK_WIDTH + (x))

void gen_chunk(ChunkLinkedHashListBucket *b);

vec3 cube_verts[];

void append_block_face(TextureColorVertexList *tvl, int x, int y, int z, int face_id, BlockType *bt);

void mesh_chunk(ChunkLinkedHashList *list, ChunkLinkedHashListBucket *b);

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

Block *get_block(World *w, int x, int y, int z);

Block *block_raycast(World *w, vec3 origin, vec3 ray, float *t, ivec3 block_pos, ivec3 face_normal);