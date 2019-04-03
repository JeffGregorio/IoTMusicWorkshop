#include "Oscillator.h"

#define LFO8_DFLT_PERIOD 8000
#define LFO8_DFLT_DUTY 0.5

LFO8::LFO8() :
period(LFO8_DFLT_PERIOD), duty(LFO8_DFLT_DUTY), state(LFOStateIncline), value(0), 
slope(0), slope_incline(0), slope_decline(0),
phase(0), len(2), len_incline(1), len_decline(1) {
	
}

LFO8::~LFO8() {

}

void LFO8::set_period(uint32_t period_samples) {
	period = period_samples < LFO8_LEN_MAX ? period_samples : LFO8_LEN_MAX;
	recompute();
}

void LFO8::set_duty_cycle(float duty_norm) {
	duty = duty_norm > 0 ? duty_norm : 0;
	duty = duty < 1 ? duty : 1;
	recompute();
}

void LFO8::recompute() {
	
	SQ9x22 dest;

	len_incline = period * duty;
	len_decline = period * (1.0 - duty);

	if (state == LFOStateIncline) {
		len = len_incline - phase;
		dest = 255;
	}
	else { 
		len = len_decline - phase;
		dest = 0;
	}

	if (len < MIN_RECOMP_LEN) 
		len = MIN_RECOMP_LEN;

	compute_slope(value, dest, len);	
}

void LFO8::compute_slope(SQ9x22 x0, SQ9x22 x1, int32_t new_len) {
	len = new_len;
	slope = x1 - x0;
	slope = static_cast<float>(slope) / (float)len;
}

uint16_t LFO8::render() {
	if (phase >= len) {
		if (state == LFOStateIncline) 
			begin_decline();
		else 
			begin_incline();
	}
	value += slope;
	int_value = (int16_t)value.getInteger();
	int_value = int_value > LFO8_LEV_MIN ? int_value : LFO8_LEV_MIN;
	int_value = int_value < LFO8_LEV_MAX ? int_value : LFO8_LEV_MAX;
	phase++;
	return int_value;
}

void LFO8::begin_incline() {
	phase = 0;
	state = LFOStateIncline;
	compute_slope(value, SQ9x22(255), len_incline);
}

void LFO8::begin_decline() {
	phase = 0;
	state = LFOStateDecline;
	compute_slope(value, SQ9x22(0), len_decline);
}

