#ifndef __WS2812_H__
#define __WS2812_H__

#include <stdio.h>
#include <string.h>
#include "sim_irq.h"
#include "sim_time.h"

typedef struct rgb_pixel_t {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} rgb_pixel_t;

typedef void (* latch_callback_t)(const rgb_pixel_t * pixels, const uint32_t strip_length, const uint64_t time_in_ns);

typedef struct ws2812_t LedStrip;

LedStrip * ws2812_init(uint32_t strip_length, latch_callback_t cb);

void ws2812_run(uint64_t time, uint32_t value, LedStrip *led_strip);

void ws2812_low(uint64_t time, LedStrip *led_strip);

_Bool ws2812_high(uint64_t time);

void ws2812_destroy(LedStrip *led_strip);

#endif 
