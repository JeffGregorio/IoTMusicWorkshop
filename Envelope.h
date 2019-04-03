/*
 *	Envelope.h
 */
#ifndef ADSR_H
#define ADSR_H

#include <FixedPoints.h>
#include <FixedPointsCommon.h>

#define ADSR8_LEN_MAX 2147483647
#define ADSR8_LEV_MAX 255
#define ADSR8_LEV_MIN 0

using SQ9x22 = SFixed<9, 22>;

class ADSR8 {

	// Minimum number of samples used to recompute the length of a state when
	// the length is changed during the state
	const int MIN_RECOMP_LEN = 8;
	const SQ9x22 ATK_LEVEL = ADSR8_LEV_MAX;
	const SQ9x22 REL_LEVEL = ADSR8_LEV_MIN;

	enum {
		ADSRStateIdle = 0,
		ADSRStateAttack,
		ADSRStateDecay,
		ADSRStateSustain,
		ADSRStateSustainAdjust,
		ADSRStateRelease
	};

public:

	// Constructor/destructor
	ADSR8();
	~ADSR8();

	// Parameter setters
	void set_attack(uint32_t len);
	void set_decay(uint32_t len);
	void set_sustain(uint8_t level);
	void set_release(uint32_t len);
	void set_retrigger(bool retrig)	{ retrigger = retrig; }

	// Setters for user callbacks on end of Decay and Release
	void set_eod_handler(void (*handler)(void *), void *userdata) {
		eod_handler = handler;
		eod_userdata = userdata;
	}
	void set_eor_handler(void (*handler)(void *), void *userdata) {
		eor_handler = handler;
		eor_userdata = userdata;
	}

	// Gate
	void gate(bool is_high) {
		if (is_high) 	begin_attack();
		else			begin_release();
	}

	// Render
	uint16_t render();

protected:

	// Compute a slope to reach x1 from x0 in new_len samples; 
	// sets current state length
	void compute_slope(SQ9x22 x0, SQ9x22 x1, int32_t new_len);

	// Begin states
	void begin_idle();
	void begin_attack();
	void begin_decay();
	void begin_sustain();
	void begin_sustain_adjust();
	void begin_release();

	uint8_t state;					// Current state

	SQ9x22 value;					// Current fixed point value
	SQ9x22 slope;					// Current fixed point slope
	int16_t int_value;				// Current value, casted and constrained to [0, 255]
	
	int32_t phase;					// Current state's phase
	int32_t len;					// Current state's length

	int32_t atk_len;				// Attack state length
	int32_t dec_len;				// Decay state length
	SQ9x22 sus_lev;					// Sustain state target level
	int32_t rel_len;				// Release state length

	bool retrigger;					// Whether to retrigger attack on end of decay

	void (*eod_handler)(void *);	// User callback for end of decay
	void *eod_userdata;				// - its userdata
	void (*eor_handler)(void *);	// User callback for end of release
	void *eor_userdata;				// - its userdata
};

#endif