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
World world;
int chunk_radius = 4;

void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

int cam_dir[3];
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	switch (action){
		case GLFW_PRESS:{
			switch (key){
				case GLFW_KEY_ESCAPE:{
					glfwSetWindowShouldClose(window, GLFW_TRUE);
					break;
				}
				case GLFW_KEY_A: if (!cam_dir[0]) cam_dir[0] = -1; else if (cam_dir[0] == 1) cam_dir[0] = 2; break;
				case GLFW_KEY_D: if (!cam_dir[0]) cam_dir[0] = 1; else if (cam_dir[0] == -1) cam_dir[0] = -2; break;
				case GLFW_KEY_S: if (!cam_dir[2]) cam_dir[2] = -1; else if (cam_dir[2] == 1) cam_dir[2] = 2; break;
				case GLFW_KEY_W: if (!cam_dir[2]) cam_dir[2] = 1; else if (cam_dir[2] == -1) cam_dir[2] = -2; break;
			}
			break;
		}
		case GLFW_RELEASE:{
			switch (key){
				case GLFW_KEY_A: if (cam_dir[0] == -1) cam_dir[0] = 0; else if (cam_dir[0] == -2) cam_dir[0] = 1; else if (cam_dir[0] == 2) cam_dir[0] = 1; break;
				case GLFW_KEY_D: if (cam_dir[0] == 1) cam_dir[0] = 0; else if (cam_dir[0] == 2) cam_dir[0] = -1; else if (cam_dir[0] == -2) cam_dir[0] = -1; break;
				case GLFW_KEY_S: if (cam_dir[2] == -1) cam_dir[2] = 0; else if (cam_dir[2] == -2) cam_dir[2] = 1; else if (cam_dir[2] == 2) cam_dir[2] = 1; break;
				case GLFW_KEY_W: if (cam_dir[2] == 1) cam_dir[2] = 0; else if (cam_dir[2] == 2) cam_dir[2] = -1; else if (cam_dir[2] == -2) cam_dir[2] = -1; break;
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

GLFWwindow *create_centered_window(int width, int height, char *title){
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	GLFWwindow *window = glfwCreateWindow(width,height,title,NULL,NULL);
	if (!window){
		glfwTerminate();
		fatal_error("Failed to create GLFW window");
	}
	GLFWmonitor *primary = glfwGetPrimaryMonitor();
	if (!primary){
		glfwTerminate();
		fatal_error("Failed to get primary monitor");
	}
	GLFWvidmode *vm = glfwGetVideoMode(primary);
	if (!vm){
		glfwTerminate();
		fatal_error("Failed to get primary monitor video mode");
	}
	glfwSetWindowPos(window,(vm->width-width)/2,(vm->height-height)/2);
	glfwShowWindow(window);
	return window;
}

void world_pos_to_chunk_pos(vec3 world_pos, ivec2 chunk_pos){
	int i = floorf(world_pos[0]);
	int k = floorf(world_pos[2]);
	chunk_pos[0] = (i < 0 ? -1 : 0) + i/CHUNK_WIDTH;
	chunk_pos[1] = (k < 0 ? -1 : 0) + k/CHUNK_WIDTH;
}

int manhattan_distance_2d(ivec2 a, ivec2 b){
	return abs(a[0]-b[0]) + abs(a[1]-b[1]);
}

typedef struct {
	ivec2 initial_pos;
	ivec2 cur_pos;
	ivec2 c;
	int steps;
	int radius;
} ManhattanSpiralGenerator;

void manhattan_spiral_generator_init(ManhattanSpiralGenerator *g, ivec2 initial_pos){
	g->initial_pos[0] = initial_pos[0];
	g->initial_pos[1] = initial_pos[1];
	g->cur_pos[0] = initial_pos[0];
	g->cur_pos[1] = initial_pos[1];
	g->steps = 0;
	g->radius = 0;
}

void manhattan_spiral_generator_get_next_position(ManhattanSpiralGenerator *g){
	if (g->steps == 4*g->radius){
		g->cur_pos[1] = g->initial_pos[1];
		g->radius += 1;
		g->cur_pos[0] = g->initial_pos[0] + g->radius;
		g->steps = 1;
		g->c[0] = 1;
		g->c[1] = 1;
	} else {
		if (g->cur_pos[0] == g->initial_pos[0]){
			g->c[1] = -g->c[1];
		} else if (g->cur_pos[1] == g->initial_pos[1]){
			g->c[0] = -g->c[0];
		}
		g->cur_pos[0] += g->c[0];
		g->cur_pos[1] += g->c[1];
		g->steps++;
	}
}
 
void main(void)
{
	glfwSetErrorCallback(error_callback);
 
	if (!glfwInit()){
		fatal_error("Failed to initialize GLFW");
	}
 
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	GLFWwindow *window = create_centered_window(640,480,"Blocks");
 
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
	srand(time(0));

	double t0 = glfwGetTime();
 
	while (!glfwWindowShouldClose(window))
	{
		double t1 = glfwGetTime();
		double dt = t1 - t0;
		t0 = t1;

		//move camera based on cam_dir
		mat4 crot;
		glm_euler_zyx(camera.euler,crot);
		vec3 forward, right;
		glm_vec3(crot[2],forward);
		glm_vec3(crot[0],right);
		glm_vec3_scale(forward,glm_sign(-cam_dir[2])*20.0f*dt,forward);
		glm_vec3_scale(right,glm_sign(cam_dir[0])*20.0f*dt,right);
		glm_vec3_add(camera.position,forward,camera.position);
		glm_vec3_add(camera.position,right,camera.position);

		ivec2 chunk_pos;
		world_pos_to_chunk_pos(camera.position,chunk_pos);
		ManhattanSpiralGenerator msg;
		manhattan_spiral_generator_init(&msg,chunk_pos);
		while (msg.radius <= chunk_radius){
			Chunk *c = ChunkFixedKeyLinkedHashListGet(&world.chunks,sizeof(msg.cur_pos),msg.cur_pos);
			if (!c){
				//find out of range chunk, replace with new chunk
				//if none found, add new chunk to hashlist
				ChunkFixedKeyLinkedHashListBucket *b = world.chunks.first;
				while (b && manhattan_distance_2d(b->key,chunk_pos) <= chunk_radius){
					b = b->next;
				}
				if (b){
					ChunkFixedKeyLinkedHashListRemove(&world.chunks,b);
				}
				Chunk *new_chunk = ChunkFixedKeyLinkedHashListNew(&world.chunks,sizeof(msg.cur_pos),msg.cur_pos);
				gen_chunk(new_chunk);
				mesh_chunk(new_chunk);
			}
			manhattan_spiral_generator_get_next_position(&msg);
		}
		printf("chunk_count: %zu\n",world.chunks.used);

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
		for (ChunkFixedKeyLinkedHashListBucket *b = world.chunks.first; b; b = b->next){
			mat4 inv_crot;
			glm_mat4_transpose_to(crot,inv_crot);
			mat4 view;
			vec3 trans = {
				((int *)b->key)[0] * 16,
				0,
				((int *)b->key)[1] * 16
			};
			glm_vec3_sub(trans,camera.position,trans);
			glm_translate_make(view,trans);
			glm_mat4_mul(inv_crot,view,view);
			mat4 persp;
			glm_perspective(camera.fov_radians,(float)width/(float)height,0.01f,1000.0f,persp);
			mat4 mvp;
			glm_mat4_mul(persp,view,mvp);
			glUniformMatrix4fv(texture_color_shader.uMVP,1,GL_FALSE,mvp);
			glBindBuffer(GL_ARRAY_BUFFER,b->value.vbo_id);
			texture_color_shader_prep_buffer();
			glDrawArrays(GL_TRIANGLES,0,b->value.vertex_count);
		}

		glCheckError();
 
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
 
	glfwDestroyWindow(window);
 
	glfwTerminate();
}