#pragma once

struct text_entry {
	struct font *font;
	const char *text;
	size_t len;
	vec3 color;
	vec3 position;
	quaternion rotation;
	vec3 scale;
	float fx;
};

void sys_text_init(struct memory_zone zone);
void sys_text_push(struct text_entry *);
void sys_text_exec(void);

void sys_text_puts(vec2 at, vec3 color, char *s);
void sys_text_printf(vec2 at, vec3 color, char *fmt, ...);

