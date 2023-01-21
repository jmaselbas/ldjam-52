#pragma once

#include "core/engine.h"
#include "hrtf.h"
struct sound {
	struct sampler sampler;
	int mode;
	int is_positional;
	int16_t ltaps[HRTF_48_TAPS];
	int16_t rtaps[HRTF_48_TAPS];
	vec3 pos;
	double vol;
};

void sound_init(struct sound *s, struct wav *wav, int mode, int trig, int is_positional, vec3 pos);
int sound_update_hrtf(struct sound *s, struct camera *c);
int sound_is_positional(struct sound *s);
int sound_get_frame(struct sound *s, struct frame *out);
int sound_pos_buf_cur_nxt(struct sound *s, struct camera *cur, struct camera *nxt, struct frame *out, int size);
