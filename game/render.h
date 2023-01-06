#pragma once

#include "asset.h"

void render_bind_shader(struct shader *shader);
void render_bind_mesh(struct shader *shader, struct mesh *mesh);
void render_bind_camera(struct shader *s, struct camera *c);
void render_mesh(struct mesh *mesh);

struct render_input {
	const char *name;
	enum asset_key res_id;
	struct texture *tex;
};

#define RENDER_MAX_TEXTURE_UNIT 8
struct render_entry {
	enum asset_key shader;
	enum asset_key mesh;
	struct render_input texture[RENDER_MAX_TEXTURE_UNIT];
	quaternion rotation;
	vec3 position;
	vec3 scale;
	vec3 color;
	int count;
	vec4 *inst;
	int cull:1;
};

void sys_render_init(struct memory_zone zone);
void sys_render_push(struct render_entry *);
void sys_render_exec(void);

void sys_render_push_cross(vec3 at, vec3 scale, vec3 color);
void sys_render_push_vec(vec3 pos, vec3 dir, vec3 color);
void sys_render_push_bounding(vec3 pos, vec3 scale, struct bounding_volume *bvol);

