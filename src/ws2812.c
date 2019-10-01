#include "ws2812.h"
#include <stdlib.h>

typedef struct timing_t
{
    uint16_t low;
    uint16_t high;
} timing_t;

//timing info in ns
//https://wp.josh.com/2014/05/13/ws2812-neopixels-are-not-so-finicky-once-you-get-to-know-them/
const timing_t T0H = {200, 500};
const timing_t T1H = {550, 5500};
const timing_t TLD = {300, 5000};
const timing_t TLL = {6000, UINT16_MAX};

typedef struct ws2812_t
{
    uint64_t last_pin_change_time;
    latch_callback_t latch_callback;
    rgb_pixel_t * pixels;
    uint32_t strip_length;
    uint8_t cur_bit;
    uint8_t cur_bit_index;
    uint8_t cur_byte;
    uint8_t cur_byte_index;
} ws2812_t;

LedStrip * ws2812_init(uint32_t strip_length, latch_callback_t cb){
    LedStrip * strip = malloc(sizeof(LedStrip));
    rgb_pixel_t * pixels = malloc(sizeof(rgb_pixel_t) * strip_length);

    memset(strip, 0, sizeof(LedStrip));
    memset(pixels, 0, sizeof(rgb_pixel_t)*strip_length);
    strip->pixels = pixels;
    strip->strip_length = strip_length;
    strip->latch_callback = cb;

    return strip;
}

_Bool ws2812_high(uint64_t time){
	if(T0H.low < time && time < T0H.high){
        return 0;
	} else 
	if (T1H.low < time && time < T1H.high){
        return 1;
	} else {
		return 1;
	}
}

void ws2812_low(uint64_t time, ws2812_t * led_metadata){
    if(TLD.low < time && time < TLD.high){
        //set bit using bitmask
        led_metadata->cur_byte = (led_metadata->cur_bit << (7 - led_metadata->cur_bit_index)) | led_metadata->cur_byte;
        led_metadata->cur_bit_index++;
        //printf(" set bit %d\n", led_metadata->cur_bit);
        if(led_metadata->cur_bit_index >= 8){
            led_metadata->cur_bit_index = 0;
            //printf("byte value: %d\n", led_metadata->cur_byte);
                        if(led_metadata->cur_byte_index < led_metadata->strip_length*sizeof(rgb_pixel_t)){
                                //cast the pixels to a uint8_t pointer so i can count by bytes
                                ((uint8_t *)led_metadata->pixels)[led_metadata->cur_byte_index] = led_metadata->cur_byte;
                                //printf("byte index: %d pixel size %ld\n", led_metadata->cur_byte_index, sizeof(rgb_pixel_t));
                                }
            led_metadata->cur_byte = 0;
            led_metadata->cur_byte_index++;
        }
    } else
    if(TLL.low < time && led_metadata->cur_byte_index){
        led_metadata->cur_byte = (led_metadata->cur_bit << led_metadata->cur_bit_index) | led_metadata->cur_byte;
        //printf("latched.");
        led_metadata->latch_callback(led_metadata->pixels, led_metadata->cur_byte_index / sizeof(rgb_pixel_t), led_metadata->last_pin_change_time);
        led_metadata->cur_byte = 0;
        led_metadata->cur_bit_index = 0;
        led_metadata->cur_byte_index = 0;
    }
}


void ws2812_run(uint64_t time, uint32_t value, ws2812_t * led_metadata){
    uint64_t diff = time - led_metadata->last_pin_change_time;
    led_metadata->last_pin_change_time = time;
    if(value == 0){
        led_metadata->cur_bit = ws2812_high(diff);
    } else 
    if(value == 1){
        ws2812_low(diff, led_metadata);
    }
//    printf("diff: %ld time: %ld ns\n", diff, time);
}

void ws2812_destroy(LedStrip *led_strip){
    free(led_strip->pixels);
    free(led_strip);
}
