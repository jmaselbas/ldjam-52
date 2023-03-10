#pragma once

void gui_begin(void *ctx);
size_t gui_size(void);
struct gui_state *gui_init(void *base);
void gui_draw(void);
void gui_text(int x, int y, const char *s, uint8_t col);
uint8_t gui_color(uint8_t r, uint8_t g, uint8_t b);
void gui_fill(int x, int y, unsigned int w, unsigned int h, uint8_t c);

#include <stdarg.h>
#include <stdio.h>
static inline void gui_printf(int x, int y, char *fmt, ...)
{
	char buf[256];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	gui_text(x, y, buf, gui_color(255,255,255));
}
