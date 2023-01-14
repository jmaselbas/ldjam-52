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

static int
nb_channels(struct sampler *s)
{
	return (s->wav->header.channels);
}

static int
is_stereo(struct sampler *s)
{
	return (nb_channels(s) == 2);
}

/* returns the position of the next frame of audio data */
static size_t
sampler_step_pos(struct sampler *s)
{
	size_t ret = 0;

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
	ret = s->cur;
	s->cur++;
	if (is_stereo(s))
		s->cur++;
	return ret;
}

int
sampler_step(struct sampler *s, int convolve, int16_t *firl, int16_t *firr, int fir_size, struct frame *out)
{
	size_t pos;
	float vol;
	int16_t *samples;
	int i, j;
	int16_t no_convolution;
	int16_t inc;

	if (!out)
		return 1;

	no_convolution = INT16_MAX;
	if (!convolve) {
		firl = &no_convolution;
		firr = &no_convolution;
		fir_size = 1;
	} else {
		if (!firl || !firr)
			return 1;
	}

	inc = is_stereo(s);
	pos = sampler_step_pos(s);

	out->l = 0;
	out->r = 0;
	switch(s->state){
	case STOP:
		break;
	case PLAY:
		vol = s->vol;
		samples = (int16_t *) s->wav->audio_data;
		for (i = 0; i < fir_size; i++) {
			j = pos - (i * (inc + 1));
			if (j >= 0) {
				out->l += (float) firl[i]/INT16_MAX * (float) samples[j]/INT16_MAX;
				out->r += (float) firr[i]/INT16_MAX * (float) samples[j + inc]/INT16_MAX;
			}
		}
		out->l *= vol;
		out->r *= vol;
		break;
	}
	return 0;
}
