#include <string.h>
#include <stdarg.h>
#include <stdio.h>
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
sound_get_panning(struct sound *s, struct listener *li)
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

void
sound_init(struct sound *s, struct wav *wav, int mode, int trig,
	int is_positional, vec3 pos)
{
	s->pos = pos;
	s->is_positional = is_positional;
	sampler_init(&s->sampler, wav, mode, trig);
}

int
sound_is_positional(struct sound *s)
{
	return s->is_positional;
}

struct frame
sound_pos_step(struct sound *s, struct listener *lis)
{
	struct frame out;
	struct lrcv lrcv;
	lrcv = sound_get_panning(s, lis);
	out = sound_step(s);
	out.l = ((out.l * lrcv.l + out.l *lrcv.c)*lrcv.v);
	out.r = ((out.r * lrcv.r + out.r *lrcv.c)*lrcv.v);
	return out;
}

struct frame
sound_step(struct sound *s)
{
	struct frame out;
	sample l, r;
	l = step_sampler(&s->sampler);
	if (s->sampler.wav->header.channels == 1) {
		r = l;
	} else {
		r = step_sampler(&s->sampler);
	}
	out.l = l;
	out.r = r;
	return out;
}
