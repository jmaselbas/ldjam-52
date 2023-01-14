#pragma once

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

#define HRTF_AZI_POSITIONS_0	37
#define HRTF_AZI_POSITIONS_10	37
#define HRTF_AZI_POSITIONS_20	37
#define HRTF_AZI_POSITIONS_30	31
#define HRTF_AZI_POSITIONS_40	29
#define HRTF_AZI_POSITIONS_50	23
#define HRTF_AZI_POSITIONS_60	19
#define HRTF_AZI_POSITIONS_70	13
#define HRTF_AZI_POSITIONS_80	7
#define HRTF_AZI_POSITIONS_90	1

#define HRTF_48_TAPS	140


struct hrtf_48
{
	int16_t left[HRTF_48_TAPS];
	int16_t right[HRTF_48_TAPS];
};

struct hrtf_set_48
{
	struct hrtf_48 e_10[HRTF_AZI_POSITIONS_10];
	struct hrtf_48 e_20[HRTF_AZI_POSITIONS_20];
	struct hrtf_48 e_30[HRTF_AZI_POSITIONS_30];
	struct hrtf_48 e_40[HRTF_AZI_POSITIONS_40];
	struct hrtf_48 e00[HRTF_AZI_POSITIONS_0];
	struct hrtf_48 e10[HRTF_AZI_POSITIONS_10];
	struct hrtf_48 e20[HRTF_AZI_POSITIONS_20];
	struct hrtf_48 e30[HRTF_AZI_POSITIONS_30];
	struct hrtf_48 e40[HRTF_AZI_POSITIONS_40];
	struct hrtf_48 e50[HRTF_AZI_POSITIONS_50];
	struct hrtf_48 e60[HRTF_AZI_POSITIONS_60];
	struct hrtf_48 e70[HRTF_AZI_POSITIONS_70];
	struct hrtf_48 e80[HRTF_AZI_POSITIONS_80];
	struct hrtf_48 e90[HRTF_AZI_POSITIONS_90];
};

int hrtf_get_taps(int* azimuth, int* elevation, int16_t* pLeft, int16_t* pRight);

