#include "game.h"
#include "asset.h"
#include "render.h"
#include "sound.h"
#include "entity.h"

static void
render_entity(size_t count, entity_id *id)
{
	struct texture *depth = &gs->depth;
	size_t i;

	sys_render_push(&(struct render_entry){
			.shader = SHADER_SKY,
			.mesh = MESH_QUAD,
			.scale = {1, 1, 1},
			.position = {0, 0, -1},
			.rotation = QUATERNION_IDENTITY,
			.cull = 0,
		});

	for (i = 0; i < count; i++) {
		struct entity *e = &gs->entity[id[i]];

		sys_render_push(&(struct render_entry){
				.shader = e->shader,
				.mesh = e->mesh,
				.scale = {1, 1, 1},
				.position = e->position,
				.rotation = QUATERNION_IDENTITY,
				.color = e->color,
				.texture = {
					{.name = "tex", .res_id = TEXTURE_CONCRETE },
					{.name = "nmap", .res_id = TEXTURE_CONCRETE_NORMAL },
					{.name = "depth", .res_id = INTERNAL_TEXTURE, .tex = depth }
				},
				.cull = 1,
			});
	}
}

vec4 it_pos[1000];

static void
grass_entity(size_t c, entity_id *id)
{
	struct texture *depth = &gs->depth;
	size_t arlen = ARRAY_LEN(it_pos);
	size_t count = 0;
	unsigned int i;

	if (c == 0)
		return;

	struct entity *e = &gs->entity[id[0]];
	for (i = 0; i < arlen; i++) {
		float v = 2.0 * M_PI * (i / ((float)arlen));
		vec4 p;

		p.x = 5 * (sin(v)+cos(v+i));
		p.y = 0;
		p.z = 5 * (sin(v)-cos(v+i));
		p.w = 1; /* scale */
		it_pos[count++] = p;
	}
	sys_render_push(&(struct render_entry){
			.shader = SHADER_GRASS,
			.mesh = MESH_GRASS_PATCH_1,
			.scale = {1, 1, 1},
			.position = e->position,
			.rotation = QUATERNION_IDENTITY,
			.color = {1, 0, 0},
			.texture = {
				{.name = "noise", .res_id = TEXTURE_NOISE },
				{.name = "depth", .res_id = INTERNAL_TEXTURE, .tex = depth }
			},
			.count = count,
			.inst = it_pos,
			.cull = 0,
		});
}

static void
move_entity(size_t count, entity_id *id)
{
	float dt = gs->dt;
	size_t i;

	for (i = 0; i < count; i++) {
		struct entity *e = &gs->entity[id[i]];
		vec3 pos = e->position;
		vec3 dir = e->direction;
		float speed = e->move_speed * dt;

		e->position = vec3_add(pos, vec3_mult(speed, dir));
	}
}

static void
debug_entity(struct entity *e)
{
	if (e->components.has_debug) {
		sys_render_push(&(struct render_entry){
				.shader = SHADER_SOLID,
				.mesh = e->debug_mesh,
				.scale = e->debug_scale,
				.position = e->position,
				.rotation = QUATERNION_IDENTITY,
				.color = { 0, 0.8, 0.2},
			});
	}

	sys_render_push_cross(e->position, (vec3){0.2,0.2,0.2}, (vec3){0,0.5,0.5});
}

void
entity_step(void)
{
	entity_id move_ids[MAX_ENTITY_COUNT];
	entity_id render_ids[MAX_ENTITY_COUNT];
	entity_id grass_ids[MAX_ENTITY_COUNT];
	size_t move_count = 0;
	size_t render_count = 0;
	size_t grass_count = 0;
	entity_id id;

	for (id = 0; id < gs->entity_count; id++) {
		struct entity *e = &gs->entity[id];

		if (e->components.has_movement)
			move_ids[move_count++] = id;

		if (e->components.has_render)
			render_ids[render_count++] = id;

		if (e->components.is_grass)
			grass_ids[grass_count++] = id;
	}

	move_entity(move_count, move_ids);
	render_entity(render_count, render_ids);
	grass_entity(grass_count, grass_ids);

	if (gs->debug) {
		for (id = 0; id < gs->entity_count; id++) {
			debug_entity(&gs->entity[id]);
		}
	}
}
