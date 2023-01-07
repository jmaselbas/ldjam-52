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
	g_state->player_cam = g_state->cam;

	g_state->state = GAME_INIT;

	g_state->gui = gui_init(malloc(gui_size()));
}

void
game_fini(struct game_memory *memory)
{
	struct game_asset *game_asset = memory->asset.base;
	game_asset_fini(game_asset);
}

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
flycam_move(void)
{
	struct input *input = g_input->input;
	struct camera *cam = &g_state->fly_cam;
	float dt = g_state->dt;
	vec3 forw = camera_get_dir(cam);
	vec3 left = camera_get_left(cam);
	vec3 dir = { 0 };
	int fly_forw = 0, fly_left = 0;
	float dx = 0, dy = 0;
	float speed;

	if (is_pressed(KEY_LEFT_SHIFT))
		speed = 10 * dt;
	else
		speed = 2 * dt;

	if (is_pressed('W'))
		fly_forw += 1;
	if (is_pressed('S'))
		fly_forw -= 1;
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
		camera_move(cam, dir);
	}

	if (g_state->mouse_grabbed) {
		dx = input->xinc;
		dy = input->yinc;
	}

	if (dx || dy) {
		camera_rotate(cam, VEC3_AXIS_Y, -0.001 * dx);
		left = camera_get_left(cam);
		left = vec3_normalize(left);
		camera_rotate(cam, left, 0.001 * dy);
	}

	/* drop camera config to stdout */
	if (on_pressed(KEY_SPACE)) {
		vec3 pos = cam->position;
		quaternion rot = cam->rotation;

		printf("camera_set(&game_state->cam, (vec3){%f, %f, %f}, (quaternion){ {%f, %f, %f}, %f});\n", pos.x, pos.y, pos.z, rot.v.x, rot.v.y, rot.v.z, rot.w);
	}
}

static void
player_move(void)
{
	const float mouse_speed = 0.002;
	const float player_speed = 1;
	const int mouse_inv_y = 0;
	struct camera *cam = &g_state->player_cam;
	int dir_forw = 0, dir_left = 0;
	vec3 left = camera_get_left(cam);
	vec3 forw = vec3_cross(left, VEC3_AXIS_Y);
	vec3 pos = g_state->player_pos;
	vec3 dir, new;
	float dx, dy;

	if (is_pressed('W'))
		dir_forw += 1;
	if (is_pressed('S'))
		dir_forw -= 1;
	if (is_pressed('A'))
		dir_left += 1;
	if (is_pressed('D'))
		dir_left -= 1;
	if (dir_forw || dir_left) {
		forw = vec3_mult(dir_forw, forw);
		left = vec3_mult(dir_left, left);
		dir = vec3_normalize(vec3_add(forw, left));
	} else {
		dir = VEC3_ZERO;
	}

	float dt = g_state->dt;
	dir = vec3_mult(dt * player_speed, dir);
	new = vec3_add(pos, dir);

	dx = g_input->input->xinc;
	dy = g_input->input->yinc;

	if (dx || dy) {
		float f = vec3_dot(VEC3_AXIS_Y, camera_get_dir(cam));
		float u = vec3_dot(VEC3_AXIS_Y, camera_get_up(cam));
		float a_dy = sin(mouse_speed * dy);
		float a_max = 0.1;

		if (mouse_inv_y)
			camera_rotate(cam, VEC3_AXIS_Y,  mouse_speed * dx);
		else
			camera_rotate(cam, VEC3_AXIS_Y, -mouse_speed * dx);
		left = camera_get_left(cam);
		left = vec3_normalize(left);

		/* clamp view to a_max angle */
		if (f > 0 && (u + a_dy) < a_max) {
			a_dy = a_max - u;
		} else if (f < 0 && (u - a_dy) < a_max) {
			a_dy = -(a_max - u);
		}
		camera_rotate(cam, left, a_dy);
	}

	for (float x = -10; x < 10; x++) {
	for (float y = -10; y < 10; y++) {
		vec3 s = (vec3){0.1,0.1,0.1};
		vec3 c = (vec3){1.0,0.1,0.1};
		vec3 p = (vec3){x, 0 , y};
		sys_render_push_cross(p, s, c);
	}}

	g_state->player_pos = new;
}

#define VEC3_ONE (vec3){1,1,1}
static void
game_play(void)
{
	sys_render_push_cross(g_state->player_pos, VEC3_ONE, VEC3_AXIS_X);
	if (g_state->flycam) {
		flycam_move();
	} else {
		vec3 player_eye = (vec3){0.0, 0.7, 0.0};
		player_move();
		camera_set_position(&g_state->player_cam, vec3_add(g_state->player_pos, player_eye));
	}
	sys_render_push_cross(g_state->player_pos, VEC3_ONE, VEC3_AXIS_Z);
}

static void
game_main(void)
{
	switch (g_state->state) {
	case GAME_INIT: /* nothing to do */
		g_state->next_state = GAME_MENU;
		g_state->mouse_grabbed = 1;
		io.show_cursor(!g_state->mouse_grabbed);
		break;
	case GAME_MENU:
	case GAME_PLAY:
		game_play();
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
	camera_set_ratio(&g_state->fly_cam, (float)input->width / (float)input->height);
	camera_set_ratio(&g_state->player_cam, (float)input->width / (float)input->height);

	memory->scrap.used = 0;
	sys_render_init(memory_zone_init(mempush(&memory->scrap, SZ_4M), SZ_4M));
	gui_begin(g_state->gui);

	if (on_pressed('X')) {
		g_state->debug = !g_state->debug;
	}
	if (on_pressed(KEY_ESCAPE)) {
		g_state->mouse_grabbed = !g_state->mouse_grabbed;
		io.show_cursor(!g_state->mouse_grabbed);
	}
	if (on_pressed('Z')) {
		g_state->flycam = !g_state->flycam;
		if (g_state->flycam)
			g_state->fly_cam = g_state->cam;
	}
	if (g_state->flycam)
		gui_text(0, 0, "flycam", gui_color(255, 255, 255));
	if (g_state->debug)
		gui_text(0, 16, "debug on", gui_color(255, 255, 255));

//	debug_origin_mark();
	/* do update here */
	game_main();

#if 1
	for (float i = 0; i < 10; i++) {
		float d = 12;
		sys_render_push(&(struct render_entry){
				.shader = SHADER_TEST,
				.mesh = MESH_ROCK_TEST,
				.scale = {1,1,1},
				.position = {d*sin((i/5.) * 3.1415),-0.5,d*cos((i/5.) * 3.1415)},
				.rotation = QUATERNION_IDENTITY,
				.color = (vec3){1.0,0,0},
			});
	}
#endif
	if (g_state->flycam || 1) {
		g_state->cam = g_state->fly_cam;
	} else {
		g_state->cam = g_state->player_cam;
	}
	sys_render_exec();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, g_state->width, g_state->height);
	glDisable(GL_CULL_FACE);
	if (g_state->debug)
		gui_draw();

	g_input->last_input = *input;
	game_asset_poll(g_asset);
}
