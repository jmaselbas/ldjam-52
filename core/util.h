#pragma once

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define ABS(a) ((a) > 0 ? (a) : -(a))
#define CAP(x, a, b) ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))
#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))
#define RAD2DEG(a) (180.f/3.14 * (a))
#define POW2(a) ((a) * (a))

void warn(const char *fmt, ...);
void die(const char *fmt, ...);

#define SZ_1M		0x00100000
#define SZ_2M		0x00200000
#define SZ_4M		0x00400000
#define SZ_8M		0x00800000
#define SZ_16M		0x01000000
#define SZ_256M		0x10000000

struct memory_zone {
	void  *base;
	size_t size;
	size_t used;
};
struct memory_zone memory_zone_init(void *base, size_t size);

void *mempush(struct memory_zone *zone, size_t size);
void  mempull(struct memory_zone *zone, size_t size);

