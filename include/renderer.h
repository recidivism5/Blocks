#pragma once

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <png.h>
#include <cglm/cglm.h>

#include <base.h>

GLenum glCheckError_(const char *file, int line);
#define glCheckError() glCheckError_(__FILE__, __LINE__)

TSTRUCT(Texture){
	GLuint id;
	int width, height;
};

void texture_from_file(Texture *t, char *path);

void check_shader(char *name, char *type, GLuint id);

void check_program(char *name, char *status_name, GLuint id, GLenum param);

GLuint compile_shader(char *name, char *vert_src, char *frag_src);

TSTRUCT(GPUMesh){
	GLuint vao, vbo;
	int vertex_count;
};

TSTRUCT(ColorVertex){
	float x,y,z;
	uint32_t color;
};

TSTRUCT(ColorShader){
	char *vert_src;
	char *frag_src;
	GLuint id;
	GLint aPosition;
	GLint aColor;
	GLint uMVP;
} color_shader;

TSTRUCT(TextureColorVertex){
	float x,y,z, u,v;
	uint32_t color;
};

TSTRUCT(TextureColorVertexList){
	int total, used;
	TextureColorVertex *elements;
};

TSTRUCT(TextureColorShader){
	char *vert_src;
	char *frag_src;
	GLuint id;
	GLint aPosition;
	GLint aTexCoord;
	GLint aColor;
	GLint uMVP;
	GLint uTex;
} texture_color_shader;

TextureColorVertex *TextureColorVertexListMakeRoom(TextureColorVertexList *list, int count);

void gpu_mesh_from_color_verts(GPUMesh *m, ColorVertex *verts, int count);

void gpu_mesh_from_texture_color_verts(GPUMesh *m, TextureColorVertex *verts, int count);

void delete_gpu_mesh(GPUMesh *m);

void compile_shaders();