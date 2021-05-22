#include "AnalogSensor.h"
#include <Arduino.h>

//This probably violates some assumptions, but shame on Arduino for doing everything in the global namespace...
#include "wiring_private.h"


FastExpMovingAverage::FastExpMovingAverage(FastExpMovingAverage::Alpha alpha) : _alpha(alpha), _accumulator(-1) {}

uint_fast16_t FastExpMovingAverage::filter(uint_fast16_t new_value)
{
	if (_accumulator < 0) {
		_accumulator = new_value; //fill in an invalid accumulator value with the first read value
	} else {
		//TODO: For some reason this average always runs slightly low... Debug this later!
		_accumulator = (((_accumulator << _alpha) - _accumulator) + new_value) >> _alpha; //bit shifts give you very fast multiplication for factors of 2.
	}
	return (uint_fast16_t) _accumulator;
}


AnalogSensor::AnalogSensor(int pin) : _pin(pin)
{
  pinMode(_pin, INPUT);
#ifdef SAMD
  analogReadResolution(12);
#endif
}

int AnalogSensor::read()
{
	return analogRead(_pin);
}



/* class MicrosecondLoopTimer */
MicrosecondLoopTimer::MicrosecondLoopTimer(): _timer(micros()), _max_time(0ul), _min_time(~0ul) {};

void MicrosecondLoopTimer::start(){
  _timer = micros();
}
void MicrosecondLoopTimer::stop(){
  _timer = micros() - _timer;
  if (_timer > _max_time) _max_time = _timer;
  if (_timer < _min_time) _min_time = _timer;
}

unsigned long MicrosecondLoopTimer::max() {
  return _max_time;
}

unsigned long MicrosecondLoopTimer::min() {
  return _min_time;
}


/* Analog Pin definitions from g_APinDescription in variant.cpp
  typedef struct _PinDescription
  {
    EPortType       ulPort ;
    uint32_t        ulPin ;
    EPioType        ulPinType ;
    uint32_t        ulPinAttribute ;
    EAnalogChannel  ulADCChannelNumber ; 
    EPWMChannel     ulPWMChannel ;
    ETCChannel      ulTCChannel ;
    EExt_Interrupts ulExtInt ;
  } PinDescription ;

  // 14..21 - Analog pins
  XX { Port, Pin, PinType,     PinAttribute,                                       ADC Channel,    PWMChannel, T/C Channel,  External Int }
  14 { PORTA,  2, PIO_ANALOG,  (PIN_ATTR_DIGITAL|PIN_ATTR_ANALOG                ), ADC_Channel0,   NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE },
  15 { PORTB,  2, PIO_ANALOG,  (PIN_ATTR_DIGITAL                                ), ADC_Channel10,  NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_2    },
  16 { PORTA, 11, PIO_ANALOG,  (PIN_ATTR_DIGITAL|PIN_ATTR_PWM|PIN_ATTR_TIMER_ALT), ADC_Channel19,  PWM0_CH3,   TCC0_CH3,     EXTERNAL_INT_NONE },
  17 { PORTA, 10, PIO_ANALOG,  (PIN_ATTR_DIGITAL|PIN_ATTR_PWM|PIN_ATTR_TIMER_ALT), ADC_Channel18,  PWM0_CH2,   TCC0_CH2,     EXTERNAL_INT_NONE },
  18 { PORTB,  8, PIO_SERCOM_ALT,  (PIN_ATTR_DIGITAL|PIN_ATTR_ANALOG            ), ADC_Channel2,   NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE }, // SDA: SERCOM4/PAD[0]
  19 { PORTB,  9, PIO_SERCOM_ALT,  (PIN_ATTR_PWM|PIN_ATTR_TIMER                 ), ADC_Channel3,   PWM4_CH1,   TC4_CH1,      EXTERNAL_INT_9    }, // SCL: SERCOM4/PAD[1]
  20 { PORTA,  9, PIO_ANALOG,  (PIN_ATTR_DIGITAL                                ), ADC_Channel17,  NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE },
  21 { PORTB,  3, PIO_ANALOG,  (PIN_ATTR_DIGITAL                                ), ADC_Channel11,  NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_3    },
*/

