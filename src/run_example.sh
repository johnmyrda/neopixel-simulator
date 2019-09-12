#!/usr/bin/env bash

directory=${BASH_SOURCE%/*}

"${directory}"/led_sim -t 10 -f ../test/rainbow_short.ino.elf &
"${directory}"/pipereader.py > led_out.csv
