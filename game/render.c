#include <stdio.h>
#include "game.h"
#include "render.h"
#include "asset.h"

static void
debug_texture(vec2 size, struct texture *tex)
{
	float w = size.x / (float)g_state->width;
	float h = size.y / (float)g_state->height;
	vec2 at = {-1, 1}; /* top - left */

	sys_render_push(&(struct render_entry){
			.shader = DEBUG_SHADER_TEXTURE,
			.mesh = DEBUG_MESH_CUBE,
			.scale = {w, h, 0},
			.position = {at.x + w, at.y - h, -1},
			.rotation = QUATERNION_IDENTITY,
			.texture = {
				{ "tex", INTERNAL_TEXTURE, .tex = tex }
			},
		});
}


void
sys_render_push(struct render_entry *entry)
{
	struct system *sys = &g_state->sys_render;
	struct render_entry *e;

	e = list_push(&sys->list, &sys->zone, sizeof(*e));
	*e = *entry;
}

void
sys_render_push_vec(vec3 pos, vec3 dir, vec3 color)
{
	vec3 x = vec3_add(pos, vec3_mult(1, dir));

	sys_render_push(&(struct render_entry){
			.shader = SHADER_SOLID,
			.mesh = DEBUG_MESH_CROSS,
			.scale = {0,0,1},
			.position = x,
			.rotation = quaternion_look_at(dir, (vec3){0,1,0}),
			.color = color,
		});
}

void
sys_render_push_bounding(vec3 pos, vec3 scale, struct bounding_volume *bvol)
{
	float radius = vec3_max(scale) * bvol->radius;

	sys_render_push(&(struct render_entry){
			.shader = SHADER_SOLID,
			.mesh = DEBUG_MESH_SPHERE,
			.scale = vec3_mult(radius, scale),
			.position = vec3_add(pos, bvol->off),
			.rotation = QUATERNION_IDENTITY,
			.color = {1,1,0},
		});
}

void
sys_render_push_cross(vec3 at, vec3 scale, vec3 color)
{
	sys_render_push(&(struct render_entry){
			.shader = SHADER_SOLID,
			.mesh = DEBUG_MESH_CROSS,
			.scale = scale,
			.position = at,
			.rotation = QUATERNION_IDENTITY,
			.color = color,
		});
}

void
render_bind_shader(struct shader *shader)
{
	/* Set the current shader program to shader->prog */
	glUseProgram(shader->prog);
}

void
render_bind_mesh(struct shader *shader, struct mesh *mesh)
{
	GLint position;
	GLint normal;
	GLint texcoord;

	position = glGetAttribLocation(shader->prog, "in_pos");
	normal = glGetAttribLocation(shader->prog, "in_normal");
	texcoord = glGetAttribLocation(shader->prog, "in_texcoord");

	mesh_bind(mesh, position, normal, texcoord);
}

void
render_bind_camera(struct shader *s, struct camera *c)
{
	GLint proj, view;

	proj = glGetUniformLocation(s->prog, "proj");
	view = glGetUniformLocation(s->prog, "view");

	glUniformMatrix4fv(proj, 1, GL_FALSE, (float *)&c->proj.m);
	glUniformMatrix4fv(view, 1, GL_FALSE, (float *)&c->view.m);
}

void
render_mesh(struct mesh *mesh)
{
	if (mesh->index_count > 0)
		glDrawElements(mesh->primitive, mesh->index_count, GL_UNSIGNED_INT, 0);
	else
		glDrawArrays(mesh->primitive, 0, mesh->vertex_count);
}

static int
frustum_cull(vec4 frustum[6], struct mesh *mesh, vec3 pos, vec3 scale)
{
	vec3 center = mesh->bounding.off;
	float radius = vec3_max(scale) * mesh->bounding.radius;

	/* move the sphere center to the object position */
	pos = vec3_fma(scale, center, pos);

	return sphere_outside_frustum(frustum, pos, radius);
}

