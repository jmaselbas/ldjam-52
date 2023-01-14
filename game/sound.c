#include <stdio.h>
#include "sound.h"
#include "hrtf.h"

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

static void
sound_get_azim_elev(struct sound *s, struct camera *c, float *az, float *el, float *d)
{
	vec3 v;
	quaternion q;
	v = vec3_sub((c->position),(s->pos));
	q = c->rotation;
	quaternion r = { v, 0 };
	quaternion qc = quaternion_conjugate(q);
	r = quaternion_mult(quaternion_mult(qc, r), q);
	*az = atan2(r.v.x, -r.v.z);
	*el = asin(r.v.y/sqrt(POW2(r.v.x) + POW2(r.v.y) + POW2(r.v.z)));
	*d = vec3_norm(v);
}

int
sound_update_hrtf(struct sound *s, struct camera *c)
{
	int sts;
	float az, el, d;
	int a, e;
	sound_get_azim_elev(s, c, &az, &el, &d);
	s->vol = 1. / d;
	a = RAD2DEG(az);
	e = RAD2DEG(el);
	sts = hrtf_get_taps(&a, &e, s->ltaps, s->rtaps);
	return sts;
}

int
sound_step(struct sound *s, struct frame *out)
{
	int ret;
	ret = sampler_step(&s->sampler, 0, NULL, NULL, 0, out);
	return ret;
}

int
sound_pos_step(struct sound *s, struct frame *out)
{
	int ret;
	s->sampler.vol = s->vol;
	ret = sampler_step(&s->sampler, 1, s->ltaps, s->rtaps, HRTF_48_TAPS, out);
	return ret;
}

int
sound_get_frame(struct sound *s, struct frame *out)
{
	if (sound_is_positional(s))
		return sound_pos_step(s, out);
	else
		return sound_step(s, out);
}
