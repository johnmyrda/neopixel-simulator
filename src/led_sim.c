/*
	ledramp.c
	
	Copyright 2008, 2009 Michel Pollet <buserror@gmail.com>

 	This file is part of simavr.

	simavr is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	simavr is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with simavr.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "sim_avr.h"
#include "avr_ioport.h"
#include "sim_elf.h"
#include "sim_gdb.h"

#include "ws2812.h"
#include "button.h"

button_t button;
ws2812_t led_strip;
int do_button_press = 0;
pthread_mutex_t avr_mutex;
avr_t * avr = NULL;
uint8_t	pin_state = 0;	// current port B

FILE * fp;//file descriptor for named pipe

void keyCB(unsigned char key, int x, int y)	/* called on key press */
{
	if (key == 'q')
		exit(0);
	//static uint8_t buf[64];
	switch (key) {
		case 'q':
		case 0x1f: // escape
			exit(0);
			break;
		case ' ':
			do_button_press++; // pass the message to the AVR thread
			break;
	}
}

static void * avr_run_thread(void * oaram)
{
	int b_press = do_button_press;
	
	while (1) {
		pthread_mutex_lock(&avr_mutex);
		avr_run(avr);
		pthread_mutex_unlock(&avr_mutex);
		if (do_button_press != b_press) {
			b_press = do_button_press;
			printf("Button pressed\n");
			button_press(&button, 1000000);
		}
	}
	return NULL;
}


//debug function to print truecolor rgb in terminal
void pixels_to_truecolor(const rgb_pixel_t * pixels, uint32_t strip_length){
        const char filled_square_format_string[] = "\x1b[47m\x1b[38;2;%d;%d;%dm\u25A0 \x1b[0m";
        const char empty_square_string[]         = "\x1b[47m\x1b[38;2;0;0;0m\u25A1 \x1b[0m";

        printf("strip_length: %d, hex: ", strip_length);
        for(int i=0; i < strip_length; i++){
                uint32_t hex_value = pixels[i].red << 16 | pixels[i].green << 8 | pixels[i].blue;
                printf("#%06x ",hex_value);         
        }
        printf("\n");

        for(int i=0; i < strip_length; i++){
                rgb_pixel_t p = pixels[i];
                if(p.red != 0 || p.green != 0 || p.blue != 0){
                        printf(filled_square_format_string,p.red,p.blue,p.green);
                } else {
                        printf(empty_square_string);       
                }
        }
        printf("\n");
}

void pixels_done_hook(const rgb_pixel_t * pixels, const uint32_t strip_length, const uint64_t time_in_ns){
        //printf("time: %ldns\n", time_in_ns);
        //pixels_to_truecolor(pixels, strip_length);
        int time_write = fwrite(&time_in_ns, sizeof(time_in_ns), 1, fp);        
        int length_write = fwrite(&strip_length, sizeof(strip_length), 1, fp);
        int pixel_write = fwrite(pixels, sizeof(pixels[0]), strip_length, fp);
        if(length_write < 0 || pixel_write < 0 || time_write < 0){
                printf("write error!");
                }
        fflush(fp);
}

struct arg_struct{
        char *file_name;
        char *pipe_name;
        uint64_t sim_time;
        uint64_t sim_time_ns;
        };

void usage(){
    fprintf(stdout, "usage: led_sim -t 10 -f firmware.elf\n");
    fprintf(stdout, "  -t <time in seconds> default: 10\n");
    fprintf(stdout, "  -n <time in nanoseconds> note: superceded by -t\n");
    fprintf(stdout, "  -f <file to simulate>\n");
    fprintf(stdout, "  -p <name of fifo pipe>\n");
    exit(2);
}

struct arg_struct parse_args(int argc, char *argv[]){
   //default arguments
    struct arg_struct arguments;
    // initialize struct members
    arguments.sim_time = 0;
    arguments.sim_time_ns = 1000000000;
    arguments.pipe_name = "/tmp/neopixel";
    arguments.file_name = "";

