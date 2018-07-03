#!python
'''
   Copyright 2018 John Myrda

   Original file Copyright  2013  Gordon McGregor

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

  Modified from https://github.com/GordonMcGregor/vcd_parser/blob/master/ubus_test.py

  An example WS2812 vcd dump parser. My Arduino file was outputting on Pin 11,
  which corresponds to PortB, B3.

'''

from vcd import *
from ws2812 import WS2812
import sys


class UbusWatcher(watcher.VcdWatcher):

    in_reset = False
    signals = ('B3-Out', 'B4-Out')

    def __init__(self, hierarchy=None):
        # set the hierarchical path first, prior to adding signals
        # so they all get the correct prefix paths\
        self.last_time = 0
        self.strip_b3 = WS2812()
        self.set_hierarchy(hierarchy)
        self.add_signals()

    def add_signals(self):
        # Signals in the 'sensitivity list' are automatically added to the watch list
        self.add_sensitive('B3-Out')

        for signal in self.signals:
            self.add_watching(signal)

    def update(self):
        # Called every time something in the 'sensitivity list' changes
        # Doing effective posedge/ negedge checks here and reset/ clock behaviour filtering
        diff = int(self.parser.now) - self.last_time

        # print("time_diff: {}ns".format(diff))
        self.last_time = int(self.parser.now)

        b3 = self.get_id('B3-Out')

        if b3 in self.activity:
            self.strip_b3.run(diff)
            return


if __name__ == "__main__":
    # Create a parser object, attach a watcher within the hierarchy and start running
    vcd = parser.VcdParser()

    watcher = UbusWatcher('PORTB')
    vcd.register_watcher(watcher)
    # vcd.debug = True

    if len(sys.argv) != 2:
        print "Usage: ubus_test.py filename"
        sys.exit(1)

    with open(sys.argv[1]) as vcd_file:
        vcd.parse(vcd_file)
