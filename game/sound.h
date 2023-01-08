#pragma once
#include "core/sampler.h"

struct sound {
	struct sampler sampler;
	enum pb_mode mode;
	int is_positional;
	vec3 pos;
};

void sound_init(struct sound *sound, struct wav *wav, enum pb_mode mode, int autoplay, int is_positional, vec3 pos);

void do_audio(struct audio *audio);