static void
render_pass(struct camera cam, int do_frustum_cull)
{
	struct system *sys = &g_state->sys_render;
	struct game_state *game_state = g_state;
	struct game_asset *game_asset = g_asset;
	struct camera *sun = &game_state->sun;
	struct light *light = &game_state->light;
	enum asset_key last_shader = ASSET_KEY_COUNT;
	enum asset_key last_mesh = ASSET_KEY_COUNT;
	struct shader *shader = NULL;
	struct mesh *mesh = NULL;
	mat4 vm = mat4_mult_mat4(&cam.proj, &cam.view);
	vec4 frustum[6];
	struct link *link;

	mat4_projection_frustum(&vm, frustum);
	for (link = sys->list.first; link != NULL; link = link->next) {
		struct render_entry e = *(struct render_entry *)link->data;

		if (!mesh || last_mesh != e.mesh)
			mesh = game_get_mesh(game_asset, e.mesh);

		if (do_frustum_cull && e.cull && frustum_cull(frustum, mesh, e.position, e.scale))
			continue;

		if (!shader || last_shader != e.shader) {
			last_shader = e.shader;
			shader = game_get_shader(game_asset, e.shader);
			render_bind_shader(shader);
			render_bind_camera(shader, &cam);
			/* mesh need to be bind again */
			last_mesh = e.mesh;
			render_bind_mesh(shader, mesh);
		} else if (last_mesh != e.mesh) {
			last_mesh = e.mesh;
			render_bind_mesh(shader, mesh);
		}

		mat4 transform = mat4_transform_scale(e.position,
						      e.rotation,
						      e.scale);

		GLint model = glGetUniformLocation(shader->prog, "model");
		if (model >= 0)
			glUniformMatrix4fv(model, 1, GL_FALSE, (float *)&transform.m);

		GLint time = glGetUniformLocation(shader->prog, "time");
		if (time >= 0)
			glUniform1f(time, g_input->time);

		GLint camp = glGetUniformLocation(shader->prog, "camp");
		if (camp >= 0)
			glUniform3f(camp, cam.position.x, cam.position.y, cam.position.z);

		GLint lightd = glGetUniformLocation(shader->prog, "lightd");
		if (lightd >= 0)
			glUniform3f(lightd,
				    light->dir.x,
				    light->dir.y,
				    light->dir.z);

		GLint lightc = glGetUniformLocation(shader->prog, "lightc");
		if (lightc >= 0)
			glUniform4f(lightc,
				    light->col.x,
				    light->col.y,
				    light->col.z,
				    light->col.w
				);
		GLint lsview = glGetUniformLocation(shader->prog, "lsview");
		if (lsview >= 0)
			glUniformMatrix4fv(lsview, 1, GL_FALSE, (float *) sun->view.m);

		GLint lsproj = glGetUniformLocation(shader->prog, "lsproj");
		if (lsproj >= 0)
			glUniformMatrix4fv(lsproj, 1, GL_FALSE, (float *) sun->proj.m);

		GLint color = glGetUniformLocation(shader->prog, "color");
		if (color >= 0)
			glUniform3f(color, e.color.x, e.color.y, e.color.z);

		GLint v2res = glGetUniformLocation(shader->prog, "v2Resolution");
		if (v2res >= 0)
			glUniform2f(v2res, game_state->width, game_state->height);

		GLint thick = glGetUniformLocation(shader->prog, "thickness");
		if (thick >= 0)
			glUniform1f(v2res, 0.93);

		int unit;
		for (unit = 0; unit < RENDER_MAX_TEXTURE_UNIT; unit++) {
			if (e.texture[unit].name == NULL)
				break;
			const char *name = e.texture[unit].name;
			enum asset_key id = e.texture[unit].res_id;
			GLint tex_loc = glGetUniformLocation(shader->prog, name);
			if (tex_loc) {
				struct texture *tex_res = e.texture[unit].tex;
				if (id < ASSET_KEY_COUNT)
					tex_res = game_get_texture(game_asset, id);

				glActiveTexture(GL_TEXTURE0 + unit);
				glUniform1i(tex_loc, unit);
				glBindTexture(tex_res->type, tex_res->id);
			}
		}

		if (e.count == 0) {
//			glDisable(GL_CULL_FACE);
			render_mesh(mesh);
		} else {
#if 0
			GLuint vbo;
			GLint id;
			glGenBuffers(1, &vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, e.count * sizeof(vec4), e.inst, GL_STREAM_DRAW);

#define OFFSET(Struct, Member) \
      (void*)(&((Struct*)NULL)->Member)

			id = glGetAttribLocation(shader->prog, "it_pos");
			if (id >= 0) {
				glEnableVertexAttribArray(id);
				glBindBuffer(GL_ARRAY_BUFFER, vbo);
				glVertexAttribPointer(id, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), OFFSET(vec4, x));
				glVertexAttribDivisor(id, 1);
			}
			id = glGetAttribLocation(shader->prog, "it_scale");
			if (id >= 0) {
				glEnableVertexAttribArray(id);
				glBindBuffer(GL_ARRAY_BUFFER, vbo);
				glVertexAttribPointer(id, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), OFFSET(vec4, w));
				glVertexAttribDivisor(id, 1);
			}
			glDisable(GL_CULL_FACE); /* TODO: only needed for the grass right now */
			glDrawArraysInstanced(mesh->primitive, 0, mesh->vertex_count, e.count);
			glEnable(GL_CULL_FACE);
			glBufferData(GL_ARRAY_BUFFER, e.count * sizeof(vec4), NULL, GL_STREAM_DRAW);
			glDeleteBuffers(1, &vbo);
#undef OFFSET
#endif
		}
	}
}



