import os
import errno
import fcntl
import sys
import struct

FIFO = "/tmp/neopixel"

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


def main():
    try:
        os.mkfifo(FIFO)
    except OSError as oe: 
            if oe.errno != errno.EEXIST:
                    raise
    while True:
            length = 0
            with open(FIFO, 'rb') as f:
                time_b = f.read(8)
                if not time_b:
                        break
                time = struct.unpack('L', time_b)[0]
                print("time: {}s ({}ns)".format(time/1000000000, time)) 
                length_b = f.read(4)
                if not length_b:
                        break
                length = struct.unpack('I', length_b)[0]
                print("strip length {}".format(length))
                if length > 0:
                    pixels_b = f.read(length*3)
                    for i in range(length):
                            pixel_tuple = struct.unpack("BBB", pixels_b[i*3:i*3+3])
                            pixel = RGBW(*pixel_tuple)
                            print(pixel)
   



if __name__=="__main__":
        main()
