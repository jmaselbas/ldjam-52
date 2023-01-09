#pragma once

#include <time.h>

#include "core/engine.h"

typedef int64_t (file_size_t)(const char *path);
typedef int64_t (file_read_t)(const char *path, void *buf, size_t size);
typedef time_t (file_time_t)(const char *path);
typedef void (window_close_t)(void);
typedef void (window_cursor_t)(int show);
typedef double (window_time_t)(void);
struct io {
	file_size_t *file_size;
	file_read_t *file_read;
	file_time_t *file_time;
	window_close_t *close;  /* request window to be closed */
	window_cursor_t *show_cursor; /* request cursor to be shown */
	window_time_t *get_time;
};

struct game_memory {
	struct memory_zone state;
	struct memory_zone asset;
	struct memory_zone scrap;
	struct memory_zone audio;
};

/* typedef for function type */
typedef void (game_init_t)(struct game_memory *memory);
typedef void (game_step_t)(struct game_memory *memory, struct input *input, struct audio *audio);
typedef void (game_fini_t)(struct game_memory *memory);

/* declare functions signature */
game_init_t game_init;
game_step_t game_step;
game_fini_t game_fini;

struct system {
	struct memory_zone zone;
	struct list list;
};

void sys_init(struct system *sys, struct memory_zone zone);

#include "asset.h"
#include "entity.h"
#include "gui.h"

struct listener {
	vec3 pos;
	vec3 dir;
	vec3 left;
};

struct ent {
	vec3 pos;
	float scale;
	float radius;
	quaternion rot;
};

struct map {
	struct ent rocks[4096];
	struct ent small[4096];
};

struct game_state {
	int width;
	int height;

	struct camera cam;

	enum {
		GAME_INIT,
		GAME_MENU,
		GAME_PLAY,
		GAME_PAUSE,
	} state, next_state;
	enum {
		MENU_MAIN,
		MENU_OPTIONS,
	} menu;

	struct options {
		int mouse_inv_y;
		float mouse_speed; // 0.002;
		float main_volume;
		int audio_mute;
	} options;

	int debug;
	struct camera sun;

	struct light light;
	struct texture depth;
	unsigned int depth_fbo;
	struct system sys_render;

	vec3 player_pos;
	struct camera player_cam;

	int flycam;
	struct camera fly_cam;

	int mouse_grabbed;
	struct gui_state *gui;

	struct map map;

};

extern struct game_state *g_state;
extern struct game_asset *g_asset;
extern struct input *g_input;
extern struct io io;

static inline int
is_pressed(int key)
{
	return is_key_pressed(g_input, key);
}

static inline int
on_pressed(int key)
{
	return on_key_pressed(g_input, key);
}

static inline int
is_mouse_clic(int btn)
{
	return is_mouse_button_pressed(g_input, btn);
}

static inline int
on_mouse_clic(int btn)
{
	return on_mouse_button_pressed(g_input, btn);
}
