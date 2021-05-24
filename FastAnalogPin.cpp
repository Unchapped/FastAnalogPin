#include "FastAnalogPin.h"
#include <Arduino.h>

//This probably violates some assumptions, but shame on Arduino for doing everything in the global namespace...
#include "wiring_private.h"

//define static variables
uint32_t FastAnalogPin::_instance_counter;
uint32_t FastAnalogPin::_nb_locking_pin;

FastAnalogPin::FastAnalogPin(uint32_t pin) : _pin(pin), _nb_state(0) {
  //validate pin
  if (_pin < A0) _pin += A0; 
  if (_pin > A7) _pin = A7; //TODO: it would probably be better to use an exception or factory method, this fails silently...
  
  //initialize pin
  pinMode(_pin, INPUT);
  pinPeripheral(_pin, PIO_ANALOG);

  // Disable DAC, if analogWrite() was used previously to enable the DAC
  if ((g_APinDescription[_pin].ulADCChannelNumber == ADC_Channel0) || (g_APinDescription[_pin].ulADCChannelNumber == DAC_Channel0)) {
    while (DAC->STATUS.bit.SYNCBUSY == 1); //syncDAC();
    DAC->CTRLA.bit.ENABLE = 0x00; // Disable DAC
    //DAC->CTRLB.bit.EOEN = 0x00; // The DAC output is turned off.
    //while (DAC->STATUS.bit.SYNCBUSY == 1); //syncDAC();
  }

  //this is the first instance, initialize the ADC
  if(_instance_counter == 0) {
      _nb_locking_pin = ANALOG_PIN_UNLOCKED; //initialize the lock
      while (ADC->STATUS.bit.SYNCBUSY == 1); //syncADC();
      ADC->CTRLA.bit.ENABLE = 0x01;             // Enable ADC
  }
  _instance_counter += 1;
}

FastAnalogPin::~FastAnalogPin() {
    _instance_counter -= 1;

    //No instances left, disable the ADC
    if(_instance_counter == 0) {
        while (ADC->STATUS.bit.SYNCBUSY == 1); //syncADC();
        ADC->CTRLA.bit.ENABLE = 0x00;             // Disable ADC
    }
}

uint32_t FastAnalogPin::read() {
  // Select the relevant channel for the positive ADC input Mux
  while (ADC->STATUS.bit.SYNCBUSY == 1); //syncADC();
  ADC->INPUTCTRL.bit.MUXPOS = g_APinDescription[_pin].ulADCChannelNumber;

  // Start conversion
  while (ADC->STATUS.bit.SYNCBUSY == 1); //syncADC();
  ADC->SWTRIG.bit.START = 1;

  //Note, since we're not changing the reference clock every damn time, the first value should be fine.
  while (ADC->INTFLAG.bit.RESRDY == 0); //Wait for the conversion to complete
  return ADC->RESULT.reg; //TODO: map this as the analogRead() does?
}


int FastAnalogPin::read_nb() {
  if (_nb_locking_pin != ANALOG_PIN_UNLOCKED) {
    if (_nb_locking_pin != _pin) return ANALOG_BUSY; //someone else has this locked
  }
  
  _nb_locking_pin = _pin;

  //overflows are intentional
  switch (_nb_state) {
    case 0:
      if (ADC->STATUS.bit.SYNCBUSY) break; //syncADC();
      _nb_state++;
    case 1:
      ADC->INPUTCTRL.bit.MUXPOS = g_APinDescription[_pin].ulADCChannelNumber;
      _nb_state++;
    case 2:
      if (ADC->STATUS.bit.SYNCBUSY) break; //syncADC();
      _nb_state++;
    case 3:
      ADC->SWTRIG.bit.START = 1; //start conversion
      _nb_state++;
    case 4:
      if (ADC->INTFLAG.bit.RESRDY == 0) break; //Wait for the conversion to complete
      //conversion complete
      _nb_locking_pin = ANALOG_PIN_UNLOCKED;
      _nb_state = 0; //ready for another call
      return ADC->RESULT.reg;
    break;
  }
  return ANALOG_BUSY;  
}
