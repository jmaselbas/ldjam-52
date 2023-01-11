#include <stdint.h>
#include <stddef.h>
#include "sampler.h"
#include "wav.h"

void
sampler_init(struct sampler *s, struct wav *wav, int loop, int autoplay)
{
	s->wav      = wav;
	s->state    = STOP;
	s->beg      = 0;
	s->end      = wav->extras.nb_samples;
	s->cur      = s->beg;
	if (loop)
		s->loop = 1;
	else
		s->loop = 0;
	s->loop_beg = 0;
	s->loop_end = s->end;
	if (autoplay)
		s->trig = 1;
	else
		s->trig = 0;
	s->vol = 1;
}

static int
pb_fini(struct sampler *s)
{
	return (s->cur >= s->wav->extras.nb_samples);
}

static int
is_retrigged(struct sampler *s)
{
	return (s->trig);
}

sample
step_sampler(struct sampler *s)
{
	int16_t *samples;
	float vol;
	size_t off;
	sample ret = 0;

	if (is_retrigged(s)) {
		s->trig = 0;
		switch (s->state) {
			case PLAY:
				s->cur = s->beg;
				break;
			case STOP:
				s->state = PLAY;
				break;
		}
	}

	if(pb_fini(s)) {
		if (s->loop) {
			s->cur = s->loop_beg;
		} else {
			s->state = STOP;
			s->cur = s->beg;
		}
	}

	switch(s->state){
	case STOP:
		ret = 0;
		break;
	case PLAY:
		off = s->cur++;
		vol = s->vol;
		samples = (int16_t *) s->wav->audio_data;
		ret = vol * (float) (samples[off] / (float) INT16_MAX);
		break;
	}
	return ret;
}
