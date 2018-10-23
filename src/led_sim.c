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
#if __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <pthread.h>

#include "sim_avr.h"
#include "avr_ioport.h"
#include "sim_elf.h"
#include "sim_gdb.h"
#include "sim_vcd_file.h"

#include "ws2812.h"
#include "button.h"

button_t button;
ws2812_t led_strip;
int do_button_press = 0;
pthread_mutex_t avr_mutex;
avr_t * avr = NULL;
avr_vcd_t vcd_file;
uint8_t	pin_state = 0;	// current port B

float pixsize = 64;
int window;

/*
 * called when the AVR change any of the pins on port B
 * so lets update our buffer
 */
void pin_changed_hook(struct avr_irq_t * irq, uint32_t value, void * param)
{
	pin_state = (pin_state & ~(1 << irq->irq)) | (value << irq->irq);
}

void ws2811_changed_hook(struct avr_irq_t * irq, uint32_t value, void * param){
	printf("Value: %d\n", value);
}


void displayCB(void)		/* function called whenever redisplay needed */
{
	// OpenGL rendering goes here...
	glClear(GL_COLOR_BUFFER_BIT);

	// Set up modelview matrix
	glMatrixMode(GL_MODELVIEW); // Select modelview matrix
	glLoadIdentity(); // Start with an identity matrix

	float grid = pixsize;
	float size = grid * 0.8;
    glBegin(GL_QUADS);
	glColor3f(1,0,0);

	for (int di = 0; di < 8; di++) {
		char on = (pin_state & (1 << di)) != 0;
		if (on) {
			float x = (di) * grid;
			float y = 0; //(si * grid * 8) + (di * grid);
			glVertex2f(x + size, y + size);
			glVertex2f(x, y + size);
			glVertex2f(x, y);
			glVertex2f(x + size, y);
		}
	}

    glEnd();
    glutSwapBuffers();
    //glFlush();				/* Complete any pending operations */
}

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
		case 'r':
			printf("Starting VCD trace\n");
			avr_vcd_start(&vcd_file);
			break;
		case 's':
			printf("Stopping VCD trace\n");
			avr_vcd_stop(&vcd_file);
			break;
	}
}

// gl timer. if the pin have changed states, refresh display
void timerCB(int i)
{
	static uint8_t oldstate = 0xff;
	// restart timer
	glutTimerFunc(1000/64, timerCB, 0);

	if (oldstate != pin_state) {
		oldstate = pin_state;
		glutPostRedisplay();
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

void pixels_done_hook(const rgb_pixel_t * pixels, const uint32_t strip_length, const uint64_t time_in_ns){
        printf("time: %ldns\n", time_in_ns);
}


int main(int argc, char *argv[])
{
	elf_firmware_t f;

	const char * fname;
	if(argc == 1){
		printf("Usage: %s firmware.elf\n", argv[0]);
		exit(1);	
	} else {
		fname = argv[1];
	}
	
	elf_read_firmware(fname, &f);

	f.frequency = 16000000;
	strncat(f.mmcu, fname, sizeof(f.mmcu)-1);
	printf("firmware %s f=%d mmcu=%s\n", fname, (int)f.frequency, f.mmcu);

	avr = avr_make_mcu_by_name("atmega328p");
	if (!avr) {
		fprintf(stderr, "%s: AVR '%s' not known\n", argv[0], f.mmcu);
		exit(1);
	}
	avr_init(avr);
	avr_load_firmware(avr, &f);

	

	// initialize our 'peripheral'
	button_init(avr, &button, "button");
        uint32_t NUM_LEDS = 100;
	rgb_pixel_t pixels[NUM_LEDS];
        ws2812_init(avr, &led_strip, pixels, NUM_LEDS, pixels_done_hook);
        // "connect" the output irw of the button to the port pin of the AVR
	avr_connect_irq(
		button.irq + IRQ_BUTTON_OUT,
		avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('C'), 0));

	// connect all the pins on port B to our callback
	/*for (int i = 0; i < 8; i++)
		avr_irq_register_notify(
			avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('B'), i),
			pin_changed_hook, 
			NULL);*/
	/*avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('B'), IOPORT_IRQ_PIN3), 
                                ws2811_changed_hook, 
                                NULL);*/
        avr_connect_irq(avr_io_getirq(avr,AVR_IOCTL_IOPORT_GETIRQ('B'), IOPORT_IRQ_PIN3), led_strip.irq);

	// even if not setup at startup, activate gdb if crashing
	avr->gdb_port = 1234;
	if (0) {
		//avr->state = cpu_Stopped;
		avr_gdb_init(avr);
	}

	/*
	 *	VCD file initialization
	 *	
	 *	This will allow you to create a "wave" file and display it in gtkwave
	 *	Pressing "r" and "s" during the demo will start and stop recording
	 *	the pin changes
	 */
	avr_vcd_init(avr, "gtkwave_output.vcd", &vcd_file, 100000 /* usec */);
	avr_vcd_add_signal(&vcd_file, 
		avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('B'), IOPORT_IRQ_PIN3), 1 /* bits */ ,
		"led_out" );
	avr_vcd_add_signal(&vcd_file, 
		button.irq + IRQ_BUTTON_OUT, 1 /* bits */ ,
		"button" );

	// 'raise' it, it's a "pullup"
	avr_raise_irq(button.irq + IRQ_BUTTON_OUT, 1);

	printf( "Demo launching: 'LED' bar is PORTB, updated every 1/64s by the AVR\n"
			"   firmware using a timer. If you press 'space' this presses a virtual\n"
			"   'button' that is hooked to the virtual PORTC pin 0 and will\n"
			"   trigger a 'pin change interrupt' in the AVR core, and will 'invert'\n"
			"   the display.\n"
			"   Press 'q' to quit\n\n"
			"   Press 'r' to start recording a 'wave' file\n"
			"   Press 's' to stop recording\n"
			);

	/*
	 * OpenGL init, can be ignored
	 */
//	glutInit(&argc, argv);		/* initialize GLUT system */
//
//	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
//	glutInitWindowSize(8 * pixsize, 1 * pixsize);		/* width=400pixels height=500pixels */
//	window = glutCreateWindow("Glut");	/* create window */


//	glLoadIdentity(); // Start with an identity matrix
//	glOrtho(0, 8 * pixsize, 0, 1 * pixsize, 0, 10);
//	glScalef(1,-1,1);
//	glTranslatef(0, -1 * pixsize, 0);

//	glutDisplayFunc(displayCB);		/* set window's display callback */
//	glutKeyboardFunc(keyCB);		/* set window's key callback */
//	glutTimerFunc(1000 / 24, timerCB, 0);

	// the AVR run on it's own thread. it even allows for debugging!

	avr_vcd_start(&vcd_file);	

	pthread_t run;
	pthread_create(&run, NULL, avr_run_thread, NULL);

	uint64_t time_nsec = 0;
	while(time_nsec < 400000000){
		pthread_mutex_lock(&avr_mutex);
		time_nsec = avr_cycles_to_nsec(avr, avr->cycle);
		pthread_mutex_unlock(&avr_mutex);
		//printf("a ");
	}

	printf("time stopped at %ldns\n", time_nsec);
	avr_vcd_stop(&vcd_file);
	exit(0);
//	glutMainLoop();

}
