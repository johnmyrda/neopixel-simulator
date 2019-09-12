#!/usr/bin/env python3
import os
import errno
import fcntl
import sys
import struct
import argparse


class RGBW:

    def __init__(self, green, red, blue, white=None):
        self.red = red
        self.green = green
        self.blue = blue
        self.white = white

    def rgb_tuple(self):
        return (self.red, self.green, self.blue)

    def __str__(self):
        return ("RGB: [{},{},{}]".format(self.red, self.green, self.blue))
        # return("RGBW: ({},{},{},{})".format(self.red, self.green, self.blue, self.white))


def parse_args():
    FIFO = "/tmp/neopixel"

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--fifo", help="name of the pipe to use. Default: {}".format(FIFO), default=FIFO)
    return parser.parse_args()


def read_from_pipe(pipe_name):
    try:
        os.mkfifo(pipe_name)
    except OSError as oe:
        if oe.errno != errno.EEXIST:
            raise
    while True:
        length = 0
        with open(pipe_name, 'rb') as f:
            csv = list()
            # binary format:
            # timestamp (uint64_t)
            # led strip length (uint32_t)
            # RGB data (rgb_pixel_t == 3*uint8_t)*length
            time_b = f.read(8)
            if not time_b:
                break
            time = struct.unpack('L', time_b)[0]
            csv.append(time)
            length_b = f.read(4)
            if not length_b:
                break
            length = struct.unpack('I', length_b)[0]
            csv.append(length)
            if length > 0:
                pixels_b = f.read(length*3)
                for i in range(length):
                    # read bytes in groups of 3 so that each represents a pixel in the csv
                    hex_bytes = pixels_b[i*3:i*3+3].hex()
                    csv.append(hex_bytes)
            print(",".join(str(x) for x in csv))


if __name__ == "__main__":
    args = parse_args()
    read_from_pipe(args.fifo)
