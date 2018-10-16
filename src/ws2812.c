#include "ws2812.h"
#include <stdlib.h>

//timing info in ns
//https://wp.josh.com/2014/05/13/ws2812-neopixels-are-not-so-finicky-once-you-get-to-know-them/
timing_t T0H = {200, 500};
timing_t T1H = {550, 5500};
timing_t TLD = {300, 5000};
timing_t TLL = {6000, UINT16_MAX};

void ws2812_pin_changed_hook(struct avr_irq_t * irq, uint32_t value, void *param){
    ws2812_t *led_metadata = (ws2812_t *) param;
    uint64_t time_nsec = avr_cycles_to_nsec(led_metadata->avr, led_metadata->avr->cycle);
//    printf("Pin state: %d\n", value);
    ws2812_run(time_nsec, value, led_metadata);
}

void ws2812_init(struct avr_t * avr, ws2812_t * led_metadata){
    *led_metadata = (struct ws2812_t) {0};

    led_metadata->avr = avr;
    led_metadata->irq = avr_alloc_irq(&avr->irq_pool, 0, 1, NULL);
    avr_irq_register_notify(led_metadata->irq, ws2812_pin_changed_hook, led_metadata);

}

void ws2812_high(uint64_t time, ws2812_t * led_metadata){
	//printf("hiiiii time=%ld", time);

	if(T0H.low < time && time < T0H.high){
		led_metadata->cur_bit = 0;
		//printf(" one\n");
	} else 
	if (T1H.low < time && time < T1H.high){
		led_metadata->cur_bit = 1;
		//printf(" zero\n");
	} else {
		led_metadata->cur_bit = 1;
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
			rgb_pixel_t * pixel = &led_metadata->pixel;
			switch(led_metadata->cur_byte_index % 3)
			{
				case 0:
					pixel->green = led_metadata->cur_byte;
					break;
				case 1:
					pixel->red   = led_metadata->cur_byte;
					break;
				case 2:
					pixel->blue = led_metadata->cur_byte;
					printf("R,G,B: %d,%d,%d\n", pixel->red,pixel->green,pixel->blue);
					break;
			}
			led_metadata->cur_byte = 0;
			led_metadata->cur_byte_index++;
		}
	} else
	if(TLL.low < time){
		led_metadata->cur_byte = (led_metadata->cur_bit << led_metadata->cur_bit_index) | led_metadata->cur_byte;
		printf("latched. byte value: %d\n", led_metadata->cur_byte);
		led_metadata->cur_byte = 0;
		led_metadata->cur_bit_index = 0;
		led_metadata->cur_byte_index = 0;
	}
}


void ws2812_run(uint64_t time, uint32_t value, ws2812_t * led_metadata){
    uint64_t diff = time - led_metadata->last_pin_change_time;
    led_metadata->last_pin_change_time = time;
    if(value == 0){
        ws2812_high(diff, led_metadata);
    } else 
    if(value == 1){
        ws2812_low(diff, led_metadata);
    }
//    printf("diff: %ld time: %ld ns\n", diff, time);
}
