#include "i8253chip.h"

/*

From 8253/8253-5 PROGRAMMABLE INTERVAL TIMER manual

MODE 3: Square Wave Rate Generator. Similar to
MODE 2 except that the output will remain high until
one half the count has been completed (or even
numbers) and go low for the other half of the count.
This is accomplished by decrementing the counter
by two on the falling edge of each clock pulse. When
the counter reaches terminal count, the state of the
output is changed and the counter is reloaded with
the full count and the whole process is repeated.
If the count is odd and the output is high, the first
clock pulse (after the count is loaded) decrements
the count by 1. Subsequent clock pulses decrement
the clock by 2. After timeout, the output goes low
and the full count is reloaded. The first clock pulse
(following the reload) decrements the counter by 3.
Subsequent clock pulses decrement the count by 2
until timeout. Then the whole process is repeated. In
this way, if the count is odd, the output will be high
for (N + 1)/2 counts and low for (N - 1)/2 counts.
In Modes 2 and 3, if a CLK source other than the
system clock is used, GATE should be pulsed immediately following WR of a new count value.

Loading all zeros into a count register will
result in the maximum count (216 for Binary

NOTE: All zeros is not implemented, and used in VGM export for silence

*/

i8253_device::i8253_device():
      counter(0),
      latch(0),
      output(0),
      clockRemainder(0),
      firstClock(true),
      chipClock(1108400) {
  reset();
}

void i8253_device::setFreq(uint16_t val) {
  if (val == 1) {
    val = 0;
  }
  latch = val;
  counter = val;
  firstClock = true;
}

short i8253_device::tick(int rate) {
  if (latch == 0) {
    return 0;
  }
  if (rate <= 0) {
    return 0;
  }

  // clockRemainder is the remaining part of a sample from previous call
  clockRemainder += chipClock;
  // we loop through this several times
  // but then we only return the last sample
  while (clockRemainder >= rate) {
    clockRemainder -= rate;

    if (latch & 1) {
      // Odd value
      if (firstClock) {
        if (output) {
          // output high: first clock -1
          counter -= 1; 
        } else {
          // output low: first clock -3
          counter -= 3; 
        }
        firstClock = false;
      } else {
        // other clocks always -2
        counter -= 2; 
      }
    } else {
      // Even value: always -2
      counter -= 2;
    }

    if (counter <= 0) {
      output ^= 1;
      counter = latch;
      firstClock = true;
    }
  }
  // return the last sample from the loop above
  return output ? 32767 : -32768;
}

void i8253_device::reset() {
  counter = 0;
  latch = 0;
  output = 0;
  clockRemainder = 0;
  firstClock = true;
}
