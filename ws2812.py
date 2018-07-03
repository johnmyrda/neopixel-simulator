import sys
import bitarray
from itertools import izip_longest


def grouper(n, iterable, fillvalue=None):
    args = [iter(iterable)] * n
    return izip_longest(fillvalue=fillvalue, *args)


# This class was written with the Value Change Dump (VCD) format in mind
# A proper emulation of the ws2812 and similar LED strips
# would likely require periodic sampling (around once every 50ns)
class WS2812:

    class Timing:
        # low is lower timing bound, high is upper timing bound for signal
        def __init__(self, low, high):
            self.low = low
            self.high = high

    # Time in ns
    # https://wp.josh.com/2014/05/13/ws2812-neopixels-are-not-so-finicky-once-you-get-to-know-them/
    T0H = Timing(200, 500)
    T1H = Timing(550, 5500)
    TLD = Timing(300, 5000)  # was 450 but data says otherwise
    TLL = Timing(6000, sys.maxsize)

    def __init__(self):
        self.bits = list()
        self.cur_bit = None
        self.voltage = 0

    # Since the signal is binary, it is assumed that the voltage
    # flips every time this is called. A sampling based emulation
    # would not make this assumption.
    def run(self, time):
        # print "Voltage was {} for {}ns".format(self.voltage, time),

        if(self.voltage == 1):
            self.voltage = 0
            if(self.T0H.low < time < self.T0H.high):
                self.cur_bit = 0
            elif(self.T1H.low < time < self.T1H.high):
                self.cur_bit = 1
            else:
                # undefined for now
                self.cur_bit = 1
        # Voltage is low
        else:
            self.voltage = 1
            if(self.TLD.low < time < self.TLD.high):
                self.bits.append(self.cur_bit)
            elif(self.TLL.low < time < self.TLL.high):
                self.bits.append(self.cur_bit)
                color_bytes = self.bits_to_bytes(self.bits)
                colors = self.bytes_to_rgb(color_bytes)
                print "------"
                self.bits = list()
                # LATCH, output/reset
            else:
                # undefined for now
                pass

    # Datasheet describes format
    # https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf
    # Big Endian?
    @staticmethod
    def bits_to_bytes(bits):
        return bytearray(bitarray.bitarray(bits).tobytes())

    # GRB ordering
    @staticmethod
    def bytes_to_rgb(color_bytes):
        color_list = list()
        for g, r, b in grouper(3, color_bytes):
            new_color = RGBW(r, g, b)
            color_list.append(new_color)
        return color_list


class RGBW:

    def __init__(self, red, green, blue, white=None):
        self.red = red
        self.green = green
        self.blue = blue
        self.white = white

    def __str__(self):
        return("RGBW: ({},{},{},{})".format(self.red, self.green, self.blue, self.white))
