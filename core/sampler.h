#pragma once

#include "audio.h"

#define STOP 0
#define PLAY 1
#define LOOP 1
#define TRIG 1

struct sampler {
	int         state;
	struct wav *wav;
	size_t      beg;
	size_t      end;
	size_t      cur;
	size_t      loop_beg;
	size_t      loop_end;
	int         loop;
	int         trig;
	float       vol;
};

void sampler_init(struct sampler *s, struct wav *wav, int loop, int trig);
sample step_sampler(struct sampler *s);
