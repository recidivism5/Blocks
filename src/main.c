#include <player.h>

Player player;

enum MoveState {
	MOVE_STATE_NORMAL,
	MOVE_STATE_FLY,
	MOVE_STATE_NOCLIP,
	MOVE_STATE_COUNT
} move_state = MOVE_STATE_NORMAL;

Texture block_atlas;
World world;
int chunk_radius = 16;

void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

struct {
	bool
		left,
		right,
		backward,
		forward,
		jump,
		crouch,
		attack,
		just_attacked,
		interact,
		just_interacted;
} keys;
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
	switch (action){
		case GLFW_PRESS:{
			switch (key){
				case GLFW_KEY_ESCAPE:{
					glfwSetWindowShouldClose(window, GLFW_TRUE);
					break;
				}
				case GLFW_KEY_A: keys.left = true; break;
				case GLFW_KEY_D: keys.right = true; break;
				case GLFW_KEY_S: keys.backward = true; break;
				case GLFW_KEY_W: keys.forward = true; break;
				case GLFW_KEY_SPACE: keys.jump = true; break;
				case GLFW_KEY_LEFT_CONTROL: keys.crouch = true; break;
				case GLFW_KEY_F:{
					move_state = (move_state + 1) % MOVE_STATE_COUNT;
					break;
				}
				case GLFW_KEY_R:{
					static int prev_width, prev_height;
					static bool fullscreen = false;
					GLFWmonitor *primary = glfwGetPrimaryMonitor();
					GLFWvidmode *vm = glfwGetVideoMode(primary);
					if (!fullscreen){
						glfwGetFramebufferSize(window,&prev_width,&prev_height);
						glfwSetWindowMonitor(window,primary,0,0,vm->width,vm->height,GLFW_DONT_CARE);
						fullscreen = true;
					} else {
						glfwSetWindowMonitor(window,0,(vm->width-prev_width)/2,(vm->height-prev_height)/2,prev_width,prev_height,GLFW_DONT_CARE);
						fullscreen = false;
					}
					break;
				}
			}
			break;
		}
		case GLFW_RELEASE:{
			switch (key){
				case GLFW_KEY_A: keys.left = false; break;
				case GLFW_KEY_D: keys.right = false; break;
				case GLFW_KEY_S: keys.backward = false; break;
				case GLFW_KEY_W: keys.forward = false; break;
				case GLFW_KEY_SPACE: keys.jump = false; break;
				case GLFW_KEY_LEFT_CONTROL: keys.crouch = false; break;
			}
			break;
		}
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods){
	switch (action){
		case GLFW_PRESS:{
			switch (button){
				case GLFW_MOUSE_BUTTON_LEFT: keys.attack = true; keys.just_attacked = true; break;
				case GLFW_MOUSE_BUTTON_RIGHT: keys.interact = true; keys.just_interacted = true; break;
			}
			break;
		}
		case GLFW_RELEASE:{
			switch (button){
				case GLFW_MOUSE_BUTTON_LEFT: keys.attack = false; break;
				case GLFW_MOUSE_BUTTON_RIGHT: keys.interact = false; keys.just_interacted = false; break;
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
void rotate_euler(vec3 e, float dx, float dy, float sens){
	e[1] += sens * dx;
	e[0] += sens * dy;
	clamp_euler(e);
}
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos){
	rotate_euler(player.head_euler,xpos,ypos,-0.001f);
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

int manhattan_distance_2d(ivec2 a, ivec2 b){
	return abs(a[0]-b[0]) + abs(a[1]-b[1]);
}

TSTRUCT(ManhattanSpiralGenerator){
	ivec2 initial_pos;
	ivec2 cur_pos;
	ivec2 c;
	int steps;
	int radius;
};

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

TSTRUCT(ChunkLinkedHashListBucketDistance){
	ChunkLinkedHashListBucket *bucket;
	int distance;
};

int compare_chunk_linked_hash_list_bucket_distance(ChunkLinkedHashListBucketDistance *a, ChunkLinkedHashListBucketDistance *b){
	return b->distance - a->distance;
}

vec3 player_head;
ivec2 chunk_pos_for_triangle_sort;
int compare_triangle_distance_from_player_head(TextureColorVertex *a, TextureColorVertex *b){
	vec3 cpos = {chunk_pos_for_triangle_sort[0]*CHUNK_WIDTH,0,chunk_pos_for_triangle_sort[1]*CHUNK_WIDTH};
	vec3 a_midpoint;
	glm_vec3_lerp(a[0].position,a[2].position,0.5f,a_midpoint);//midpoint of quad
	vec3 b_midpoint;
	glm_vec3_lerp(b[0].position,b[2].position,0.5f,b_midpoint);//midpoint of quad
	glm_vec3_add(cpos,a_midpoint,a_midpoint);
	glm_vec3_add(cpos,b_midpoint,b_midpoint);
	float ad = glm_vec3_distance2(player_head,a_midpoint);
	float bd = glm_vec3_distance2(player_head,b_midpoint);
	return COMPARE(bd,ad);
}

void main(void)
{
	glfwSetErrorCallback(error_callback);
 
	if (!glfwInit()){
		fatal_error("Failed to initialize GLFW");
	}
 
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	GLFWwindow *window = create_centered_window(640,480,"Blocks");
 
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	if (glfwRawMouseMotionSupported()){
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}
	glfwSetCursorPos(window, 0, 0);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetKeyCallback(window, key_callback);
 
	glfwMakeContextCurrent(window);
	gladLoadGL();
	glfwSwapInterval(1);

	compile_shaders();

	texture_from_file(&block_atlas,"../res/blocks.png");
	srand(time(0));

	init_player(&player,(vec3){8,CHUNK_HEIGHT,8},(vec3){0,0,0});

	init_block_outline();

	init_light_coefficients();

	double t0 = glfwGetTime();
 
	while (!glfwWindowShouldClose(window))
	{
		double t1 = glfwGetTime();
		double dt = t1 - t0;
		t0 = t1;

		int width,height;
		glfwGetFramebufferSize(window, &width, &height);

		mat4 crot;
		glm_euler_zyx(player.head_euler,crot);
		vec3 c_right,c_backward;
		glm_vec3(crot[0],c_right);
		glm_vec3(crot[2],c_backward);
		mat4 inv_crot;
		glm_mat4_transpose_to(crot,inv_crot);

		ivec2 move_dir;
		if (keys.left && keys.right){
			move_dir[0] = 0;
		} else if (keys.left){
			move_dir[0] = -1;
		} else if (keys.right){
			move_dir[0] = 1;
		} else {
			move_dir[0] = 0;
		}
		if (keys.backward && keys.forward){
			move_dir[1] = 0;
		} else if (keys.backward){
			move_dir[1] = 1;
		} else if (keys.forward){
			move_dir[1] = -1;
		} else {
			move_dir[1] = 0;
		}
		switch (move_state){
			case MOVE_STATE_NOCLIP:{
				glm_vec3_scale(c_right,move_dir[0],c_right);
				glm_vec3_scale(c_backward,move_dir[1],c_backward);
				vec3 move_vec;
				glm_vec3_add(c_right,c_backward,move_vec);
				glm_vec3_normalize(move_vec);
				glm_vec3_scale(move_vec,20.0f*dt,move_vec);
				glm_vec3_add(player.aabb.position,move_vec,player.aabb.position);
				break;
			}
			case MOVE_STATE_NORMAL:{
				vec3 move_vec = {move_dir[0],0,move_dir[1]};
				if (move_dir[0] || move_dir[1]){
					glm_vec3_normalize(move_vec);
					glm_vec3_scale(move_vec,8.0f,move_vec);
					glm_vec3_rotate(move_vec,player.head_euler[1],GLM_YUP);
				}
				player.aabb.velocity[0] = glm_lerp(player.aabb.velocity[0],move_vec[0],8.0f*dt);
				player.aabb.velocity[2] = glm_lerp(player.aabb.velocity[2],move_vec[2],8.0f*dt);
				if (player.aabb.on_ground && keys.jump){
					player.aabb.velocity[1] = 10.0f;
				}
				player.aabb.velocity[1] -= 32.0f * dt;
				move_aabb_against_chunks(&world.chunks,&player.aabb,dt);
				break;
			}
			case MOVE_STATE_FLY:{
				vec3 move_vec = {move_dir[0],keys.crouch ? -1 : keys.jump ? 1 : 0,move_dir[1]};
				if (move_dir[0] || move_dir[1] || keys.crouch || keys.jump){
					glm_vec3_normalize(move_vec);
					glm_vec3_scale(move_vec,16.0f,move_vec);
					glm_vec3_rotate(move_vec,player.head_euler[1],GLM_YUP);
				}
				glm_vec3_lerp(player.aabb.velocity,move_vec,8.0f*dt,player.aabb.velocity);
				move_aabb_against_chunks(&world.chunks,&player.aabb,dt);
				break;
			}
		}

		get_player_head_position(&player,player_head);
		mat4 persp;
		glm_perspective(0.5f*M_PI,(float)width/(float)height,0.01f,1000.0f,persp);
		vec3 player_look;
		glm_vec3_negate_to(c_backward,player_look);
		glm_vec3_scale(player_look,5.0f,player_look);
		BlockRayCastResult rcr;
		cast_ray_into_blocks(&world.chunks,player_head,player_look,&rcr);
		if (rcr.block){
			if (keys.just_attacked){
				set_block(&world.chunks,rcr.block_pos,BLOCK_AIR);
				cast_ray_into_blocks(&world.chunks,player_head,player_look,&rcr);
				keys.just_attacked = false;
			} else if (keys.just_interacted){
				ivec3 new_block_pos;
				glm_ivec3_add(rcr.block_pos,rcr.face_normal,new_block_pos);
				set_block(&world.chunks,new_block_pos,BLOCK_RED_GLASS);
				cast_ray_into_blocks(&world.chunks,player_head,player_look,&rcr);
				keys.just_interacted = false;
			}
		}

		ivec2 chunk_pos;
		world_pos_to_chunk_pos(player.aabb.position,chunk_pos);
		ManhattanSpiralGenerator msg;
		manhattan_spiral_generator_init(&msg,chunk_pos);
		int new_chunks = 0;
		int max_new_chunks_per_frame = 1;
		static bool chunk_benchmark_complete = false;
		double start = glfwGetTime();
		while (msg.radius <= chunk_radius){
			ChunkLinkedHashListBucket *b = ChunkLinkedHashListGetChecked(&world.chunks,msg.cur_pos);
			if (!b){
				b = ChunkLinkedHashListNew(&world.chunks,msg.cur_pos);
				ChunkLinkedHashListBucket *furthest = 0;
				int dist_to_furthest = 0;
				for (ChunkLinkedHashListBucket *oldb = world.chunks.first; oldb; oldb = oldb->next){
					int md = manhattan_distance_2d(oldb->position,chunk_pos);
					if (md > dist_to_furthest){
						furthest = oldb;
						dist_to_furthest = md;
					}
				}
				if (furthest && dist_to_furthest > chunk_radius){
					b->chunk = furthest->chunk;
					ChunkLinkedHashListRemove(&world.chunks,furthest);
					memset(b->chunk,0,sizeof(Chunk));
				} else {
					b->chunk = zalloc_or_die(sizeof(Chunk));
				}
				gen_chunk(b);
				light_chunk(&world.chunks,b);
				if (chunk_benchmark_complete){
					new_chunks++;
					if (new_chunks >= max_new_chunks_per_frame) break;
				}
			}
			manhattan_spiral_generator_get_next_position(&msg);
		}
		double end = glfwGetTime();
		if (!chunk_benchmark_complete) printf("time to mesh all chunks: %f nanoseconds\n",(end-start)*1000000000.0);
		chunk_benchmark_complete = true;
 
		glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

		if (world.chunks.used){
			glUseProgram(texture_color_shader.id);
			glUniform1i(texture_color_shader.uTex,0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D,block_atlas.id);
			ChunkLinkedHashListBucketDistance *sorted_chunk_buckets = malloc_or_die(world.chunks.used * sizeof(*sorted_chunk_buckets));
			int i = 0;
			for (ChunkLinkedHashListBucket *b = world.chunks.first; b; b = b->next){
				sorted_chunk_buckets[i].bucket = b;
				sorted_chunk_buckets[i].distance = manhattan_distance_2d(b->position,chunk_pos);
				i++;
			}
			qsort(sorted_chunk_buckets,world.chunks.used,sizeof(*sorted_chunk_buckets),compare_chunk_linked_hash_list_bucket_distance);
			for (i = 0; i < world.chunks.used; i++){
				ChunkLinkedHashListBucket *b = sorted_chunk_buckets[i].bucket;
				if (b->chunk->opaque_verts.vertex_count || b->chunk->transparent_verts.used){
					mat4 view;
					vec3 trans = {
						b->position[0] * CHUNK_WIDTH,
						0,
						b->position[1] * CHUNK_WIDTH
					};
					glm_vec3_sub(trans,player_head,trans);
					glm_translate_make(view,trans);
					glm_mat4_mul(inv_crot,view,view);
					mat4 mvp;
					glm_mat4_mul(persp,view,mvp);
					glUniformMatrix4fv(texture_color_shader.uMVP,1,GL_FALSE,mvp);

					if (b->chunk->opaque_verts.vertex_count){
						glBindVertexArray(b->chunk->opaque_verts.vao);
						glDrawArrays(GL_TRIANGLES,0,b->chunk->opaque_verts.vertex_count);
					}

					if (b->chunk->transparent_verts.used){
						//sort transparent triangles by distance from player_head
						//upload to gpu using gpu_mesh_from_texture_color_verts
						//render
						//delete gpu mesh
						glm_ivec2_copy(b->position,chunk_pos_for_triangle_sort);
						qsort(b->chunk->transparent_verts.elements,b->chunk->transparent_verts.used/3,3*sizeof(*b->chunk->transparent_verts.elements),compare_triangle_distance_from_player_head);
						GPUMesh m;
						gpu_mesh_from_texture_color_verts(&m,b->chunk->transparent_verts.elements,b->chunk->transparent_verts.used);

						glBindVertexArray(m.vao);
						glDrawArrays(GL_TRIANGLES,0,m.vertex_count);

						delete_gpu_mesh(&m);
					}
				}
			}
			free(sorted_chunk_buckets);
		}

		if (rcr.block){
			glUseProgram(color_shader.id);
			mat4 mvp;
			float padding = 0.0125f;
			vec3 trans = {
				rcr.block_pos[0]-player_head[0]-padding/2,
				rcr.block_pos[1]-player_head[1]-padding/2,
				rcr.block_pos[2]-player_head[2]-padding/2,
			};
			glm_translate_make(mvp,trans);
			mvp[0][0] = 1.0f + padding;
			mvp[1][1] = 1.0f + padding;
			mvp[2][2] = 1.0f + padding;
			glm_mat4_mul(inv_crot,mvp,mvp);
			glm_mat4_mul(persp,mvp,mvp);
			glUniformMatrix4fv(color_shader.uMVP,1,GL_FALSE,mvp);
			glBindVertexArray(block_outline.vao);
			glDrawArrays(GL_LINES,0,block_outline.vertex_count);
		}

		glCheckError();
 
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
 
	glfwDestroyWindow(window);
 
	glfwTerminate();
}