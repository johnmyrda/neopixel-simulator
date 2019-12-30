#ifndef __WS2812_H__
#define __WS2812_H__

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct rgb_pixel_t {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} rgb_pixel_t;

typedef struct ws2812_t LedStrip;

typedef void (* latch_callback_t)(const LedStrip * const strip);

LedStrip * ws2812_init(uint32_t strip_length, latch_callback_t cb);

void ws2812_run(LedStrip * const led_strip, uint64_t time, uint32_t value);

_Bool ws2812_colors_changed(const LedStrip * const led_strip);

rgb_pixel_t * ws2812_get_pixels(const LedStrip * const led_strip);

uint32_t ws2812_get_length(const LedStrip * const led_strip);

uint64_t ws2812_get_last_pin_change_time(const LedStrip * const led_strip);

void ws2812_pixel_to_hex(const rgb_pixel_t * const pixel, char * buffer);

void ws2812_destroy(LedStrip * const led_strip);

#endif 
