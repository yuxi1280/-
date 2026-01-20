#ifndef __WS2812_H_
#define __WS2812_H_

#include "system_hc32f460.h"

typedef struct
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
}ws2812_rgb_value;

void ws2812_init(void);
void ws2812_set_rgb(uint8_t r, uint8_t g, uint8_t b);
void ws2812_reset(void);
void ws2812_idle(void);

#endif


