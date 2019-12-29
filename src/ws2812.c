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

void ws2812_low(LedStrip * const led_metadata, uint64_t time){
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
        led_metadata->latch_callback(led_metadata);
        led_metadata->cur_byte = 0;
        led_metadata->cur_bit_index = 0;
        led_metadata->cur_byte_index = 0;
    }
}


void ws2812_run(LedStrip * const led_metadata, uint64_t time, uint32_t value){
    uint64_t diff = time - led_metadata->last_pin_change_time;
    led_metadata->last_pin_change_time = time;
    if(value == 0){
        led_metadata->cur_bit = ws2812_high(diff);
    } else 
    if(value == 1){
        ws2812_low(led_metadata, diff);
    }
//    printf("diff: %ld time: %ld ns\n", diff, time);
}

_Bool rgb_pixel_compare(rgb_pixel_t pixel_a, rgb_pixel_t pixel_b){
    if( pixel_a.red   == pixel_b.red && 
        pixel_a.green == pixel_b.green &&
        pixel_a.blue  == pixel_b.blue
    ) {
        return 1;
    } else {
        return 0;
    }
}

_Bool ws2812_compare(const LedStrip * const strip_a, const LedStrip * const strip_b){
    if(!strip_a || !strip_b){ // check for null
        return 0;
    }
    if(strip_a->strip_length != strip_b->strip_length){
        return 0;
    }
    for ( int cur_pixel = 0; cur_pixel < strip_a->strip_length; cur_pixel++ ){
        if ( !rgb_pixel_compare(strip_a->pixels[cur_pixel], strip_b->pixels[cur_pixel]) ){
            return 0;
        }
    }
    return 1;
}

rgb_pixel_t * ws2812_get_pixels(const LedStrip * const led_strip){
    return led_strip->pixels;
}

uint32_t ws2812_get_length(const LedStrip * const led_strip){
    return led_strip->strip_length;
}

uint64_t ws2812_get_last_pin_change_time(const LedStrip * const led_strip){
    return led_strip->last_pin_change_time;
}

void ws2812_destroy(LedStrip * const led_strip){
    free(led_strip->pixels);
    free(led_strip);
}
