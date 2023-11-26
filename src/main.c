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
	.position = {17,34,17},
	.euler = {0,0,0},
	.fov_radians = 0.5f*M_PI
};
Texture block_atlas;
Chunk chunk;

void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

int camDir[3];
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	switch (action){
		case GLFW_PRESS:{
			switch (key){
				case GLFW_KEY_ESCAPE:{
					glfwSetWindowShouldClose(window, GLFW_TRUE);
					break;
				}
				case GLFW_KEY_A: if (!camDir[0]) camDir[0] = -1; else if (camDir[0] == 1) camDir[0] = 2; break;
				case GLFW_KEY_D: if (!camDir[0]) camDir[0] = 1; else if (camDir[0] == -1) camDir[0] = -2; break;
				case GLFW_KEY_S: if (!camDir[2]) camDir[2] = -1; else if (camDir[2] == 1) camDir[2] = 2; break;
				case GLFW_KEY_W: if (!camDir[2]) camDir[2] = 1; else if (camDir[2] == -1) camDir[2] = -2; break;
			}
			break;
		}
		case GLFW_RELEASE:{
			switch (key){
				case GLFW_KEY_A: if (camDir[0] == -1) camDir[0] = 0; else if (camDir[0] == -2) camDir[0] = 1; else if (camDir[0] == 2) camDir[0] = 1; break;
				case GLFW_KEY_D: if (camDir[0] == 1) camDir[0] = 0; else if (camDir[0] == 2) camDir[0] = -1; else if (camDir[0] == -2) camDir[0] = -1; break;
				case GLFW_KEY_S: if (camDir[2] == -1) camDir[2] = 0; else if (camDir[2] == -2) camDir[2] = 1; else if (camDir[2] == 2) camDir[2] = 1; break;
				case GLFW_KEY_W: if (camDir[2] == 1) camDir[2] = 0; else if (camDir[2] == 2) camDir[2] = -1; else if (camDir[2] == -2) camDir[2] = -1; break;
			}
			break;
		}
	}
}
void clamp_euler(vec3 e){
	float fp = 4*M_PI;
	for (int i = 0; i < 3; i++){
		if (e[i] > fp) e[i] -= fp;
		else if (e[i] < -fp) e[i] += fp;
	}
}
void rotate_camera(Camera *c, float dx, float dy, float sens){
	c->euler[1] += sens * dx;
	c->euler[0] += sens * dy;
	clamp_euler(c->euler);
}
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos){
	rotate_camera(&camera,xpos,ypos,-0.001f);
	glfwSetCursorPos(window, 0, 0);
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
 
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	if (glfwRawMouseMotionSupported()){
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}
	glfwSetCursorPos(window, 0, 0);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetKeyCallback(window, key_callback);
 
	glfwMakeContextCurrent(window);
	gladLoadGL();
	glfwSwapInterval(1);

	compile_shaders();

	texture_from_file(&block_atlas,"../res/blocks.png");
	chunk.vbo_id = 0;
	gen_chunk(&chunk);
	mesh_chunk(&chunk);

	double t0 = glfwGetTime();
 
	while (!glfwWindowShouldClose(window))
	{
		double t1 = glfwGetTime();
		double dt = t1 - t0;
		t0 = t1;

		//move camera based on camDir
		mat4 crot;
		glm_euler_zyx(camera.euler,crot);
		vec3 forward, right;
		glm_vec3(crot[2],forward);
		glm_vec3(crot[0],right);
		glm_vec3_scale(forward,glm_sign(-camDir[2])*2.0f*dt,forward);
		glm_vec3_scale(right,glm_sign(camDir[0])*2.0f*dt,right);
		glm_vec3_add(camera.position,forward,camera.position);
		glm_vec3_add(camera.position,right,camera.position);

		int width,height;
		glfwGetFramebufferSize(window, &width, &height);
 
		glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

		glUseProgram(texture_color_shader.id);
		glUniform1i(texture_color_shader.uTex,0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D,block_atlas.id);
		glm_mat4_transpose(crot);
		mat4 view;
		glm_translate_make(view,(vec3){-camera.position[0],-camera.position[1],-camera.position[2]});
		glm_mat4_mul(crot,view,view);
		mat4 persp;
		glm_perspective(camera.fov_radians,(float)width/(float)height,0.01f,1000.0f,persp);
		mat4 mvp;
		glm_mat4_mul(persp,view,mvp);
		glUniformMatrix4fv(texture_color_shader.uMVP,1,GL_FALSE,mvp);
		glBindBuffer(GL_ARRAY_BUFFER,chunk.vbo_id);
		texture_color_shader_prep_buffer();
		glDrawArrays(GL_TRIANGLES,0,chunk.vertex_count);

		glCheckError();
 
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
 
	glfwDestroyWindow(window);
 
	glfwTerminate();
}