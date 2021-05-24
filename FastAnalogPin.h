#ifndef FastAnalogPin_h
#define FastAnalogPin_h

#include <stdint.h>
#include <Arduino.h>
#include "wiring_private.h"

class FastAnalogPin {
  static uint32_t _instance_counter;
  uint32_t _pin;

  //nonblocking mode mutex and state
  static uint32_t _nb_locking_pin;
  uint32_t _nb_state;

  public:
    //Secret value for no pin lock, 0xFF... is unlikely to be a real pin #
    static const uint32_t ANALOG_PIN_UNLOCKED = ~0;

    //return code for an incomplete nonblocking conversion
    static const int ANALOG_BUSY = -1;

    FastAnalogPin(uint32_t pin);
    ~FastAnalogPin();

    uint32_t read();
    int read_nb();
};



#endif