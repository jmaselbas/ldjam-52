#pragma once
#include <stddef.h>

typedef float sample;

struct frame {
	sample l;
	sample r;
};

struct audio {
	size_t size; /* in frames */
	struct frame *buffer;
};
