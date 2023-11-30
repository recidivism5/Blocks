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
#include <zlib.h>
#include <png.h>

#include "base.c"

#pragma pop_macro("INCLUDED")

GLenum glCheckError_(const char *file, int line)
#if INCLUDED == 0
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR){
		char *error;
		switch (errorCode){
		case GL_INVALID_ENUM:      error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:     error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION: error = "INVALID_OPERATION"; break;
		case GL_OUT_OF_MEMORY:     error = "OUT_OF_MEMORY"; break;
		default: error = "UNKNOWN TYPE BEAT";break;
		}
		fatal_error("%s %s (%d)",error,file,line);
	}
	return errorCode;
}
#else
;
#endif
#define glCheckError() glCheckError_(__FILE__, __LINE__)

typedef struct {
	GLuint id;
	int width, height;
} Texture;

void texture_from_file(Texture *t, char *path)
#if INCLUDED == 0
{
	//from https://gist.github.com/Svensational/6120653
	png_image image = {0};
	image.version = PNG_IMAGE_VERSION;
	if (!png_image_begin_read_from_file(&image,path)){
		fatal_error("texture_from_file: failed to load %s",path);
	}
	image.format = PNG_FORMAT_RGBA;
	png_byte *buffer = malloc_or_die(PNG_IMAGE_SIZE(image));
	// a negative stride indicates that the bottom-most row is first in the buffer
	// (as expected by openGL)
	if (!png_image_finish_read(&image, NULL, buffer, -image.width*4, NULL)) {
		png_image_free(&image);
		fatal_error("texture_from_file: failed to load %s",path);
	}
	t->width = image.width;
	t->height = image.height;
	glGenTextures(1,&t->id);
	glBindTexture(GL_TEXTURE_2D,t->id);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,t->width,t->height,0,GL_RGBA,GL_UNSIGNED_BYTE,buffer);
	free(buffer);
}
#else
;
#endif

void check_shader(char *name, char *type, GLuint id)
#if INCLUDED == 0
{
	GLint result;
	glGetShaderiv(id,GL_COMPILE_STATUS,&result);
	if (!result){
		char infolog[512];
		glGetShaderInfoLog(id,512,NULL,infolog);
		fatal_error("%s %s shader compile error: %s",name,type,infolog);
	}
}
#else
;
#endif

void check_program(char *name, char *status_name, GLuint id, GLenum param)
#if INCLUDED == 0
{
	GLint result;
	glGetProgramiv(id,param,&result);
	if (!result){
		char infolog[512];
		glGetProgramInfoLog(id,512,NULL,infolog);
		fatal_error("%s shader %s error: %s",name,status_name,infolog);
	}
}
#else
;
#endif

GLuint compile_shader(char *name, char *vert_src, char *frag_src)
#if INCLUDED == 0
{
	GLuint v = glCreateShader(GL_VERTEX_SHADER);
	GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(v,1,&vert_src,NULL);
	glShaderSource(f,1,&frag_src,NULL);
	glCompileShader(v);
	check_shader(name,"vertex",v);
	glCompileShader(f);
	check_shader(name,"fragment",f);
	GLuint p = glCreateProgram();
	glAttachShader(p,v);
	glAttachShader(p,f);
	glLinkProgram(p);
	check_program(name,"link",p,GL_LINK_STATUS);
	glDeleteShader(v);
	glDeleteShader(f);
	return p;
}
#else
;
#endif

typedef struct {
	float x,y,z, u,v;
	uint32_t color;
} TextureColorVertex;

typedef struct {
	size_t total, used;
	TextureColorVertex *elements;
} TextureColorVertexList;

TextureColorVertex *TextureColorVertexListMakeRoom(TextureColorVertexList *list, size_t count)
#if INCLUDED == 0
{
	if (list->used+count > list->total){
		if (!list->total) list->total = 1;
		while (list->used+count > list->total) list->total *= 2;
		list->elements = realloc_or_die(list->elements,list->total*sizeof(*list->elements));
	}
	list->used += count;
	return list->elements+list->used-count;
}
#else
;
#endif

struct {
	char *vert_src;
	char *frag_src;
	GLuint id;
	GLint aPosition;
	GLint aTexCoord;
	GLint aColor;
	GLint uMVP;
	GLint uTex;
} texture_color_shader
#if INCLUDED == 0
 = {
	"#version 110\n"
	"attribute vec3 aPosition;\n"
	"attribute vec2 aTexCoord;\n"
	"attribute vec4 aColor;\n"
	"uniform mat4 uMVP;\n"
	"varying vec2 vTexCoord;\n"
	"varying vec4 vColor;\n"
	"void main(){\n"
	"	gl_Position = uMVP * vec4(aPosition,1.0);\n"
	"	vTexCoord = aTexCoord;\n"
	"	vColor = aColor;\n"
	"}",

	"#version 110\n"
	"uniform sampler2D uTex;\n"
	"varying vec2 vTexCoord;\n"
	"varying vec4 vColor;\n"
	"void main(){\n"
	"	gl_FragColor = texture2D(uTex,vTexCoord) * vColor;\n"
	"}"
};
#else
;
#endif

#define COMPILE_SHADER(s) s.id = compile_shader(#s,s.vert_src,s.frag_src)
#define GET_ATTRIB(s,a) s.a = glGetAttribLocation(s.id,#a)
#define GET_UNIFORM(s,u) s.u = glGetUniformLocation(s.id,#u)

void compile_texture_color_shader()
#if INCLUDED == 0
{
	COMPILE_SHADER(texture_color_shader);
	GET_ATTRIB(texture_color_shader,aPosition);
	GET_ATTRIB(texture_color_shader,aTexCoord);
	GET_ATTRIB(texture_color_shader,aColor);
	GET_UNIFORM(texture_color_shader,uMVP);
	GET_UNIFORM(texture_color_shader,uTex);
}
#else
;
#endif

void texture_color_shader_prep_buffer()
#if INCLUDED == 0
{
	glEnableVertexAttribArray(texture_color_shader.aPosition);
	glEnableVertexAttribArray(texture_color_shader.aTexCoord);
	glEnableVertexAttribArray(texture_color_shader.aColor);
	glVertexAttribPointer(texture_color_shader.aPosition,3,GL_FLOAT,GL_FALSE,sizeof(TextureColorVertex),0);
	glVertexAttribPointer(texture_color_shader.aTexCoord,2,GL_FLOAT,GL_FALSE,sizeof(TextureColorVertex),offsetof(TextureColorVertex,u));
	glVertexAttribPointer(texture_color_shader.aColor,4,GL_UNSIGNED_BYTE,GL_TRUE,sizeof(TextureColorVertex),offsetof(TextureColorVertex,color));
}
#else
;
#endif

void compile_shaders()
#if INCLUDED == 0
{
	compile_texture_color_shader();
}
#else
;
#endif