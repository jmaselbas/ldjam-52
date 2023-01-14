#include <stdint.h>
#include <stdio.h>
#include "hrtf.h"
#include "fir.h"
#include "core/engine.h"

/*
 * This file is Based on the work of
 * Bill Gardner, Keith Martin and Mike Casey.
 * KEMAR HRTF data sets and 3Daudio Demonstration Program.
 * KEMAR HRTF data is Copyright 1994 by the MIT Media Laboratory.
 * provided free with no restrictions on use,
 * provided the authors are cited when the data is used in any research
 * or commercial application.
 * https://sound.media.mit.edu/resources/KEMAR.html
 *
 * AND
 *
 * the mit-hrtf-lib which is the work of Aristotel Digenis
 * Copyright (c) 2014 greekgoddj
 * Distributed under MIT licence
 * https://github.com/greekgoddj
 */

static int
hrtf_get_nearest_elev(int e)
{
	int ret;
	e = CAP(e, -40, 90);
	ret = ((ABS(e) + 5) / 10) * 10;
	if (e < 0)
		ret = -ret;
	return ret;
}

static const int hrtf_azi_positions[] = {
	[0] = HRTF_AZI_POSITIONS_0,
	[1] = HRTF_AZI_POSITIONS_10,
	[2] = HRTF_AZI_POSITIONS_20,
	[3] = HRTF_AZI_POSITIONS_30,
	[4] = HRTF_AZI_POSITIONS_40,
	[5] = HRTF_AZI_POSITIONS_50,
	[6] = HRTF_AZI_POSITIONS_60,
	[7] = HRTF_AZI_POSITIONS_70,
	[8] = HRTF_AZI_POSITIONS_80,
	[9] = HRTF_AZI_POSITIONS_90,
};

static int
hrtf_get_nearest_azim(int e, int a)
{
	int da;
	a = CAP(a, -180, 180);
	if(e == 50)
		a = CAP(a, -176, 176);

	da = 180.f / (hrtf_azi_positions[ABS(e)/10] - 1);
	a = (int)((int)((ABS(a) + da / 2.f) / da) * da + 0.5f);
	return a;
}

static int
hrtf_get_azim_index(int e, int a)
{
	float q = 180./hrtf_azi_positions[ABS(e)/10];
	int ret;
	a = CAP(a, 0, 180);
	ret = a/q;
	return ret;
}

static struct hrtf_48 * hrtf_el_set[] = {
	[0] = normal_48.e00,
	[1] = normal_48.e10,
	[2] = normal_48.e20,
	[3] = normal_48.e30,
	[4] = normal_48.e40,
	[5] = normal_48.e50,
	[6] = normal_48.e60,
	[7] = normal_48.e70,
	[8] = normal_48.e80,
	[9] = normal_48.e90,
	[11] = normal_48.e_10,
	[12] = normal_48.e_20,
	[13] = normal_48.e_30,
	[14] = normal_48.e_40,
};

static void
_hrtf_get_taps(int e, int a, int16_t **ltaps, int16_t **rtaps)
{
	int ai = hrtf_get_azim_index(e, a);

	if (e < 0) {
		*ltaps = hrtf_el_set[10 + (ABS(e) / 10)][ai].left;
		*rtaps = hrtf_el_set[10 + (ABS(e) / 10)][ai].right;
	} else {
		*ltaps = hrtf_el_set[e/10][ai].left;
		*rtaps = hrtf_el_set[e/10][ai].right;
	};
}

int
hrtf_get_taps(int* azimuth, int* elevation, int16_t* pLeft, int16_t* pRight)
{
	int a = *azimuth;
	int e = *elevation;
	int16_t* ltaps = 0;
	int16_t* rtaps = 0;

	e = hrtf_get_nearest_elev(e);
	a = hrtf_get_nearest_azim(e, a);

	_hrtf_get_taps(e, a, &ltaps, &rtaps);

	if(!ltaps || !rtaps)
		return 1;

	if(*azimuth < 0) {
		int16_t* tmp = rtaps;
		rtaps = ltaps;
		ltaps = tmp;
	}

	for(int i= 0; i < HRTF_48_TAPS; i++) {
		pLeft[i] = ltaps[i];
		pRight[i] = rtaps[i];
	}

	*azimuth = a;
	*elevation = e;

	return 0;
}

