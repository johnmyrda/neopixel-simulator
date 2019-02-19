# Neopixel Simulator
The intent of this project is to simulate a ws2812 led strip and visualize it. The program led_sim takes in a .hex or .elf file
produced by Arduino and simulates it using simavr.

## How to run

Build the project using the makefile at the root of the repo. This project has the same dependencies as simavr.
An executable file called led_sim is produced under the src directory.

Here's an example of how to run the simulator:

```
make
./src/led_sim -f test/RGBCalibrate.ino.elf -t 10
```

In another window, run pipereader:

```
./src/pipereader.py
```

CSV formatted RGB values should be written to STDOUT.

## Limitations

* The simulator only works with an  led strip on Pin 17 / Arduino Digital Pin 11 / PORTB3.
* The simulator only supports the ATmega328p at the moment - the chip found on the Arduino UNO.
  * Check the [Arduino Board Comparison](https://www.arduino.cc/en/products.compare) to see which Arduino boards use this mcu.
