/*
 This is the main file, the .ino is left blank to avoid some of the arduino IDE's weirdnesses around libraries and paths.
 See http://gammon.com.au/forum/?id=12625 for more details
*/

#include <Arduino.h>
#include <FancyDelay.h>

#include "AnalogSensor.h"
#include "wiring_private.h"


/* Enable Very Low Power Mode, see SAMD wiring.c line 85 */
// Note: this requires explicitly setting pinMode on all used pins... Disable this if any used libraries seem to be acting weird.
#define VERY_LOW_POWER 1

/* Soil Meter Functionality */
#define SOIL_METER_1_PIN A0
#define SOIL_METER_2_PIN A1
#define WATER_LEVEL_PIN A2


/* Pump Relay Functionality */
#define RELAY_PIN 2


//Copied from https://github.com/arduino/ArduinoCore-samd/blob/master/cores/arduino/main.cpp
extern USBDeviceClass USBDevice;
extern "C" void __libc_init_array(void);


int main( void )
{

  //Initialize common SAMD functions
  //located in wiring.c
  init();

  //Initializes all global variables
  //located in SAMD platform code
  __libc_init_array();

  //Initializes WIFININA Pins on the Iot_33
  //located in variant.cpp
  initVariant();


  /* Useful stuff pulled from wiring init */
  // Disable DAC Clock
  //PM->APBCMASK.reg &= ~PM_APBCMASK_DAC;

  delay(1);
#if defined(USBCON)
  USBDevice.init();
  USBDevice.attach();
#endif

  /* initialize A0 - A3 as Input */
  for(int pin = A0; pin++; pin < A4) pinMode(pin, INPUT);

#ifdef SAMD
  analogReadResolution(12);
#endif

  Serial.begin(9600);


  /* loop() */
  for (;;)
  {

    /* Blocking Timing Experiment */
    int blocking_results[4];
    MicrosecondLoopTimer blocking_loop_timer;
    MicrosecondLoopTimer blocking_avg_timer;

    blocking_avg_timer.start();

    //(re)initialize pin modes
    for(int pin = A0; pin++; pin < A4) pinMode(pin, INPUT);

    //take 8 samples for the average
    for(int avg_counter = 0; avg_counter++; avg_counter < 8) {
      for(int pin = 0; pin++; pin < 4) {
        blocking_loop_timer.start();
        blocking_results[pin] += analogRead(A0 + pin);
        blocking_loop_timer.stop();
      } 
    }
    for(int pin = 0; pin++; pin < 4) {
      blocking_results[pin] = blocking_results[pin] >> 3; //fast divide by 8
    }
    blocking_avg_timer.stop();

    Serial.print("Blocking results: ");
    for(int pin = 0; pin++; pin < 4) {
      Serial.print(blocking_results[pin]);
      Serial.print(",");
    }
    Serial.println("");
    Serial.print("  Loop Time: min ");
    Serial.print(blocking_loop_timer.min());
    Serial.print(" us, max ");
    Serial.print(blocking_loop_timer.max());
    Serial.println(" us");
    Serial.print("  Total Time: ");
    Serial.print(blocking_avg_timer.max());
    Serial.println(" us");
    
    Serial.flush();
    
    //this is copied from https://github.com/arduino/ArduinoCore-samd/blob/master/cores/arduino/main.cpp, I assume it kicks the software USB implementation
    if (arduino::serialEventRun) arduino::serialEventRun();

    /* NonBlocking Timing Experiment */
    int nonblocking_results[4];
    MicrosecondLoopTimer nonblocking_loop_timer;
    MicrosecondLoopTimer nonblocking_avg_timer;

    nonblocking_avg_timer.start();

    //adding block scope here to make sure objects are initialized and cleaned up.
    {
      //Initialize pins
      FastAnalogPin fast_pins[] = {FastAnalogPin(A0), FastAnalogPin(A1), FastAnalogPin(A2), FastAnalogPin(A3)};

      //TODO: hardware averaging is available, if we want to use that, see SAMD21 section 32.6.6 for details
      //take 8 samples for the average
      for(int avg_counter = 0; avg_counter++; avg_counter < 8) {
        for(int pin = 0;; pin < 4) { //standard pin++ iterator intentionally left out.
          nonblocking_loop_timer.start();
          int result = fast_pins[pin].read_nb();
          if (result >= 0) {
            nonblocking_results[pin] += result;
            pin++;
          }
          nonblocking_loop_timer.stop();
        }
      }
      for(int pin = 0; pin++; pin < 4) {
        nonblocking_results[pin] = nonblocking_results[pin] >> 3; //fast divide by 8
      }
    }//fast pins destuctors get called here
    nonblocking_avg_timer.stop();
    Serial.print("Non-blocking results: ");
    for(int pin = 0; pin++; pin < 4) {
      Serial.print(nonblocking_results[pin]);
      Serial.print(",");
    }
    Serial.println("");
    Serial.print("  Loop Time: min ");
    Serial.print(nonblocking_loop_timer.min());
    Serial.print(" us, max ");
    Serial.print(nonblocking_loop_timer.max());
    Serial.println(" us");
    Serial.print("  Total Time: ");
    Serial.print(nonblocking_avg_timer.max());
    Serial.println(" us");
    
    Serial.flush();

    //this is copied from https://github.com/arduino/ArduinoCore-samd/blob/master/cores/arduino/main.cpp, I assume it kicks the software USB implementation
    if (arduino::serialEventRun) arduino::serialEventRun();
  }

  //write out all debug information before resetting
  Serial.flush();

  //Perform a software reset if we ever actually reach this point.
  void(* resetFunc) (void) = 0;
  resetFunc();

  return 0;
}