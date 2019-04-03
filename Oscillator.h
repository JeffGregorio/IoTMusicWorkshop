/*
 *	Oscillator.h
 */
#ifndef OSCILLATOR_H
#define OSCILLATOR_H

#include <FixedPoints.h>
#include <FixedPointsCommon.h>

#define LFO8_LEN_MAX 2147483647
#define LFO8_LEV_MAX 255
#define LFO8_LEV_MIN 0

using SQ9x22 = SFixed<9, 22>;

class LFO8 {

	// Minimum number of samples used to recompute the length of a state when
	// the period is changed during the state
	const int MIN_RECOMP_LEN = 8;

	enum {
		LFOStateIncline = 0,
		LFOStateDecline
	};

public:

	LFO8();
	~LFO8();

	void set_period(uint32_t period_samples);
	void set_duty_cycle(float duty_norm);
	uint16_t render();

protected:

	void compute_slope(SQ9x22 x0, SQ9x22 x1, int32_t new_len);
	void recompute();
	void begin_incline();
	void begin_decline();

	int32_t period;
	float duty;
	uint8_t state;

	int16_t int_value;
	SQ9x22 value;
	SQ9x22 slope;
	SQ9x22 slope_incline;
	SQ9x22 slope_decline;
	
	int32_t phase;
	int32_t len;
	int32_t len_incline;
	int32_t len_decline;
};


#endif