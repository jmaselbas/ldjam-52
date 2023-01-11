#pragma once
#include "core/sampler.h"

struct sound {
	struct sampler sampler;
	int mode;
	int is_positional;
	vec3 pos;
};

void sound_init(struct sound *s, struct wav *wav, int mode, int trig, int is_positional, vec3 pos);

void do_audio(struct audio *audio);


