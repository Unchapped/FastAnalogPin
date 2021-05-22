/* This file intentionally left blank, see Main.cpp */

//Include libraries used to get around weirdnesses in the arduino build paths
#include <stdint.h>
#include <Arduino.h>
#include "wiring_private.h"
#include "AnalogSensor.h"


void setup() {
#ifdef SAMD
	analogReadResolution(12);
#endif

	Serial.begin(9600);
  //while (!Serial); // wait for serial port to connect. Needed for native USB port only 
  //(re)initialize pin modes
  for(int pin = A0; pin < A4; pin++) pinMode(pin, INPUT);
}

void loop() {
	/* Blocking Timing Experiment */
    int core_results[4];
    MicrosecondLoopTimer core_loop_timer;
    MicrosecondLoopTimer core_avg_timer;
    core_avg_timer.start();
    {
	    //(re)initialize pin modes
	    for(int pin = A0; pin < A4; pin++) pinMode(pin, INPUT);

	    //take 8 samples for the average
	    for(int avg_counter = 0; avg_counter < 8; avg_counter++) {
	      for(int pin = 0; pin < 4; pin++) {
	        core_loop_timer.start();
	        core_results[pin] += analogRead(A0 + pin);
	        core_loop_timer.stop();
	      } 
	    }

	    for(int pin = 0; pin < 4; pin++) {
	      core_results[pin] = core_results[pin] >> 3; //fast divide by 8
      }
	  }
    core_avg_timer.stop();

    Serial.print("Arduino core results: ");
    for(int pin = 0; pin < 4; pin++) {
      Serial.print(core_results[pin]);
      Serial.print(",");
    }
    Serial.println("");
    Serial.print("  Loop Time: min ");
    Serial.print(core_loop_timer.min());
    Serial.print(" us, max ");
    Serial.print(core_loop_timer.max());
    Serial.println(" us");
    Serial.print("  Total Time: ");
    Serial.print(core_avg_timer.max());
    Serial.println(" us");

    /* Fast Timing Experiment */
    int fast_results[4];
    MicrosecondLoopTimer fast_loop_timer;
    MicrosecondLoopTimer fast_avg_timer;

    fast_avg_timer.start();

    //adding block scope here to make sure objects are initialized and cleaned up.
    {
      //Initialize pins
      FastAnalogPin fast_pins[] = {FastAnalogPin(A0), FastAnalogPin(A1), FastAnalogPin(A2), FastAnalogPin(A3)};

      //TODO: hardware averaging is available, if we want to use that, see SAMD21 section 32.6.6 for details
      //take 8 samples for the average
      for(int avg_counter = 0; avg_counter < 8; avg_counter++) {
        for(int pin = 0; pin < 4;pin++) { //standard pin++ iterator intentionally left out.
          fast_loop_timer.start();
          fast_results[pin] += fast_pins[pin].read();
          fast_loop_timer.stop();
        }
      }
      for(int pin = 0; pin < 4; pin++) {
        fast_results[pin] = fast_results[pin] >> 3; //fast divide by 8
      }
    }//fast pins destuctors get called here
    fast_avg_timer.stop();
    Serial.print("Fast read results: ");
    for(int pin = 0; pin < 4; pin++) {
      Serial.print(fast_results[pin]);
      Serial.print(",");
    }
    Serial.println("");
    Serial.print("  Loop Time: min ");
    Serial.print(fast_loop_timer.min());
    Serial.print(" us, max ");
    Serial.print(fast_loop_timer.max());
    Serial.println(" us");
    Serial.print("  Total Time: ");
    Serial.print(fast_avg_timer.max());
    Serial.println(" us");


    /* NonBlocking Timing Experiment */
    int nonblocking_results[4];
    MicrosecondLoopTimer nonblocking_loop_timer;
    MicrosecondLoopTimer nonblocking_avg_timer;

    nonblocking_avg_timer.start();

    //adding block scope here to make sure objects are initialized and cleaned up.
    {
      //Initialize pins
      FastAnalogPin nonblocking_pins[] = {FastAnalogPin(A0), FastAnalogPin(A1), FastAnalogPin(A2), FastAnalogPin(A3)};

      //TODO: hardware averaging is available, if we want to use that, see SAMD21 section 32.6.6 for details
      //take 8 samples for the average
      for(int avg_counter = 0; avg_counter < 8; avg_counter++) {
        for(int pin = 0; pin < 4;) { //standard pin++ iterator intentionally left out.
          nonblocking_loop_timer.start();
          int result = nonblocking_pins[pin].read_nb();
          if (result >= 0) {
            nonblocking_results[pin] += result;
            pin++;
          }
          nonblocking_loop_timer.stop();
        }
      }
      for(int pin = 0; pin < 4; pin++) {
        nonblocking_results[pin] = nonblocking_results[pin] >> 3; //fast divide by 8
      }
    }//nonblocking pins destuctors get called here
    nonblocking_avg_timer.stop();
    Serial.print("Non-blocking results: ");
    for(int pin = 0; pin < 4; pin++) {
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

    delay(500);
}