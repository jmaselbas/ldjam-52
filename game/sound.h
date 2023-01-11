#pragma once
#include "core/sampler.h"
#include "core/math.h"
#include "core/wav.h"

struct listener {
	vec3 pos;
	vec3 dir;
	vec3 left;
};

struct sound {
	struct sampler sampler;
	int mode;
	int is_positional;
	vec3 pos;
};

void sound_init(struct sound *s, struct wav *wav, int mode, int trig, int is_positional, vec3 pos);
int sound_is_positional(struct sound *s);
struct frame sound_pos_step(struct sound *s, struct listener *lis);
struct frame sound_step(struct sound *s);
