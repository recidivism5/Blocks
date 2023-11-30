#pragma once

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <png.h>
#include <cglm/cglm.h>

#include <base.h>

GLenum glCheckError_(const char *file, int line);
#define glCheckError() glCheckError_(__FILE__, __LINE__)

typedef struct {
	GLuint id;
	int width, height;
} Texture;

void texture_from_file(Texture *t, char *path);

void check_shader(char *name, char *type, GLuint id);

void check_program(char *name, char *status_name, GLuint id, GLenum param);

GLuint compile_shader(char *name, char *vert_src, char *frag_src);

typedef struct {
	float x,y,z, u,v;
	uint32_t color;
} TextureColorVertex;

typedef struct {
	size_t total, used;
	TextureColorVertex *elements;
} TextureColorVertexList;

TextureColorVertex *TextureColorVertexListMakeRoom(TextureColorVertexList *list, size_t count);

struct TextureColorShader {
	char *vert_src;
	char *frag_src;
	GLuint id;
	GLint aPosition;
	GLint aTexCoord;
	GLint aColor;
	GLint uMVP;
	GLint uTex;
} texture_color_shader;

#define COMPILE_SHADER(s) s.id = compile_shader(#s,s.vert_src,s.frag_src)
#define GET_ATTRIB(s,a) s.a = glGetAttribLocation(s.id,#a)
#define GET_UNIFORM(s,u) s.u = glGetUniformLocation(s.id,#u)

void compile_texture_color_shader();

void texture_color_shader_prep_buffer();

void compile_shaders();

typedef struct {
	vec3 position;
	vec3 euler;
	float fov_radians;
} Camera;