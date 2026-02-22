#pragma once
#include <stdint.h>

class i8253_device {
  int counter;
  uint16_t latch;
  uint8_t output;
  int clockRemainder;
  bool firstClock;

  public:
    int chipClock;

    void setFreq(uint16_t val);
    short tick(int rate);
    void reset();

    i8253_device();
  
};