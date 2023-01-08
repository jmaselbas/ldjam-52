#include <stdint.h>
#include <stddef.h>
#include "sampler.h"
#include "wav.h"

void
sampler_init(struct sampler *sampler, struct wav *wav, enum pb_mode mode, int autoplay)
{
	sampler->wav      = wav;
	sampler->state    = STOP;
	sampler->pb_start = 0;
	sampler->pb_end   = wav->extras.nb_samples;
	sampler->pb_head  = 0;
	if (mode == LOOP)
		sampler->loop_on = 1;
	else
		sampler->loop_on = 0;
	sampler->loop_start = 0;
	sampler->loop_end  = sampler->pb_end;
	if (autoplay)
		sampler->trig_on = 1;
	else
		sampler->trig_on = 0;
	sampler->vol = 1;
}

static int
is_pb_fini(struct sampler *sampler)
{
	return (sampler->pb_head >= sampler->wav->extras.nb_samples);
}

static int
is_retrigged(struct sampler *sampler)
{
	return (sampler->trig_on);
}

sample step_sampler(struct sampler *sampler)
{
	int16_t *samples;
	float vol;
	size_t offset;
	float ret = 0;

	if (is_retrigged(sampler)) {
		sampler->trig_on = 0;
		switch (sampler->state) {
			case PLAY:
				sampler->pb_head = sampler->pb_start;
				break;
			case STOP:
				sampler->state = PLAY;
				break;
		}
	}

	if(is_pb_fini(sampler)) {
		if (sampler->loop_on) {
			sampler->pb_head = sampler->loop_start;
		} else {
			sampler->state = STOP;
			sampler->pb_head = sampler->pb_start;
		}
	}

	switch(sampler->state){
	case STOP:
		ret = 0;
		break;
	case PLAY:
		offset = sampler->pb_head++;
		vol = sampler->vol;
		samples = (int16_t *) sampler->wav->audio_data;
		ret = vol * (float) (samples[offset] / (float) INT16_MAX);
		break;
	}
	return ret;
}
