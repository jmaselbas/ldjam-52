#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "plat/glad.h"
#include <SDL.h>

#include "core/engine.h"
#include "game/game.h"
#include "plat/core.h"
#include "plat/audio.h"
#include "plat/libgame.h"

#define MSEC_PER_SEC 1000
static double
window_get_time(void)
{
	return SDL_GetTicks() / (double) MSEC_PER_SEC;
}

static double
rate_limit(double rate)
{
	static double tlast;
	double tnext, tcurr;
	struct timespec ts;
	double period = 1 / (double) rate;

	tnext = tlast + period;
	tcurr = window_get_time() + 0.0001; /* plus some time for the overhead */
	if (tcurr < tnext) {
		ts.tv_sec = 0;
		ts.tv_nsec = 1000000000 * (tnext - tcurr);
		nanosleep(&ts, NULL);
	}
	/* get current frame rate */
	tcurr = window_get_time();
	rate = 1.0 / (double) (tcurr - tlast);
	tlast = tcurr;
	return rate;
}

SDL_Window *window;
SDL_GLContext context;
unsigned int width = 1080;
unsigned int height = 800;
int should_close;
int focused;
int show_cursor;
static int xpre, ypre;

struct input game_input_next;
struct input game_input;
struct audio game_audio;
struct game_memory game_memory;

struct audio_config audio_config = {
	.samplerate = 48000,
	.channels = 2,
	.format = AUDIO_FORMAT_F32,
};

struct audio_state audio_state;

static void
request_close(void)
{
	should_close = 1;
}

static void
request_cursor(int show)
{
	show_cursor = show;
	SDL_SetRelativeMouseMode(show_cursor ? SDL_FALSE : SDL_TRUE);
}

static void
swap_input(struct input *new, struct input *buf)
{
	size_t i;

	/* input buffering: this is to make sure that input callbacks
	 * doesn't modify the game input while the game step runs. */

	buf->time = new->time;
	/* copy buffered input into the new input buffer */
	*new = *buf;
	/* grab time */
	new->time = window_get_time();
	new->dt = new->time - buf->time;

	/* update window's framebuffer size */
	new->width = width;
	new->height = height;

	/* flush xinc and yinc */
	buf->xinc = 0;
	buf->yinc = 0;
	for (i = 0; i < ARRAY_LEN(buf->keys); i++)
		buf->keys[i] &= ~KEY_CHANGED;
	for (i = 0; i < ARRAY_LEN(buf->buttons); i++)
		buf->buttons[i] &= ~KEY_CHANGED;
}

static void
window_init(char *name)
{
	if (SDL_InitSubSystem(SDL_INIT_VIDEO))
		die("SDL init failed: %s\n", SDL_GetError());

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, SDL_TRUE);

	window = SDL_CreateWindow(name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
				  width, height,
				  SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
				  | SDL_WINDOW_INPUT_FOCUS
				  | SDL_WINDOW_MOUSE_FOCUS
				  | SDL_WINDOW_MAXIMIZED
				  );

	if (!window)
		die("Failed to create window: %s\n", SDL_GetError());

	context = SDL_GL_CreateContext(window);
	if (!context)
		die("Failed to create openGL context: %s\n", SDL_GetError());

	SDL_GL_SetSwapInterval(1);

	show_cursor = 1;
	SDL_SetRelativeMouseMode(show_cursor ? SDL_FALSE : SDL_TRUE);

	if (!gladLoadGL((GLADloadfunc) SDL_GL_GetProcAddress))
		die("GL init failed\n");

	glViewport(0, 0, width, height);
}

