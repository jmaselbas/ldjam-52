#pragma once

#include "audio.h"

enum sampler_state{
	STOP,
	PLAY,
};

enum pb_mode{
	ONE,
	LOOP,
};

struct sampler {
	struct wav *wav;
	enum sampler_state state;
	size_t pb_start;
	size_t pb_end;
	size_t pb_head; /* playhead position */

	size_t loop_start;
	size_t loop_end;
	int    loop_on;

	/* Trig */
	unsigned char trig_on; /* input trig signal */

	/* Amp */
	float vol;
};

void sampler_init(struct sampler *sampler, struct wav *wav, enum pb_mode mode, int autoplay);
sample step_sampler(struct sampler *sampler);
