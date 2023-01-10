#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "game.h"
#include "asset.h"
#include "render.h"
#include "sound.h"
#include "core/wav.h"

extern struct game_state *g_state;

struct lrcv {
	float l;
	float r;
	float c;
	float v;
};

static struct lrcv
sound_spatializer(struct sound *s, struct listener *li)
{
	vec3 sound_pos = s->pos;
	vec3 player_pos = li->pos;
	vec3 player_dir = li->dir;
	vec3 player_left = li->left;
	vec3 v = vec3_normalize(vec3_sub(sound_pos, player_pos));
	float sin = vec3_dot(player_left, v);
	float cos = vec3_dot(player_dir, v);
	float l = MAX(0, sin);
	float r = ABS(MIN(0, sin));
	float c = ABS(cos);

	float d = vec3_norm(vec3_sub(player_pos, sound_pos));
	float vol = 1 / (d);
	return (struct lrcv) {
		.l = l,
		.r = r,
		.c = c,
		.v = vol
	};
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
sound_init(struct sound *snd, struct wav *wav, enum pb_mode mode, int autoplay,
	int is_positional, vec3 pos)
{
	snd->pos = pos;
	snd->is_positional = is_positional;
	sampler_init(&snd->sampler, wav, mode, autoplay);
}

void
do_audio(struct audio *audio)
{
	struct listener cur;
	struct listener nxt;
	size_t i, j;
	float l, r;
	static float lvol;
	float vol, nvol = g_state->options.audio_mute ? 0.0 :
		g_state->options.main_volume;
	struct sound *s;
	struct listener li;
	struct lrcv lrcv;
	cur = g_state->cur_listener;
	nxt = g_state->nxt_listener;

	for (i = 0; i < audio->size; i++) {
		audio->buffer[i].l = 0;
		audio->buffer[i].r = 0;
	}

	/* Note: listener interpolation seems to be working
	 * well with buffers of 1024 frames
	 */
	for (i=0; i < NB_SOUND; i++) {
		s = &g_state->sound[i];
		for (j = 0; j < audio->size; j++) {
			const float f = j / (float)audio->size;
			li = listener_lerp(cur, nxt, f);
			vol = mix(lvol, nvol, f);
			lrcv = sound_spatializer(s, &li);

			l = step_sampler(&s->sampler);
			if (s->sampler.wav->header.channels == 1) {
				r = l;
			} else {
				r = step_sampler(&s->sampler);
			}
			if (s->is_positional){
				audio->buffer[j].l += ((l * lrcv.l + l *lrcv.c)*lrcv.v)*vol;
				audio->buffer[j].r += ((r * lrcv.r + r *lrcv.c)*lrcv.v)*vol;
			} else {
				audio->buffer[j].l += l * vol;
				audio->buffer[j].r += r * vol;
			}

		}
	}
	if (audio->size > 0) {
		g_state->cur_listener = nxt;
		lvol = nvol;
	}
}


