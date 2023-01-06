#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

#include "game.h"
#include "asset.h"
#include "entity.h"
#include "render.h"
#include "text.h"
#include "sound.h"

struct game_state *g_state;
struct game_asset *g_asset;
struct game_input *g_input;

void
sys_init(struct system *sys, struct memory_zone zone)
{
	sys->zone = zone;
	list_init(&sys->list);
}

void
game_init(struct game_memory *game_memory)
{
	g_state = mempush(&game_memory->state, sizeof(struct game_state));
	g_asset = mempush(&game_memory->asset, sizeof(struct game_asset));

	game_asset_init(g_asset, &game_memory->asset, &game_memory->audio);

	camera_init(&g_state->cam, 1.05, 1);
	camera_set_znear(&g_state->cam, 0.1);
	camera_set(&g_state->cam, (vec3){-1., 4.06, 8.222}, (quaternion){ {0.018959, 0.429833, 0.009028}, -0.902673});
	camera_look_at(&g_state->cam, (vec3){0.0, 0., 0.}, (vec3){0., 1., 0.});
	g_state->flycam_speed = 1;

	g_state->state = GAME_INIT;
	g_state->next_state = GAME_MENU;
}

void
game_fini(struct game_memory *memory)
{
	struct game_asset *game_asset = memory->asset.base;
	game_asset_fini(game_asset);
}

#if 0
static void
game_set_player_movement(void)
{
	int dir_forw = 0, dir_left = 0;

	if (key_pressed(input, 'W'))
		dir_forw += 1;
	if (key_pressed(input, 'S'))
		dir_forw -= 1;
	if (key_pressed(input, 'A'))
		dir_left += 1;
	if (key_pressed(input, 'D'))
		dir_left -= 1;
	if (dir_forw || dir_left) {
		vec3 forw = vec3_mult(dir_forw, (vec3){0,0,1});
		vec3 left = vec3_mult(dir_left, (vec3){1,0,0});
		vec3 dir = vec3_add(forw, left);
		e->direction = vec3_normalize(dir);
	} else {
		e->direction = VEC3_ZERO;
	}
}
#endif

static void
debug_origin_mark(void)
{
	vec3 o = {0, 0, 0};
	vec3 r = {1, 0, 0};
	vec3 g = {0, 1, 0};
	vec3 b = {0, 0, 1};
	sys_render_push_cross(o, (vec3){1,0,0}, r);
	sys_render_push_cross(o, (vec3){0,1,0}, g);
	sys_render_push_cross(o, (vec3){0,0,1}, b);
}

static void
debug_light_mark(struct light *l)
{
	vec3 r = {1, 0, 0};
	vec3 g = {0, 1, 0};
	sys_render_push_cross(l->pos, (vec3){0.1,0.1,0.1}, g);
	sys_render_push_vec(l->pos, l->dir, r);
}

static void
flycam_move(struct input *input)
{
	float dt = g_state->dt;
	vec3 forw = camera_get_dir(&g_state->cam);
	vec3 left = camera_get_left(&g_state->cam);
	vec3 dir = { 0 };
	int fly_forw, fly_left;
	float dx, dy;
	float speed;

	if (is_pressed(KEY_LEFT_SHIFT))
		speed = 10 * dt;
	else
		speed = 2 * dt;

	fly_forw = 0;
	if (is_pressed('W'))
		fly_forw += 1;
	if (is_pressed('S'))
		fly_forw -= 1;

	fly_left = 0;
	if (is_pressed('A'))
		fly_left += 1;
	if (is_pressed('D'))
		fly_left -= 1;

	if (fly_forw || fly_left) {
		forw = vec3_mult(fly_forw, forw);
		left = vec3_mult(fly_left, left);
		dir = vec3_add(forw, left);
		dir = vec3_normalize(dir);
		dir = vec3_mult(speed, dir);
		camera_move(&g_state->cam, dir);
	}

	dx = input->xinc;
	dy = input->yinc;

	if (dx || dy) {
		camera_rotate(&g_state->cam, VEC3_AXIS_Y, -0.001 * dx);
		left = camera_get_left(&g_state->cam);
		left = vec3_normalize(left);
		camera_rotate(&g_state->cam, left, 0.001 * dy);
	}

	/* drop camera config to stdout */
	if (on_pressed(KEY_SPACE)) {
		vec3 pos = g_state->cam.position;
		quaternion rot = g_state->cam.rotation;

		printf("camera_set(&game_state->cam, (vec3){%f, %f, %f}, (quaternion){ {%f, %f, %f}, %f});\n", pos.x, pos.y, pos.z, rot.v.x, rot.v.y, rot.v.z, rot.w);
	}
}

void
game_update(void)
{
	switch (g_state->state) {
	case GAME_INIT: /* nothing to do */
		g_state->next_state = GAME_MENU;
		break;
	case GAME_MENU:
	case GAME_PLAY:
		break;
	}
	g_state->state = g_state->next_state;
}

void
game_step(struct game_memory *memory, struct input *input, struct audio *audio)
{
	g_state = memory->state.base;
	g_asset = memory->asset.base;
	g_input = &g_state->input;

	g_state->width = input->width;
	g_state->height = input->height;
	g_input->input = input;
	g_state->dt = input->time - g_input->last_input.time;
	camera_set_ratio(&g_state->cam, (float)input->width / (float)input->height);

	memory->scrap.used = 0;
	sys_render_init(memory_zone_init(mempush(&memory->scrap, SZ_4M), SZ_4M));

	if (on_pressed('X')) {
		g_state->debug = !g_state->debug;
		printf("debug %s\n", g_state->flycam ? "on":"off");
	}
	if (on_pressed('Z')) {
		g_state->flycam = !g_state->flycam;
		printf("flycam %s\n", g_state->flycam ? "on":"off");
	}
	if (g_state->flycam) {
		flycam_move(input);
	}

	debug_origin_mark();
	/* do update here */
	game_update();
	sys_render_exec();

	g_input->last_input = *input;
	game_asset_poll(g_asset);
}
