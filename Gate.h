/*
 *	Gate.h  
 */
#ifndef GATE_H
#define GATE_H

#include <EEPROM.h>

struct GateCalibration {
	char valid[8];
	int min;
	int max;
	int high;
	int low;
	bool invert;
};

class Gate {
	
public:

	Gate(int dflt_min, int dflt_max, int eeprom_addr, Stream *debug_serial) 
	: 	dflt_min(dflt_min), 
		dflt_max(dflt_max), 
		eeprom_addr(eeprom_addr),  
		debug_serial(debug_serial) {}

	void init() {
		if (!load_calibration(dflt_min, dflt_max))
			calibrate();
	}

	// Set current value as min and recalibrate
	void calibrate_min() {
		cal.min = value;
		calibrate();
	}

	// Set current value as max and recalibrate
	void calibrate_max() {
		cal.max = value;
		calibrate();
	}

	// Get the current state 
	bool get_state() { return cal.invert ? !state : state; }
	bool get_value() { return value; }

	// Process a new sample, return true if state changes
	bool process(int val) {
		bool update = false;
		value = val;
		if (state == true && value < cal.low) {
		    state = false;
		    update = true;
		  }
		  else if (state == false && value > cal.high) {
		    state = true;
		    update = true;
		}
		return update;
	}

protected:

	void calibrate() {

		// Compute hysteresis thresholds
		int range = cal.max - cal.min;
		int min = range > 0 ? cal.min : cal.max;
		cal.invert = range < 0;
		range = abs(range);
		cal.low = min + range / 3.0;
		cal.high = min + 2 * range / 3.0;

		if (debug_serial) {
			debug_serial->println("Saving calibration:");
			print_calibration();
		}
		
		// Write new calibration values to EEPROM
		strcpy(cal.valid, "you");
		EEPROM.put(eeprom_addr, cal);
		EEPROM.commit();
	}

	bool load_calibration(int dflt_min, int dflt_max) {

		bool success = true;
		EEPROM.get(eeprom_addr, cal);

		if (!(strcmp(cal.valid, "you") == 0)) {

			// Set defaults
			cal.min = dflt_min;
			cal.max = dflt_max;
			calibrate();

			success = false;
		}
		else if (debug_serial) {
			debug_serial->println("Calibration loaded:");
			print_calibration();
		}

		return success;
	}

	void print_calibration() {
		debug_serial->print("cal.min = ");
		debug_serial->println(cal.min);
		debug_serial->print("cal.max = ");
		debug_serial->println(cal.max);
		debug_serial->print("cal.high = ");
		debug_serial->println(cal.high);
		debug_serial->print("cal.low = ");
		debug_serial->println(cal.low);
		debug_serial->print("cal.invert = ");
		debug_serial->println(cal.invert);
	}

	int value;
	bool state;
	struct GateCalibration cal;
	int dflt_min;
	int dflt_max;
	int eeprom_addr;
	Stream *debug_serial;
};

#endif