# Neopixel Simulator
The intent of this project is to simulate a ws2812 led strip and visualize it. The program led_sim takes in a .hex or .elf file
produced by Arduino and simulates it using simavr.

## Prerequisites

This project makes use of several git submodules. Make sure they are properly checked out.

Unit tests are run via [ceedling](http://www.throwtheswitch.org/ceedling).

Python 3 is recommended to use the `pipereader.py` program.

This project has the same system dependencies as [simavr](https://github.com/buserror/simavr).

## How to run

Build the project using the makefile at the root of the repo. 
An executable file called led_sim is produced under the src directory.

Here's an example of how to run the simulator:

```
make
./src/led_sim -f test/RGBCalibrate.ino.elf -t 10
```

CSV formatted RGB values will be written to `led_out.csv`.

## Running tests

Make sure [ceedling](http://www.throwtheswitch.org/ceedling) is installed and run:

```
ceedling test:all
```

This will run all unit tests found in the `test` directory.


## Limitations

* The simulator only works with an  led strip on Pin 17 / Arduino Digital Pin 11 / PORTB3.
* The simulator only supports the ATmega328p at the moment - the chip found on the Arduino UNO.
  * Check the [Arduino Board Comparison](https://www.arduino.cc/en/products.compare) to see which Arduino boards use this mcu.
