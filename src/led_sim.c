#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "avr_ioport.h"
#include "sim_avr.h"
#include "sim_elf.h"
#include "sim_gdb.h"

#include "sim_irq.h"
#include "ws2812.h"

FILE *fp; // file descriptor for named pipe
avr_t *avr = NULL;

_Bool print_percentage_flag = 0;
#define NANOSECONDS_PER_SECOND 1000000000

// debug function to print truecolor rgb in terminal
void pixels_to_truecolor(const rgb_pixel_t *pixels, uint32_t strip_length) {
  const char filled_square_format_string[] =
      "\x1b[47m\x1b[38;2;%d;%d;%dm\u25A0 \x1b[0m";
  const char empty_square_string[] = "\x1b[47m\x1b[38;2;0;0;0m\u25A1 \x1b[0m";

  printf("strip_length: %d, hex: ", strip_length);
  for (int i = 0; i < strip_length; i++) {
    uint32_t hex_value =
        pixels[i].red << 16 | pixels[i].green << 8 | pixels[i].blue;
    printf("#%06x ", hex_value);
  }
  printf("\n");

  for (int i = 0; i < strip_length; i++) {
    rgb_pixel_t p = pixels[i];
    if (p.red != 0 || p.green != 0 || p.blue != 0) {
      printf(filled_square_format_string, p.red, p.blue, p.green);
    } else {
      printf(empty_square_string);
    }
  }
  printf("\n");
}

void ws2812_pin_changed_hook(struct avr_irq_t * irq, uint32_t value, void *param){
    uint64_t time_nsec = avr_cycles_to_nsec(avr, avr->cycle);
    ws2812_run(param, time_nsec, value);
}

void pixels_done_hook(const rgb_pixel_t *pixels, const uint32_t strip_length,
                      const uint64_t time_in_ns) {
  // printf("time: %ldns\n", time_in_ns);
  // pixels_to_truecolor(pixels, strip_length);
  int time_write = fwrite(&time_in_ns, sizeof(time_in_ns), 1, fp);
  int length_write = fwrite(&strip_length, sizeof(strip_length), 1, fp);
  int pixel_write = fwrite(pixels, sizeof(pixels[0]), strip_length, fp);
  if (length_write < 0 || pixel_write < 0 || time_write < 0) {
    printf("write error!");
  }
  fflush(fp);
}

void percentage_signal_handler(){
  print_percentage_flag = 1;
}

void print_percentage(avr_cycle_count_t current_cycle, avr_cycle_count_t end_cycle){
      printf("%.2f%% cycle=%llu\n", 100.0 * current_cycle / end_cycle, current_cycle);
      print_percentage_flag = 0;
      alarm(1);
}

// converts a number of nsec to a number of machine cycles, at current speed
static inline avr_cycle_count_t
avr_nsec_to_cycles(struct avr_t * avr, uint64_t nsec)
{
	return avr->frequency * (avr_cycle_count_t)nsec / NANOSECONDS_PER_SECOND;
}

void remove_fifo(const char * fifo_name){
  int remove_error = remove(fifo_name);
  if (remove_error != 0) {
    printf("can't remove %s\n", fifo_name);
    perror("error:");
  }
}

struct arg_struct {
  char *file_name;
  char *pipe_name;
  uint64_t sim_time_ns;
};

void usage() {
  fprintf(stdout, "usage: led_sim -t 10 -f firmware.elf\n");
  fprintf(stdout, "  -t <time in seconds> default: 5\n");
  fprintf(stdout, "  -n <time in nanoseconds> note: superceded by -t\n");
  fprintf(stdout, "  -f <file to simulate>\n");
  fprintf(stdout, "  -p <name of fifo pipe>\n");
  exit(2);
}

struct arg_struct parse_args(int argc, char *argv[]) {
  // default arguments
  struct arg_struct arguments = {0};
  // initialize struct members
  uint64_t sim_time = 0;
  arguments.sim_time_ns = NANOSECONDS_PER_SECOND * (uint64_t) 5;
  arguments.pipe_name = "/tmp/neopixel";
  arguments.file_name = "";

  int opt;
  while ((opt = getopt(argc, argv, ":n:t:f:p:h")) != -1) {
    switch (opt) {
    case 'n': // simulation time in nanoseconds
      arguments.sim_time_ns = strtoull(optarg, NULL, 10);
      break;
    case 't': // simulation time in seconds
      sim_time = strtoull(optarg, NULL, 10);
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

  if (sim_time != 0) {
    arguments.sim_time_ns = sim_time * NANOSECONDS_PER_SECOND;
  }

  if (access(arguments.file_name, F_OK) < 0) {
    printf("Could not find file %s\n", arguments.file_name);
    usage();
    exit(1);
  }

  return arguments;
}

int main(int argc, char *argv[]) {
  struct arg_struct arguments = parse_args(argc, argv);

  elf_firmware_t f;
  elf_read_firmware(arguments.file_name, &f);

  f.frequency = 16000000;
  strncat(f.mmcu, arguments.file_name, sizeof(f.mmcu) - 1);
  printf("firmware %s f=%d mmcu=%s\n", arguments.file_name, (int)f.frequency,
         f.mmcu);
  printf("sim_time_ns=%llu\n", arguments.sim_time_ns);

  avr = avr_make_mcu_by_name("atmega328p");
  if (!avr) {
    fprintf(stderr, "%s: AVR '%s' not known\n", argv[0], f.mmcu);
    exit(1);
  }
  avr_init(avr);
  avr_load_firmware(avr, &f);

  // initialize our peripheral
  uint32_t NUM_LEDS = 32;

  avr_irq_t *led_strip_irq = avr_alloc_irq(&avr->irq_pool, 0, 1, NULL);
  LedStrip *led_strip = ws2812_init(NUM_LEDS, pixels_done_hook);

  avr_irq_register_notify(led_strip_irq, ws2812_pin_changed_hook, led_strip);

  avr_connect_irq(
      avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('B'), IOPORT_IRQ_PIN3),
      led_strip_irq);

  // create a named pipe
  int mkfifo_error = mkfifo(arguments.pipe_name, 0666);
  if (mkfifo_error != 0) {
    perror("mkfifo failed");
  }
  printf("using named pipe: %s\n", arguments.pipe_name);
  fp = fopen(arguments.pipe_name, "w");

  avr_cycle_count_t end_cycle = avr_nsec_to_cycles(avr, arguments.sim_time_ns);
  signal(SIGALRM, percentage_signal_handler);
  alarm(1);
  while (avr->cycle <= end_cycle) {
    avr_run(avr);
    if(print_percentage_flag){
      print_percentage(avr->cycle, end_cycle);
    }
  }
  print_percentage(avr->cycle, end_cycle);

  fclose(fp);
  ws2812_destroy(led_strip);
  remove_fifo(arguments.pipe_name);

  printf("time stopped at %lluns\n", avr_cycles_to_nsec(avr, avr->cycle));
  exit(0);
}
