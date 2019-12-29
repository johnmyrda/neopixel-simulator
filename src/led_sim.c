#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// simavr
#include "avr_ioport.h"
#include "sim_avr.h"
#include "sim_elf.h"
#include "sim_gdb.h"
#include "sim_irq.h"

// string handling library
#include "sds.h"

// led strip emulation
#include "ws2812.h"

FILE *fp; // file descriptor for output file or named pipe
avr_t *avr = NULL;

bool print_percentage_flag = false;
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

// call the ws2812_run function every time the pin transitions from low->high or high->low.
void ws2812_pin_changed_hook(struct avr_irq_t * irq, uint32_t value, void *param){
    uint64_t time_nsec = avr_cycles_to_nsec(avr, avr->cycle);
    ws2812_run((LedStrip *)param, time_nsec, value);
}

// output RGB led strip data in CSV format to a file pointer
// This function is called when the led strip latches
void pixels_done_hook_csv(const LedStrip * const led_strip) {
  if(ws2812_colors_changed(led_strip)){  
    uint8_t header_offset = 2;
    uint32_t strip_length = ws2812_get_length(led_strip);
    uint32_t csv_line_length = header_offset + strip_length;
    sds * csv_line = malloc(sizeof(void *) *(csv_line_length));
    csv_line[0] = sdsfromlonglong(ws2812_get_last_pin_change_time(led_strip));
    csv_line[1] = sdsfromlonglong(strip_length);

    for(int i = 0; i < strip_length; i++){
      rgb_pixel_t current_pixel = ws2812_get_pixels(led_strip)[i];
      sds color_hex = sdsnewlen("", 6); // make sds string with big enough buffer for hex representation of pixel
      ws2812_pixel_to_hex(&current_pixel, color_hex);
      csv_line[i + header_offset] = color_hex;
    }

    sds full_line = sdsjoinsds(csv_line, csv_line_length, ",", 1);
    full_line = sdscat(full_line, "\n");
    fwrite(full_line, sizeof(char), sdslen(full_line), fp);
    sdsfree(full_line);
    sdsfreesplitres(csv_line, csv_line_length);
  }
}

// output RGB led strip data in a binary format to a file pointer
// This function is called when the led strip latches
void pixels_done_hook_binary(const rgb_pixel_t *pixels, const uint32_t strip_length,
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
      alarm(1); // reset alarm so this function triggers again in 1 second
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

void create_fifo(const char * fifo_name){
  // create a named pipe
  int mkfifo_error = mkfifo(fifo_name, 0666);
  if (mkfifo_error != 0) {
    perror("mkfifo failed");
  }
  printf("using named pipe: %s\n", fifo_name);
  fp = fopen(fifo_name, "w");
}

struct arg_struct {
  char *input_name;
  char *output_name;
  bool use_pipe;
  uint64_t sim_time_ns;
};

void usage() {
  fprintf(stdout, "usage: led_sim -t 10 -f firmware.elf\n");
  fprintf(stdout, "  -t <time in seconds> default: 5\n");
  fprintf(stdout, "  -n <time in nanoseconds> note: superceded by -t\n");
  fprintf(stdout, "  -f <file to simulate>\n");
  fprintf(stdout, "  -o <output file>\n");
  fprintf(stdout, "  -p output to fifo instead of file. uses file from -o\n");
  exit(2);
}

struct arg_struct parse_args(int argc, char *argv[]) {
  // default arguments
  struct arg_struct arguments = {0};
  // initialize struct members
  uint64_t sim_time = 0;
  arguments.sim_time_ns = NANOSECONDS_PER_SECOND * (uint64_t) 5;
  arguments.use_pipe = false;
  arguments.input_name = "";
  arguments.output_name = "led_out.csv";

  int opt;
  while ((opt = getopt(argc, argv, ":n:t:f:o:ph")) != -1) {
    switch (opt) {
    case 'n': // simulation time in nanoseconds
      arguments.sim_time_ns = strtoull(optarg, NULL, 10);
      break;
    case 't': // simulation time in seconds
      sim_time = strtoull(optarg, NULL, 10);
      break;
    case 'f': // filename of file to simulate
      arguments.input_name = optarg;
      break;
    case 'o': // name of file to use for output
      arguments.output_name = optarg;
      break;
    case 'p': // whether to use pipe
      arguments.use_pipe = true;
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

  if (access(arguments.input_name, F_OK) < 0) {
    printf("Could not find file %s\n", arguments.input_name);
    usage();
    exit(1);
  }

  return arguments;
}

int main(int argc, char *argv[]) {
  struct arg_struct arguments = parse_args(argc, argv);

  elf_firmware_t f;
  elf_read_firmware(arguments.input_name, &f);

  f.frequency = 16000000;
  strncat(f.mmcu, arguments.input_name, sizeof(f.mmcu) - 1);
  printf("firmware %s f=%d mmcu=%s\n", arguments.input_name, (int)f.frequency,
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
  LedStrip *led_strip = ws2812_init(NUM_LEDS, pixels_done_hook_csv);

  avr_irq_register_notify(led_strip_irq, ws2812_pin_changed_hook, led_strip);

  avr_connect_irq(
      avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('B'), IOPORT_IRQ_PIN3),
      led_strip_irq);

  if(arguments.use_pipe){
    create_fifo(arguments.output_name);
  } else {
    remove(arguments.output_name);
    fp = fopen(arguments.output_name, "w" );
    printf("writing to output file %s\n", arguments.output_name);
  }

  avr_cycle_count_t end_cycle = avr_nsec_to_cycles(avr, arguments.sim_time_ns);
  signal(SIGALRM, percentage_signal_handler);
  alarm(1); // set percentage_signal_handler to trigger one second from now
  while (avr->cycle <= end_cycle) {
    avr_run(avr);
    if(print_percentage_flag){
      print_percentage(avr->cycle, end_cycle);
    }
  }
  print_percentage(avr->cycle, end_cycle);

  fclose(fp);
  ws2812_destroy(led_strip);
  if(arguments.use_pipe){
    remove_fifo(arguments.output_name);
  } else {
    fclose(fp);
  }

  printf("time stopped at %lluns\n", avr_cycles_to_nsec(avr, avr->cycle));
  exit(0);
}
