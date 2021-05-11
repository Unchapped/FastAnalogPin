#ifndef AnalogSensor_h
#define AnalogSensor_h

#include <stdint.h>
#include <Arduino.h>
#include "wiring_private.h"

//TODO: at some point we should probably wrap this in a namespace, but whatever...

class FastExpMovingAverage {
  //This enum abstracts our quick-and-dirty binary rolling average filter implementation into an enum
  public:
    enum Alpha {
      ALPHA_2 = 1,
      ALPHA_4 = 2,
      ALPHA_8 = 3,
      ALPHA_16 = 4,
      ALPHA_32 = 5,
      ALPHA_64 = 6,
      ALPHA_128 = 7,
      ALPHA_256 = 8
    };

  private:
    Alpha _alpha;
    int32_t _accumulator;

  public:
    FastExpMovingAverage(Alpha alpha);

    uint_fast16_t filter(uint_fast16_t new_value);

};

class AnalogSensor {
  int _pin;

  public:
    AnalogSensor(int pin);
    int read();
};

class MicrosecondLoopTimer {
  unsigned long _timer, _max_time, _min_time;
  
  public:
    MicrosecondLoopTimer();
    void start();
    void stop();
    unsigned long max();
    unsigned long min();
};

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