static void
window_fini(void)
{
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

static int
window_should_close(void)
{
	return should_close;
}

static void
window_swap_buffers(void)
{
	SDL_GL_SwapWindow(window);
}

static void
focus_event(int focus)
{
	focused = focus;
	if (focused) {
		SDL_SetRelativeMouseMode(show_cursor ? SDL_FALSE : SDL_TRUE);
	} else {
		SDL_SetRelativeMouseMode(SDL_FALSE);
	}
}

static void
mouse_motion_event(int xpos, int ypos, int xinc, int yinc)
{
	struct input *input = &game_input_next;

	if (focused) {
		input->xpos = xpos;
		input->ypos = ypos;
		input->xinc += xinc;
		input->yinc += yinc;
	}
	xpre = xpos;
	ypre = ypos;
}

static void
mouse_button_event(unsigned int button, int act)
{
	struct input *input = &game_input_next;

	if (button < ARRAY_LEN(input->buttons))
		input->buttons[button] = act | KEY_CHANGED;
}

static void
key_event(unsigned int key, int mod, int act)
{
	struct input *input = &game_input_next;

	if (key == KEY_BACKSPACE) {
		if ((mod & KMOD_ALT) && (mod & KMOD_CTRL))
			should_close = 1;
	}

	if (key < ARRAY_LEN(input->keys))
		input->keys[key] = act | KEY_CHANGED;
}

/* Scancode vs Keycode:
 * Physical keyboard wiring
 *      -:-X-:-:-:-:-:-:--- > x1
 *      / / / / / / / /
 *    -:-:-:-:-:-:-:-:--- > x2
 *    / / / / / / /
 *  -:-:-:-:-:-:-:--- > x3
 *  / / / / / / /
 *  v v v v v v v
 *  y y2 ...    y7
 *
 * When key X is pressed we detect (x1,y2) which is converted into a keycode
 *                   driver
 * keyboard:scancode -------> layout(fr,en,...) -------> keycode
 */

static int
map_key(SDL_Scancode key)
{
	switch (key) {
	case SDL_SCANCODE_UNKNOWN:
	default: return KEY_UNKNOWN;
	case SDL_SCANCODE_RETURN: return KEY_ENTER;
	case SDL_SCANCODE_ESCAPE: return KEY_ESCAPE;
	case SDL_SCANCODE_BACKSPACE: return KEY_BACKSPACE;
	case SDL_SCANCODE_TAB: return KEY_TAB;
	case SDL_SCANCODE_SPACE: return KEY_SPACE;
	case SDL_SCANCODE_A: return KEY_A;
	case SDL_SCANCODE_B: return KEY_B;
	case SDL_SCANCODE_C: return KEY_C;
	case SDL_SCANCODE_D: return KEY_D;
	case SDL_SCANCODE_E: return KEY_E;
	case SDL_SCANCODE_F: return KEY_F;
	case SDL_SCANCODE_G: return KEY_G;
	case SDL_SCANCODE_H: return KEY_H;
	case SDL_SCANCODE_I: return KEY_I;
	case SDL_SCANCODE_J: return KEY_J;
	case SDL_SCANCODE_K: return KEY_K;
	case SDL_SCANCODE_L: return KEY_L;
	case SDL_SCANCODE_M: return KEY_M;
	case SDL_SCANCODE_N: return KEY_N;
	case SDL_SCANCODE_O: return KEY_O;
	case SDL_SCANCODE_P: return KEY_P;
	case SDL_SCANCODE_Q: return KEY_Q;
	case SDL_SCANCODE_R: return KEY_R;
	case SDL_SCANCODE_S: return KEY_S;
	case SDL_SCANCODE_T: return KEY_T;
	case SDL_SCANCODE_U: return KEY_U;
	case SDL_SCANCODE_V: return KEY_V;
	case SDL_SCANCODE_W: return KEY_W;
	case SDL_SCANCODE_X: return KEY_X;
	case SDL_SCANCODE_Y: return KEY_Y;
	case SDL_SCANCODE_Z: return KEY_Z;
	case SDL_SCANCODE_RIGHT: return KEY_RIGHT;
	case SDL_SCANCODE_LEFT:  return KEY_LEFT;
	case SDL_SCANCODE_DOWN:  return KEY_DOWN;
	case SDL_SCANCODE_UP:    return KEY_UP;
	case SDL_SCANCODE_LCTRL:  return KEY_LEFT_CONTROL;
	case SDL_SCANCODE_LSHIFT: return KEY_LEFT_SHIFT;
	case SDL_SCANCODE_LALT:   return KEY_LEFT_ALT;
	case SDL_SCANCODE_RCTRL:  return KEY_RIGHT_CONTROL;
	case SDL_SCANCODE_RSHIFT: return KEY_RIGHT_SHIFT;
	case SDL_SCANCODE_RALT:   return KEY_RIGHT_ALT;
	}
}

static void
window_poll_events(void)
{
	SDL_Event e;
	int key, mod, act;
	int w, h;

	SDL_GL_GetDrawableSize(window, &w, &h);
	width  = (w < 0) ? 0 : (int) w;
	height = (h < 0) ? 0 : (int) h;

	while (SDL_PollEvent(&e)) {
		switch(e.type) {
		case SDL_QUIT:
			should_close = 1;
			break;
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			key = map_key(e.key.keysym.scancode);
			mod = e.key.keysym.mod;
			act = e.key.state == SDL_PRESSED ? KEY_PRESSED : KEY_RELEASED;
			key_event(key, mod, act);
			break;
		case SDL_MOUSEMOTION:
			mouse_motion_event(e.motion.x, e.motion.y, e.motion.xrel, e.motion.yrel);
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			act = e.button.state == SDL_PRESSED ? KEY_PRESSED : KEY_RELEASED;
			mouse_button_event(e.button.button - 1, act);
			break;
		case SDL_WINDOWEVENT:
			if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
				focus_event(1);
			else if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
				focus_event(0);
			break;
		}
	}
}

static struct memory_zone
alloc_memory_zone(void *base, size_t align, size_t size)
{
	struct memory_zone zone;

	zone = memory_zone_init(xvmalloc(base, align, size), size);

	return zone;
}

static void
alloc_game_memory(struct game_memory *memory)
{
	memory->state = alloc_memory_zone(NULL, SZ_4M, SZ_16M);
	memory->scrap = alloc_memory_zone(NULL, SZ_4M, SZ_256M);
	memory->asset = alloc_memory_zone(NULL, SZ_4M, SZ_16M);
	memory->audio = alloc_memory_zone(NULL, SZ_4M, SZ_256M);
}

struct io io = {
	.file_size = file_size,
	.file_read = file_read,
	.file_time = file_time,
	.close = request_close,
	.show_cursor = request_cursor,
	.get_time = window_get_time,
};

struct libgame libgame;

static void
main_loop_step(void)
{
	window_poll_events();

	/* get an audio buffer */
	game_audio.size   = ring_buffer_write_size(&audio_state.buffer);
	game_audio.buffer = ring_buffer_write_addr(&audio_state.buffer);

	swap_input(&game_input, &game_input_next);
	if (libgame.step)
		libgame.step(&game_memory, &game_input, &game_audio);

	/* finalize audio write */
	ring_buffer_write_done(&audio_state.buffer, game_audio.size);
	audio_step(&audio_state);

	window_swap_buffers();
}

#ifdef __EMSCRIPTEN__
void emscripten_set_main_loop(void (* f)(), int, int);
#define main_loop_step() \
	{ emscripten_set_main_loop(main_loop_step, 0, 10); return 0; } while (0)
#endif

int
main(int argc, char **argv)
{
	if (argc == 2 && strcmp(argv[1], "-v") == 0) {
		printf("version %s\n", VERSION);
		return 0;
	}

	alloc_game_memory(&game_memory);

	libgame_init(&libgame);

	window_init(argv[0]);

	if (libgame.init)
		libgame.init(&game_memory);

	audio_state = audio_create(audio_config);
	audio_init(&audio_state);

	while (!window_should_close()) {
		if (libgame_changed(&libgame))
			libgame_reload(&libgame);
		main_loop_step();
		rate_limit(300);
	}

	if (libgame.fini)
		libgame.fini(&game_memory);

	window_fini();

	audio_fini(&audio_state);

	return 0;
}
