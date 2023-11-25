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
#include <tinydir.h>

#include "world.c"
#include "camera.c"

#pragma pop_macro("INCLUDED")

Camera camera = {
	.position = {17,17,17},
	.euler = {0,0,0},
	.fov_radians = 0.5f*M_PI
};
Texture block_atlas;
Chunk chunk;

void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}
 
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
}
 
void main(void)
{
	GLFWwindow* window;
 
	glfwSetErrorCallback(error_callback);
 
	if (!glfwInit()){
		fatal_error("Failed to initialize GLFW");
	}
 
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
 
	window = glfwCreateWindow(640, 480, "Blocks", NULL, NULL);
	if (!window){
		glfwTerminate();
		fatal_error("Failed to create GLFW window");
	}
 
	glfwSetKeyCallback(window, key_callback);
 
	glfwMakeContextCurrent(window);
	gladLoadGL();
	glfwSwapInterval(1);

	compile_shaders();

	texture_from_file(&block_atlas,"../res/blocks.png");
	chunk.vbo_id = 0;
	gen_chunk(&chunk);
	mesh_chunk(&chunk);
 
	while (!glfwWindowShouldClose(window))
	{
		int width,height;
		glfwGetFramebufferSize(window, &width, &height);
 
		glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

		/*TextureColorVertex screen_quad[] = {
			{0,1,0, 0,1, 0xffffffff},
			{0,0,0, 0,0, 0xffffffff},
			{1,0,0, 1,0, 0xffffffff},

			{1,0,0, 1,0, 0xffffffff},
			{1,1,0, 1,1, 0xffffffff},
			{0,1,0, 0,1, 0xffffffff}
		};
		GLuint buf;
		glGenBuffers(1,&buf);
		glBindBuffer(GL_ARRAY_BUFFER,buf);
		glBufferData(GL_ARRAY_BUFFER,sizeof(screen_quad),screen_quad,GL_STATIC_DRAW);*/

		glUseProgram(texture_color_shader.id);
		glUniform1i(texture_color_shader.uTex,0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D,block_atlas.id);
		glUniformMatrix4fv(texture_color_shader.uModel,1,GL_FALSE,GLM_MAT4_IDENTITY);
		mat4 view;
		glm_lookat(camera.position,(vec3){0,0,0},GLM_YUP,view);
		glUniformMatrix4fv(texture_color_shader.uView,1,GL_FALSE,view);
		mat4 proj;
		glm_perspective(camera.fov_radians,(float)width/(float)height,0.01f,1000.0f,proj);
		glUniformMatrix4fv(texture_color_shader.uProj,1,GL_FALSE,proj);
		glBindBuffer(GL_ARRAY_BUFFER,chunk.vbo_id);
		texture_color_shader_prep_buffer();
		//glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
		glDrawArrays(GL_TRIANGLES,0,chunk.vertex_count);
		//glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

		glCheckError();
 
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
 
	glfwDestroyWindow(window);
 
	glfwTerminate();
}