void
sys_render_init(struct memory_zone zone)
{
	struct system *sys = &g_state->sys_render;
	sys_init(sys, zone);
#if 0
	if (1 && g_state->depth_fbo != 0) {
		/* Free texture */
		delete_tex(&g_state->depth);

		/* Free the framebuffer */
		glDeleteFramebuffers(1, &g_state->depth_fbo);
		g_state->depth_fbo = 0;
	}
#endif
	if (g_state->depth_fbo != 0)
		return;

	struct texture *depth = &g_state->depth;
	size_t size = 1024*4;
	*depth = create_2d_tex(size, size, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glGenFramebuffers(1, &g_state->depth_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, g_state->depth_fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth->id, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		printf("Error framebuffer incomplete\n");
	}
}

static void
camera_update_orth(struct camera *c, int width, int height)
{
	float r = 64.0;
	float t = 64.0;
	float f = 800.0;//c->zFar;
	float n = 0.0; //c->zNear;

	c->proj.m[0][0] = 1.0 / r;
	c->proj.m[1][1] = 1.0 / t;
	c->proj.m[2][2] = -2.0  / (f - n);
	c->proj.m[2][3] = 0.0;
	c->proj.m[3][2] = -(f + n) / (f - n);
	c->proj.m[3][3] = 1.0;
}

void
sys_render_exec(void)
{
	struct light *light = &g_state->light;
//	camera_set(&g_state->sun, (vec3){-13.870439, 27.525631, -11.145432}, (quaternion){ {0.379877, 0.442253, -0.214377}, 0.783676});

	glBindFramebuffer(GL_FRAMEBUFFER, g_state->depth_fbo);

//	camera_init(&g_state->sun, 10.0, g_state->depth.width / g_state->depth.height);
	camera_update_orth(&g_state->sun, g_state->depth.width, g_state->depth.height);
	camera_set(&g_state->sun, light->pos, light->rot);

	/* clean depth buffer */
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, g_state->depth.width, g_state->depth.height);

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glCullFace(GL_FRONT);
	glEnable(GL_CULL_FACE);

	render_pass(g_state->sun, 1);

	if (g_state->debug) {
		debug_texture((vec2){200, 200}, &g_state->depth);
	}

	/* Rebind the default framebuffer */
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, g_state->width, g_state->height);

	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	render_pass(g_state->cam, 1);
}
