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

	Serial.begin(115200);
}

void loop() {
	/* Blocking Timing Experiment */
    int blocking_results[4];
    MicrosecondLoopTimer blocking_loop_timer;
    MicrosecondLoopTimer blocking_avg_timer;

    blocking_avg_timer.start();
    {
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
}