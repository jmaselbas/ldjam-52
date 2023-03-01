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

struct game_state *g_state;
struct game_asset *g_asset;
struct input *g_input;


static inline uint64_t rotl(const uint64_t x, int k) {
	return (x << k) | (x >> (64 - k));
}

static uint64_t s[4];
void rand_seed(uint64_t seed) {
	s[0] = s[1] = s[2] = s[3] = seed;
}

uint64_t rand_next(void) {

	const uint64_t result = s[0] + s[3];

	const uint64_t t = s[1] << 17;

	s[2] ^= s[0];
	s[3] ^= s[1];
	s[1] ^= s[2];
	s[0] ^= s[3];

	s[2] ^= t;

	s[3] = rotl(s[3], 45);

	return result;
}
void rand_jump(void) {
	static const uint64_t JUMP[] = { 0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa, 0x39abdc4529b1661c };

	uint64_t s0 = 0;
	uint64_t s1 = 0;
	uint64_t s2 = 0;
	uint64_t s3 = 0;
	for (size_t i = 0; i < ARRAY_LEN(JUMP); i++)
		for (int b = 0; b < 64; b++) {
			if (JUMP[i] & UINT64_C(1) << b) {
				s0 ^= s[0];
				s1 ^= s[1];
				s2 ^= s[2];
				s3 ^= s[3];
			}
			rand_next();
		}

	s[0] = s0;
	s[1] = s1;
	s[2] = s2;
	s[3] = s3;
}

