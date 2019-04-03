/** LEDPin.h
 *  
 */

#ifndef LEDPIN_H
#define LEDPIN_H

#include "Arduino.h"

#define LP_HIGH (LOW)
#define LP_LOW (HIGH)

class LEDPin {

public:

	LEDPin(int digitalpin, int blink_duration) 
	: pin(digitalpin), t0(0), dt(blink_duration) {}

	void blink() {
		digitalWrite(pin, LP_LOW);
		t0 = millis();
	}

	void loop() {
		if (t0 && ((millis() - t0) > dt)) {
			digitalWrite(pin, LP_HIGH);
			t0 = 0;
		}
	}

protected:

	int pin;
	int t0;
	int dt;
};

#endif
