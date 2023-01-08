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
struct input *g_input;

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

	g_state->fly_cam = g_state->player_cam = g_state->cam;

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
dbg_origin_mark(void)
{
	if (g_state->debug || 1) {
	vec3 o = {0, 0, 0};
	vec3 r = {1, 0, 0};
	vec3 g = {0, 1, 0};
	vec3 b = {0, 0, 1};
	sys_render_push_cross(o, (vec3){1,0,0}, r);
	sys_render_push_cross(o, (vec3){0,1,0}, g);
	sys_render_push_cross(o, (vec3){0,0,1}, b);
	}
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
	struct camera *cam = &g_state->fly_cam;
	float dt = g_input->dt;
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
		dx = g_input->xinc;
		dy = g_input->yinc;
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
dbg_circle(vec3 at, vec3 scale, vec3 color)
{
	if (g_state->debug)
	sys_render_push(&(struct render_entry){
			.shader = SHADER_SOLID,
			.mesh = DEBUG_MESH_SPHERE,
			.scale = scale,
			.position = at,
			.rotation = QUATERNION_IDENTITY,
			.color = color,
		});
}
static void
dbg_cross(vec3 at, vec3 scale, vec3 color)
{
	if (g_state->debug)
	sys_render_push_cross(at, scale, color);
}

static void
dbg_line(vec3 a, vec3 b, vec3 color)
{
	if (g_state->debug) {
	vec3 dir = vec3_sub(b, a);
	vec3 pos = vec3_add(a, vec3_mult(0.5, dir));
	vec3 scale = {0, 0, vec3_norm(dir) * 0.5};
	quaternion rot = quaternion_look_at(dir, (vec3){0,1,0});
	sys_render_push(&(struct render_entry){
			.shader = SHADER_SOLID,
			.mesh = DEBUG_MESH_CROSS,
			.scale = scale,
			.position = pos,
			.rotation = rot,
			.color = color,
		});
	}
}

static vec3
vec3_on_edge(vec3 p, vec3 a, vec3 b)
{
	vec3 ab = vec3_sub(b, a);
	vec3 ap = vec3_sub(p, a); /* a -> p */
	vec3 bp = vec3_sub(p, b); /* b -> p */

	if (vec3_dot(ap, ab) <= 0)
		return a; /* p is before a on the ab edge */
	if (vec3_dot(bp, ab) >= 0)
		return b; /* p is after b on the ab edge */
	ab = vec3_normalize(ab);
	return vec3_add(a, vec3_mult(vec3_dot(ab, ap), ab));
}

static int
dist_to_triangle(vec3 q, vec3 a, vec3 b, vec3 c, float r)
{
	/* triangle:
	 *      + c
	 *     / \
	 *    /   \
	 * a +-----+ b
	 */
	/* n = (b - a) x (c - a) */
	vec3 n = vec3_normalize(vec3_cross(vec3_sub(b, a), vec3_sub(c, a)));
	vec3 e1 = vec3_sub(b, a); /* edges */
	vec3 e2 = vec3_sub(c, b);
	vec3 e3 = vec3_sub(a, c);
	vec3 qa = vec3_sub(q, a);
	vec3 qb = vec3_sub(q, b);
	vec3 qc = vec3_sub(q, c);
	vec3 x1 = vec3_cross(e1, qa); /* side / "normal" */
	vec3 x2 = vec3_cross(e2, qb);
	vec3 x3 = vec3_cross(e3, qc);

	if (vec3_dot(x1, n) >= 0 &&
	    vec3_dot(x2, n) >= 0 &&
	    vec3_dot(x3, n) >= 0) {
		/* point inside triangle */
		return 1;
	}

	vec3 q1 = vec3_on_edge(q, a, b);
	vec3 q2 = vec3_on_edge(q, b, c);
	vec3 q3 = vec3_on_edge(q, c, a);
	vec3 d1 = vec3_sub(q, q1);
	vec3 d2 = vec3_sub(q2, q);
	vec3 d3 = vec3_sub(q3, q);

	if (vec3_dot(d1, d1) < r*r ||
	    vec3_dot(d2, d2) < r*r ||
	    vec3_dot(d3, d3) < r*r)
		return 1;

	return 0;
}

static vec3
player_walk(vec3 pos, vec3 new)
{
	const float player_radius = 0.5;
	const float S = 0.1;
	const size_t N = 10;
	vec3 tri[3] = {
		{1.2,0,0},
		{-1.6,0,-1.2},
		{0,0,2.3},
	};

	vec3 s_pos[N][N];
	int x, y;
	float ox, oy;
	int px = pos.x/S, py = pos.z/S;
	ox = S*(px - (int)(N/2));
	oy = S*(py - (int)(N/2));
	static float t;
	t += g_input->dt;
	for (x = 0; x < N; x++) {
		for (y = 0; y < N; y++) {
			vec3 pp = {ox + S*x, 0, oy + S*y};
			if (dist_to_triangle(pp, tri[0], tri[1], tri[2], player_radius))
				dbg_cross(pp, (vec3){0.05*sin(t),0.05*sin(t+x+y),0.05*sin(t+x+y)}, (vec3){1,0,0});
			else
				dbg_cross(vec3_add((vec3){0,0.05*sin(t+x+y),0},pp), (vec3){0.05,0.05,0.05}, (vec3){0,1,0});
		}
	}
	dbg_line(tri[0], tri[1], (vec3){0,1.0,1.0});
	dbg_line(tri[1], tri[2], (vec3){0,1.0,1.0});
	dbg_line(tri[0], tri[2], (vec3){0,1.0,1.0});
	dbg_circle(pos,(vec3){player_radius,0.0,player_radius}, (vec3){0,1.0,1.0});
	dbg_circle(new,(vec3){player_radius,0.0,player_radius}, (vec3){0,1.0,0});

	return new;
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

	float dt = g_input->dt;
	dir = vec3_mult(dt * player_speed, dir);
	new = vec3_add(pos, dir);

	dx = g_input->xinc;
	dy = g_input->yinc;

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

	g_state->player_pos = player_walk(pos, new);
}

#define VEC3_ONE (vec3){1,1,1}
static void
game_play(void)
{
	vec3 player_eye = (vec3){0.0, 0.7, 0.0};
	if (g_state->flycam)
		flycam_move();
	else {
	player_move();
	camera_set_position(&g_state->player_cam, vec3_add(g_state->player_pos, player_eye));
	}
	if (g_state->debug) {
		vec3 p = g_state->player_pos;
		gui_printf(0, g_input->height - 32, "pos %f %f %f", p.x, p.y, p.z);
	}

#if 1
	for (float i = 0; i < 10; i++) {
		float d = 40;
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

}

static int
mouse_in(int x, int y, int w, int h)
{
	return x < g_input->xpos && g_input->xpos < (x+w) &&
		y < g_input->ypos && g_input->ypos < (y+h);
}

static int
button(int x, int y, const char *txt)
{
	int l = 20;//strlen(txt);
	int c;
	int w = l*14;
	int h = 18;

	if (mouse_in(x, y, w, h)) {
		c = gui_color(255, 255, 255);
		gui_fill(x, y, w, h, gui_color(18, 18, 18));
	} else {
		c = gui_color(128, 128, 128);
	}
	gui_text(x, y, txt, c);

	return mouse_in(x, y, w, h) && mouse_button_pressed(g_input, 0);
}

static void
game_menu(void)
{
	int i = 0;
	int w = g_input->width;
	int y = g_input->height / 2;;
	w = 64;

	gui_text(w, y + i++ * 32, "HARVEST LD52", gui_color(255, 255, 255));
	if (button(w, y + i++ * 32, "PLAY"))
		g_state->next_state = GAME_PLAY;
	if (button(w, y + i++ * 32, "OPTIONS"))
		;
	i++;
	if (button(w, y + i++ * 32, "EXIT"))
		io.close();
	gui_fill(g_input->xpos, g_input->ypos, 2, 2, gui_color(255, 255, 255));
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
		game_menu();
		gui_text(0, 32, "menu", gui_color(250, 250, 250));
		if (on_pressed(KEY_ESCAPE))
			g_state->next_state = GAME_PLAY;
		break;
	case GAME_PLAY:
		game_play();
		if (on_pressed(KEY_ESCAPE))
			g_state->next_state = GAME_MENU;
		break;
	}

	if (g_state->state == g_state->next_state)
		return;

	g_state->state = g_state->next_state;
	switch (g_state->state) {
	case GAME_MENU:
		g_state->mouse_grabbed = 0;
		io.show_cursor(!g_state->mouse_grabbed);
		break;
	case GAME_PLAY:
		g_state->mouse_grabbed = 1;
		io.show_cursor(!g_state->mouse_grabbed);
		break;
	}
}

static void
show_fps(double f) {
	static float fps[64];
	static size_t fps_count;
	float tf = 0;
	int col = gui_color(255,255,255);
	int x = g_input->width - ARRAY_LEN(fps)*2;
	int y = g_input->height - 32;
	fps_count = (fps_count + 1) % ARRAY_LEN(fps);
	fps[fps_count] = f;
	for (size_t i = 0; i < ARRAY_LEN(fps); i++) {
		tf += f = fps[(fps_count+i)%ARRAY_LEN(fps)];
		gui_fill(x+i, y+0.25 * f, 1, 0.25 * MAX(120.0 - f, 0.0), col);
	}
	tf /= ARRAY_LEN(fps);
	gui_printf(x+ARRAY_LEN(fps), y, "%2.0ffps", tf);
}
static void
show_ms(double f) {
	static float fps[64];
	static size_t fps_count;
	float tf = 0;
	int col = gui_color(255,255,255);
	int x = g_input->width - ARRAY_LEN(fps)*2;
	int y = g_input->height - 64;
	fps_count = (fps_count + 1) % ARRAY_LEN(fps);
	fps[fps_count] = f;
	for (size_t i = 0; i < ARRAY_LEN(fps); i++) {
		tf += f = fps[(fps_count+i)%ARRAY_LEN(fps)];
		gui_fill(x+i, y+0.25 * f, 1, 0.25 * MAX(120.0 - f, 0.0), col);
	}
	tf /= ARRAY_LEN(fps);
	gui_printf(x+ARRAY_LEN(fps), y, "%2.0fms", tf);
}

void
game_step(struct game_memory *memory, struct input *input, struct audio *audio)
{
	double t1 = io.get_time();
	g_state = memory->state.base;
	g_asset = memory->asset.base;
	g_input = input;

	g_state->width = input->width;
	g_state->height = input->height;
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

	dbg_origin_mark();
	/* do update here */
	game_main();

	if (g_state->flycam || 1) {
		g_state->cam = g_state->fly_cam;
	} else {
		g_state->cam = g_state->player_cam;
	}
	sys_render_exec();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, g_input->width, g_input->height);
	glDisable(GL_CULL_FACE);
	show_fps(1.0/g_input->dt);
	show_ms(io.get_time() - t1);
	gui_printf(500, 500, "W: %s", is_pressed('W') ? "yes" : "no");
	gui_draw();

	game_asset_poll(g_asset);
}