/*
Copied from the SAMD21 Datasheet
32.6.13 Synchronization
Due to the asynchronicity between CLK_ADC_APB and GCLK_ADC, some registers must be synchronized when
accessed. A register can require:
  * Synchronization when written
  * Synchronization when read
  * Synchronization when written and read
  * No synchronization
When executing an operation that requires synchronization, the Synchronization Busy bit in the Status register
(STATUS.SYNCBUSY) will be set immediately, and cleared when synchronization is complete. The Synchronization
Ready interrupt can be used to signal when synchronization is complete.
If an operation that requires synchronization is executed while STATUS.SYNCBUSY is one, the bus will be stalled. All
operations will complete successfully, but the CPU will be stalled and interrupts will be pending as long as the bus is
stalled.
The following bits need synchronization when written:
  * Software Reset bit in the Control A register (CTRLA.SWRST)
  * Enable bit in the Control A register (CTRLA.ENABLE)
The following registers need synchronization when written:
  * Control B (CTRLB)
  * Software Trigger (SWTRIG)
  * Window Monitor Control (WINCTRL)
  * Input Control (INPUTCTRL)
  * Window Upper/Lower Threshold (WINUT/WINLT)
Write-synchronization is denoted by the Write-Synchronized property in the register description.
The following registers need synchronization when read:
  * Software Trigger (SWTRIG)
  * Input Control (INPUTCTRL)
  * Result (RESULT)
Read-synchronization is denoted by the Read-Synchronized property in the register description.
*/

/* original function... for reference...
int analogRead(pin_size_t pin)
{
  uint32_t valueRead = 0;

  if (pin < A0) {
    pin += A0;
  }

  pinPeripheral(pin, PIO_ANALOG);

  // Disable DAC, if analogWrite() was used previously to enable the DAC
  if ((g_APinDescription[pin].ulADCChannelNumber == ADC_Channel0) || (g_APinDescription[pin].ulADCChannelNumber == DAC_Channel0)) {
    syncDAC();
    DAC->CTRLA.bit.ENABLE = 0x00; // Disable DAC
    //DAC->CTRLB.bit.EOEN = 0x00; // The DAC output is turned off.
    syncDAC();
  }

  syncADC();
  ADC->INPUTCTRL.bit.MUXPOS = g_APinDescription[pin].ulADCChannelNumber; // Selection for the positive ADC input

  // Control A
  /*
   * Bit 1 ENABLE: Enable
   *   0: The ADC is disabled.
   *   1: The ADC is enabled.
   * Due to synchronization, there is a delay from writing CTRLA.ENABLE until the peripheral is enabled/disabled. The
   * value written to CTRL.ENABLE will read back immediately and the Synchronization Busy bit in the Status register
   * (STATUS.SYNCBUSY) will be set. STATUS.SYNCBUSY will be cleared when the operation is complete.
   *
   * Before enabling the ADC, the asynchronous clock source must be selected and enabled, and the ADC reference must be
   * configured. The first conversion after the reference is changed must not be used.
   * /
  syncADC();
  ADC->CTRLA.bit.ENABLE = 0x01;             // Enable ADC

  // Start conversion
  syncADC();
  ADC->SWTRIG.bit.START = 1;

  // Waiting for the 1st conversion to complete
  while (ADC->INTFLAG.bit.RESRDY == 0);

  // Clear the Data Ready flag
  ADC->INTFLAG.reg = ADC_INTFLAG_RESRDY;

  // Start conversion again, since The first conversion after the reference is changed must not be used.
  syncADC();
  ADC->SWTRIG.bit.START = 1;

  // Store the value
  while (ADC->INTFLAG.bit.RESRDY == 0);   // Waiting for conversion to complete
  valueRead = ADC->RESULT.reg;

  syncADC();
  ADC->CTRLA.bit.ENABLE = 0x00;             // Disable ADC
  syncADC();

  return mapResolution(valueRead, _ADCResolution, _readResolution);
}
*/


//define static variables
uint32_t FastAnalogPin::_instance_counter;
uint32_t FastAnalogPin::_nb_locking_pin;

FastAnalogPin::FastAnalogPin(uint32_t pin) : _pin(pin), _nb_state(0) {
  //validate pin
  if (_pin < A0) _pin += A0; 
  if (_pin > A7) _pin += A7; //TODO: it would probably be better to use an exception or factory method, this fails silently...
  
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
