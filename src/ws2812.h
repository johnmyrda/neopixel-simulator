#ifndef __WS2812_H__
#define __WS2812_H__

#include <stdio.h>
#include "sim_irq.h"
#include "sim_time.h"

typedef struct rgb_pixel_t {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} rgb_pixel_t;

typedef void (* latch_callback_t)(const rgb_pixel_t * pixels, const uint32_t strip_length, const uint64_t time_in_ns);

typedef struct ws2812_t
{
	avr_irq_t * irq;
	struct avr_t * avr;
        uint64_t last_pin_change_time;
        latch_callback_t latch_callback;
        rgb_pixel_t * pixels;
        uint32_t strip_length;
        uint8_t cur_bit;
	uint8_t cur_bit_index;
	uint8_t cur_byte;
	uint8_t cur_byte_index;
} ws2812_t;

typedef struct timing_t
{
	uint16_t low;
	uint16_t high;
} timing_t;

void ws2812_pin_changed_hook(struct avr_irq_t * irq, uint32_t value, void *param);

void ws2812_init(struct avr_t * avr, ws2812_t * led_metadata, rgb_pixel_t * pixels, uint32_t strip_length, latch_callback_t cb);

void ws2812_run(uint64_t time, uint32_t value, ws2812_t * led_metadata);

void ws2812_low(uint64_t time, ws2812_t * led_metadata);

_Bool ws2812_high(uint64_t time);

#endif 