    int opt;
    while ((opt = getopt(argc, argv, ":n:t:f:p:h")) != -1){
        switch(opt) {
            case 'n': // simulation time in nanoseconds
                arguments.sim_time_ns = strtoull(optarg, NULL, 10);
                break;
            case 't': // simulation time in seconds
                arguments.sim_time = strtoull(optarg, NULL, 10);
                break;
            case 'f': // filename of file to simulate
                arguments.file_name = optarg;
                break;
            case 'p': // name of pipe to use
                arguments.pipe_name = optarg;
                break;
            case ':':
                fprintf(stderr, "Option -%c requires an operand\n", optopt);
                usage();
            case 'h':
            default:
                usage();
        }
    }

    if(arguments.sim_time != 0){
            arguments.sim_time_ns = arguments.sim_time*1000000000;
    }

    if( access(arguments.file_name, F_OK) < 0){
        printf("Could not find file %s\n", arguments.file_name);
        usage();
        exit(1);
    }

    return arguments;
}

int main(int argc, char *argv[])
{
        struct arg_struct arguments = parse_args(argc, argv);

        elf_firmware_t f;
        elf_read_firmware(arguments.file_name, &f);

        f.frequency = 16000000;
        strncat(f.mmcu, arguments.file_name, sizeof(f.mmcu)-1);
        printf("firmware %s f=%d mmcu=%s\n", arguments.file_name, (int)f.frequency, f.mmcu);
        printf("sim_time=%llu sim_time_ns=%llu\n", arguments.sim_time, arguments.sim_time_ns);

	avr = avr_make_mcu_by_name("atmega328p");
	if (!avr) {
		fprintf(stderr, "%s: AVR '%s' not known\n", argv[0], f.mmcu);
		exit(1);
	}
	avr_init(avr);
	avr_load_firmware(avr, &f);

	// initialize our 'peripheral'
	button_init(avr, &button, "button");
        uint32_t NUM_LEDS = 32;
	rgb_pixel_t pixels[NUM_LEDS];
        ws2812_init(avr, &led_strip, pixels, NUM_LEDS, pixels_done_hook);
        // "connect" the output irw of the button to the port pin of the AVR
	avr_connect_irq(
		button.irq + IRQ_BUTTON_OUT,
		avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('C'), 0));

        avr_connect_irq(avr_io_getirq(avr,AVR_IOCTL_IOPORT_GETIRQ('B'), IOPORT_IRQ_PIN3), led_strip.irq);

	// even if not setup at startup, activate gdb if crashing
	avr->gdb_port = 1234;
	if (0) {
		//avr->state = cpu_Stopped;
		avr_gdb_init(avr);
	}

	// 'raise' it, it's a "pullup"
	avr_raise_irq(button.irq + IRQ_BUTTON_OUT, 1);

	// create a named pipe
        int remove_error = remove(arguments.pipe_name);
        if(remove_error != 0){
                printf("can't remove %s\n", arguments.pipe_name);
                perror("error:");
                }
        int mkfifo_error = mkfifo(arguments.pipe_name, 0666);
        if(mkfifo_error != 0){
                perror("mkfifo failed");
                }
        printf("using named pipe: %s\n", arguments.pipe_name);
        fp = fopen(arguments.pipe_name, "w");
        
        // the AVR run on it's own thread. it even allows for debugging!
	pthread_t run;
	pthread_create(&run, NULL, avr_run_thread, NULL);

	uint64_t time_nsec = 0;
	while(time_nsec < arguments.sim_time_ns){
		pthread_mutex_lock(&avr_mutex);
		time_nsec = avr_cycles_to_nsec(avr, avr->cycle);
		pthread_mutex_unlock(&avr_mutex);
	}

        fclose(fp);
        
        printf("time stopped at %lluns\n", time_nsec);
        exit(0);

}
