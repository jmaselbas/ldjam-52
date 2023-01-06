#pragma once

#include <time.h>

#include "core/engine.h"

typedef int64_t (file_size_t)(const char *path);
typedef int64_t (file_read_t)(const char *path, void *buf, size_t size);
typedef time_t (file_time_t)(const char *path);
typedef void (window_close_t)(void);
typedef void (window_cursor_t)(int show);

struct io {
	file_size_t *file_size;
	file_read_t *file_read;
	file_time_t *file_time;
	window_close_t *close;  /* request window to be closed */
	window_cursor_t *show_cursor; /* request cursor to be shown */
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

struct listener {
	vec3 pos;
	vec3 dir;
	vec3 left;
};

struct game_state {
	struct game_input {
		struct input *input;
		struct input last_input;
	} input;
	float dt;
	int width;
	int height;

	struct camera cam;
	int flycam;
	int flycam_forward, flycam_left;
	float flycam_speed;

	enum {
		GAME_INIT,
		GAME_MENU,
		GAME_PLAY,
		GAME_PAUSE,
	} state, next_state;

	int debug;
	int key_debug;
	struct camera sun;

	struct light light;
	struct texture depth;
	unsigned int depth_fbo;
	struct system sys_render;

#if 0

	struct camera cam;
	struct camera sun;
	int flycam;
	int flycam_forward, flycam_left;
	float flycam_speed;

	struct listener cur_listener;
	struct listener nxt_listener;

	struct font ascii_font;

	float fps;
	struct system sys_text;
	struct system sys_sound;
	struct system sys_midi;

#define MAX_ENTITY_COUNT 512
	size_t entity_count;
	struct entity entity[MAX_ENTITY_COUNT];
#endif
};

extern struct game_state *g_state;
extern struct game_asset *g_asset;
extern struct game_input *g_input;
extern struct io io;

static inline int
is_pressed(int key)
{
	return key_pressed(g_input->input, key);
}

static inline int
on_pressed(int key)
{
	return key_pressed(g_input->input, key) && !key_pressed(&g_input->last_input, key);
}