static void
game_gen_map(struct map *map)
{
	size_t i;
	rand_seed(0xff55aa55deafbeef);
	for (i = 0; i < ARRAY_LEN(map->rocks); i++) {
		float x = -250.0 + 500.0 * ((rand_next()) / (float)UINT64_MAX);
		float y = -0.1   + 0.2 * ((rand_next()) / (float)UINT64_MAX);
		float z = -250.0 + 500.0 * ((rand_next()) / (float)UINT64_MAX);
		float a = -3.1415 + 3.1415 * ((rand_next()) / (float)UINT64_MAX);
		float s = 0.4 + 0.5 * ((rand_next()) / (float)UINT64_MAX);
		quaternion q = quaternion_axis_angle(VEC3_AXIS_Y, a);
		struct ent r = {
			.pos = {x, y, z},
			.scale = s,
			.radius = s*2,
			.rot = q,
		};
		map->rocks[i] = r;
	}
	for (i = 0; i < ARRAY_LEN(map->small); i++) {
		float x = -250.0 + 500.0 * ((rand_next()) / (float)UINT64_MAX);
		float y = -0.1   + 0.2 * ((rand_next()) / (float)UINT64_MAX);
		float z = -250.0 + 500.0 * ((rand_next()) / (float)UINT64_MAX);
		float a = -3.1415 + 3.1415 * ((rand_next()) / (float)UINT64_MAX);
		float s = 0.4 + 0.5 * ((rand_next()) / (float)UINT64_MAX);
		quaternion q = quaternion_axis_angle(VEC3_AXIS_Y, a);
		struct ent r = {
			.pos = {x, y, z},
			.scale = s,
			.radius = s*2,
			.rot = q,
		};
		map->small[i] = r;
	}
}

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
	g_state->options.mouse_inv_y = 0;
	g_state->options.mouse_speed = 0.001;
	g_state->options.main_volume = 0.5;
	g_state->options.audio_mute = 0;

	game_gen_map(&g_state->map);
	g_state->gui = gui_init(malloc(gui_size()));
	sound_init(&g_state->sound[0], game_get_wav(g_asset, WAV_THEME), LOOP, TRIG, 0, (vec3){0.,0.,0.});
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
	if (g_state->debug) {
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
dbg_light_mark(struct light *l)
{
	if (g_state->debug) {
	vec3 r = {1, 0, 0};
	vec3 g = {0, 1, 0};
	sys_render_push_cross(l->pos, (vec3){0.1,0.1,0.1}, g);
	sys_render_push_vec(l->pos, l->dir, r);
	}
}

static void
flycam_move(void)
{
	const float mouse_speed = g_state->options.mouse_speed;
	const int mouse_inv_y = g_state->options.mouse_inv_y;
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
		if (mouse_inv_y)
			dy = -dy;
		camera_rotate(cam, VEC3_AXIS_Y, -mouse_speed * dx);
		left = camera_get_left(cam);
		left = vec3_normalize(left);
		camera_rotate(cam, left, mouse_speed * dy);
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
	struct map *map = &g_state->map;
	const float player_radius = 0.4;
	const float S = 0.2;
	float deep;
	vec3 slv;
	size_t it, i;

	new.y = 0;
	slv = new;
	for (it = 0; it < 4; it++) {
		deep = -1;
	for (i = 0; i < ARRAY_LEN(map->rocks); i++) {
		float r = map->rocks[i].radius + player_radius;
		vec2 p = {map->rocks[i].pos.x, map->rocks[i].pos.z};
		vec2 n = {new.x, new.z};
		vec2 d = vec2_sub(p, n);
		float dist = vec2_dot(d, d);
		if (dist < r*r) {
			dbg_circle((vec3){p.x,0,p.y}, vec3_mult(map->rocks[i].radius,(vec3){1,0,1}), (vec3){1,0,0});
			dbg_circle((vec3){p.x,0,p.y}, vec3_mult(r, (vec3){1,0,1}), (vec3){0,1,0});

//			vec2 d = vec2_sub(dst, point_on_edge(dst, e[i])); /* distance to the edge */
			float depth = sqrt(dist);
			if (r - depth > deep) {
				vec2 dir = vec2_mult(1.0 / depth, d); /* normalized */
				deep = r - depth;
				dbg_line((vec3){p.x, 0, p.y}, (vec3){p.x-r*dir.x, 0, p.y-r*dir.y}, (vec3){1,0,0});
//				dbg_line((vec3){p.x, 0, p.y}, (vec3){p.x+r*dir.x, 0, p.y+r*dir.y}, (vec3){1,0,0});
				slv.x = p.x-r*dir.x;
				slv.z = p.y-r*dir.y;
			}
		}
	}
	new = slv;
	dbg_circle(new, vec3_mult(player_radius, (vec3){1,0,1}), (vec3){1,0,1});
	}

	return new;
}

static void
player_move(void)
{
	const float mouse_speed = g_state->options.mouse_speed;
	const float player_speed = 5;
	const int mouse_inv_y = g_state->options.mouse_inv_y;
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
		float a_dy = sin(mouse_speed * dy * (mouse_inv_y ? -1 : 1));
		float a_max = 0.1;

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

	new = player_walk(pos, new);
	g_state->player_pos = new;
}

#define VEC3_ONE (vec3){1,1,1}
static void
game_play(void)
{
	vec3 player_eye = (vec3){0.0, 1.5, 0.0};
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

}
static void
game_render(void)
{
	struct texture *depth = &g_state->depth;
	sys_render_push(&(struct render_entry){
			.shader = SHADER_TEST,
			.mesh = MESH_FLOOR,
			.scale = {1.0, 1.0, 1.0},
			.cull = 0,
			.position = g_state->player_pos,
			.rotation = QUATERNION_IDENTITY,
			.texture = {
				{.name = "shadowmap", .res_id = INTERNAL_TEXTURE, .tex = depth }
			},
		});

	struct map *map = &g_state->map;
	for (size_t i = 0; i < ARRAY_LEN(map->rocks); i++) {
		vec3 scale = { map->rocks[i].scale, map->rocks[i].scale, map->rocks[i].scale};
		sys_render_push(&(struct render_entry){
				.shader = SHADER_TEST,
				.mesh = MESH_ROCK_PILAR,
				.scale = scale,
				.cull = 1,
				.position = map->rocks[i].pos,
				.rotation = map->rocks[i].rot,
				.texture = {
					{.name = "shadowmap", .res_id = INTERNAL_TEXTURE, .tex = depth }
				},
			});
	}
	for (size_t i = 0; i < ARRAY_LEN(map->small); i++) {
		vec3 scale = { map->small[i].scale, map->small[i].scale, map->small[i].scale};
		sys_render_push(&(struct render_entry){
				.shader = SHADER_TEST,
				.mesh = MESH_ROCK_SMALL,
				.scale = scale,
				.cull = 1,
				.position = map->small[i].pos,
				.rotation = map->small[i].rot,
				.texture = {
					{.name = "shadowmap", .res_id = INTERNAL_TEXTURE, .tex = depth }
				},
			});
	}
	sys_render_push(&(struct render_entry){
			.shader = SHADER_SKY,
			.mesh = MESH_QUAD,
			.scale = {1, 1, 1},
			.position = {0, 0, 0},
			.color = { 0 },
			.rotation = QUATERNION_IDENTITY,
		});

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
	int l = 30;//strlen(txt);
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

	return mouse_in(x, y, w, h) && on_mouse_clic(0);
}

static int
enable(int x, int y, const char *txt, int en)
{
	int l = 30;//strlen(txt);
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
	gui_fill(x+w-h, y, h, h, c);
	if (en)
		gui_fill(x+w-h+2, y+2, h-4, h-4, gui_color(128, 128, 128));
	else
		gui_fill(x+w-h+2, y+2, h-4, h-4, gui_color(18, 18, 18));

	return mouse_in(x, y, w, h) && on_mouse_clic(0);
}

static float
slider(int x, int y, const char *txt, float min, float max, float val)
{
	int l = 30;//strlen(txt);
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
	gui_fill(x+w/2, y, w/2, h, c);
	gui_fill(x+w/2+2, y+2, w/2-4, h-4, gui_color(18, 18, 18));
	if (mouse_in(x+w/2+2, y+2, w/2-4, h-4) && is_mouse_clic(0)) {
		val = min + (max-min) * ((g_input->xpos - (x+w/2+2)) / (float)(w/2-4));
	}

	if (val < min) val = min;
	if (val > max) val = max;
	float per = (val - min)/(max - min);

	gui_fill(x+w/2+2, y+2, per*(w/2-4), h-4, gui_color(180, 18, 18));
//	gui_fill(x+w+2, y, h, h, c);
//	gui_fill(x+w-h*n+2, y+2, h-4, h-4, gui_color(128, 128, 128));
	if (mouse_in(x, y, w, h))
		gui_printf(x+w/2+2, y+2, "%f", val);
	return val;
}

static void
game_menu_main(void)
{
	int i = 0;
	int w = g_input->width;
	int y = g_input->height / 2;
	w = 64;

	gui_text(w, y + i++ * 32, "HARVEST LD52", gui_color(255, 255, 255));
	if (button(w, y + i++ * 32, "PLAY")) {
		g_state->next_state = GAME_PLAY;
	}
	if (button(w, y + i++ * 32, "OPTIONS"))
		g_state->menu = MENU_OPTIONS;
	if (button(w, y + i++ * 32, "ABOUT"))
		g_state->menu = MENU_ABOUT;
	i++;

	if (button(w, y + i++ * 32, "EXIT"))
		io.close();
	if (on_pressed(KEY_ESCAPE))
		g_state->next_state = GAME_PLAY;
}

static void
game_menu_options(void)
{
	int i = 0;
	int l = 30*14;
	int w = (g_input->width - l) / 2;
	int y = g_input->height / 2;

	if (button(64, y + i++ * 32, "EXIT"))
		g_state->menu = MENU_MAIN;
	if (on_pressed(KEY_ESCAPE))
		g_state->menu = MENU_MAIN;
	i = 0;

	i++;
	button(w, i++ * 32, "CONTROLS: WASD");

	i++;
	button(w, i++ * 32, "INPUT");
	if (enable(w, i++ * 32, "inv Y", g_state->options.mouse_inv_y))
		g_state->options.mouse_inv_y = !g_state->options.mouse_inv_y;

	g_state->options.mouse_speed = slider(w, i++ * 32, "speed", 0.0001, 0.005,
					      g_state->options.mouse_speed);

	i++;
	button(w, i++ * 32, "SOUND");
	if (enable(w, i++ * 32, "mute", g_state->options.audio_mute))
		g_state->options.audio_mute = !g_state->options.audio_mute;
	g_state->options.main_volume = slider(w, i++ * 32, "volume", 0.0, 1.0,
					      g_state->options.main_volume);

	i++;
	button(w, i++ * 32, "DEBUG");
	if (enable(w, i++ * 32, "enable", g_state->debug))
		g_state->debug = !g_state->debug;
	if (g_state->debug) {
		button(w, i++ * 32, " - flycam: Z");
		button(w, i++ * 32, " - debug: X");
	}
}

static void
game_menu_about(void)
{
	int i = 0;
	int l = 30*14;
	int w = (g_input->width - l) / 2;
	int y = g_input->height / 2;
	int c = gui_color(255, 255, 255);

	if (button(64, y + i++ * 32, "EXIT"))
		g_state->menu = MENU_MAIN;
	if (on_pressed(KEY_ESCAPE))
		g_state->menu = MENU_MAIN;

	i = 4;
	gui_text(w, i++ * 32, "LD52 jam entry haarvest", c);
	i++;
	gui_text(w, i++ * 32, "This \"game\" has been made by 2 people in 3 days", c);
	gui_text(w, i++ * 32, "There isn't much a gameplay ... that's hard", c);
	gui_text(w, i++ * 32, "But still here is the result", c);
	gui_text(w, i++ * 32, "We had a lot of doubts", c);
	gui_text(w, i++ * 32, "We had a lot of fun", c);
	gui_text(w, i++ * 32, "We came a long way to show you this", c);
	gui_text(w, i++ * 32, "This is not the best game in the world", c);
	gui_text(w, i++ * 32, "This is a tribute to our first jam", c);
	i++;

	gui_text(w, i++ * 32, "-- thanks", c);
	gui_text(w, i++ * 32, "Have a good year :3", c);
}

static void
game_menu(void)
{
	switch (g_state->menu) {
	case MENU_MAIN:
		game_menu_main();
		break;
	case MENU_OPTIONS:
		game_menu_options();
		break;
	case MENU_ABOUT:
		game_menu_about();
		break;
	}
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
		camera_set(&g_state->player_cam, (vec3){-68.169380, 1.500000, -42.527836}, (quaternion){ {0.119874, -0.395366, -0.052128}, -0.909185});
//		camera_set(&g_state->player_cam, (vec3){80.631607, 0.872238, 7.466835}, (quaternion){ {0.172121, -0.341403, -0.063745}, -0.921831});
		game_menu();
		if (g_state->debug)
			gui_text(0, 32, "menu", gui_color(250, 250, 250));
		break;
	case GAME_PLAY:
		game_play();
		if (on_pressed(KEY_ESCAPE))
			g_state->next_state = GAME_MENU;
		break;
	}
	game_render();
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

static void
audio_set_listener(vec3 pos, vec3 dir, vec3 left)
{
	g_state->nxt_listener.pos = pos;
	g_state->nxt_listener.dir = dir;
	g_state->nxt_listener.left = left;
}

static struct listener
listener_lerp(struct listener a, struct listener b, float x)
{
	struct listener r;

	r.pos = vec3_lerp(a.pos, b.pos, x);
	r.dir = vec3_lerp(a.dir, b.dir, x);
	r.left = vec3_lerp(a.left, b.left, x);

	return r;
}

void
do_audio(struct audio *a)
{
	size_t i, j;
	struct frame f;
	static float lvol;
	float x;
	float vol, nvol = g_state->options.audio_mute ? 0.0 :
		g_state->options.main_volume;
	struct sound *s;
	struct listener cur;
	struct listener nxt;
	struct listener lis;

	for (i = 0; i < a->size; i++) {
		a->buffer[i].l = 0;
		a->buffer[i].r = 0;
	}

	cur = g_state->cur_listener;
	nxt = g_state->nxt_listener;

	for (i=0; i < NB_SOUND; i++) {
		s = &g_state->sound[i];
		for (j = 0; j < a->size; j++) {
			x = j / (float)a->size;
			lis = listener_lerp(cur, nxt, x);
			if (sound_is_positional(s))
				f = sound_pos_step(s, &lis);
			else
				f = sound_step(s);

			vol = mix(lvol, nvol, x);
			a->buffer[j].l += f.l * vol;
			a->buffer[j].r += f.r * vol;
		}
	}
	if (a->size > 0) {
		g_state->cur_listener = nxt;
		lvol = nvol;
	}
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
	if (on_pressed('R')) {
		game_gen_map(&g_state->map);
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

	if (g_state->flycam) {
		g_state->cam = g_state->fly_cam;
	} else {
		g_state->cam = g_state->player_cam;
	}
	vec3 pos = g_state->cam.position;
	light_set_pos(&g_state->light, (vec3){-13.870439, 27.525631, -11.145432});
	light_look_at(&g_state->light, VEC3_ZERO, VEC3_AXIS_Y);
	pos = vec3_add(pos, vec3_mult(-50, light_get_dir(&g_state->light)));
	light_set_pos(&g_state->light, pos);
	dbg_light_mark(&g_state->light);

	sys_render_exec();
	audio_set_listener(g_state->cam.position,
                               vec3_normalize(camera_get_dir(&g_state->cam)),
                               vec3_normalize(camera_get_left(&g_state->cam)));
	do_audio(audio);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, g_input->width, g_input->height);
	glDisable(GL_CULL_FACE);
	gui_draw();

	gui_begin(g_state->gui);
	show_fps(1.0/g_input->dt);
	show_ms((io.get_time() - t1) * 1000.0);
	if (g_state->debug)
		gui_draw();

	game_asset_poll(g_asset);
}

