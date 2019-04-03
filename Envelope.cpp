#include "Envelope.h"

#define ADSR8_DFLT_LEN 160
#define ADSR8_DFLT_SUS 127

ADSR8::ADSR8() :
state(ADSRStateIdle), int_value(0), value(0), slope(0), phase(0), len(0), 
atk_len(ADSR8_DFLT_LEN), dec_len(ADSR8_DFLT_LEN), 
sus_lev(ADSR8_DFLT_SUS), rel_len(ADSR8_DFLT_LEN),
retrigger(false),
eod_handler(NULL), eod_userdata(NULL), eor_handler(NULL), eor_userdata(NULL) {

}

ADSR8::~ADSR8() {

}

void ADSR8::set_attack(uint32_t p_len) {
	atk_len = p_len < ADSR8_LEN_MAX ? p_len : ADSR8_LEN_MAX;
	if (state == ADSRStateAttack) {
		len = atk_len - phase;
		len = len > MIN_RECOMP_LEN ? len : MIN_RECOMP_LEN;
		compute_slope(value, ATK_LEVEL, len);
	}
}

void ADSR8::set_decay(uint32_t p_len) {
	dec_len = p_len < ADSR8_LEN_MAX ? p_len : ADSR8_LEN_MAX;
	if (state == ADSRStateDecay) {
		len = dec_len - phase;
		len = len > MIN_RECOMP_LEN ? len : MIN_RECOMP_LEN;
		compute_slope(value, sus_lev, len);
	}
}

void ADSR8::set_sustain(uint8_t p_lev) {
	sus_lev = p_lev < ADSR8_LEV_MAX ? p_lev : ADSR8_LEV_MAX;
	if (state == ADSRStateDecay) {
		len = dec_len - phase;
		len = len > MIN_RECOMP_LEN ? len : MIN_RECOMP_LEN;
		compute_slope(value, sus_lev, len);
	}
	else if (state == ADSRStateSustain) {
		begin_sustain_adjust();
	}
}

void ADSR8::set_release(uint32_t p_len) {
	rel_len = p_len < ADSR8_LEN_MAX ? p_len : ADSR8_LEN_MAX;
	if (state == ADSRStateRelease) {
		len = rel_len - phase;
		len = len > MIN_RECOMP_LEN ? len : MIN_RECOMP_LEN;
		compute_slope(value, REL_LEVEL, len);
	}
}

void ADSR8::compute_slope(SQ9x22 x0, SQ9x22 x1, int32_t new_len) {
	len = new_len;
	slope = x1 - x0;
	slope = static_cast<float>(slope) / (float)len;
}

uint16_t ADSR8::render() {
	if (state == ADSRStateIdle) {
		return 0;
	}
	if (state == ADSRStateSustain) {
		return (int16_t)sus_lev.getInteger();
	}
	if (phase >= len) {
		switch (state) {
			case ADSRStateAttack:
				begin_decay();
				break;
			case ADSRStateDecay:
				if (retrigger) 	
					begin_attack();
				else 			
					begin_sustain();				
				if (eod_handler)
					eod_handler(eod_userdata);
				break;
			case ADSRStateSustainAdjust:
				begin_sustain();
				break;
			case ADSRStateRelease:
				begin_idle();
				if (eor_handler)
					eor_handler(eor_userdata);
				break;
			default:
				break;		
		}
	}
	value += slope;
	int_value = (int16_t)value.getInteger();
	int_value = int_value > ADSR8_LEV_MIN ? int_value : ADSR8_LEV_MIN;
	int_value = int_value < ADSR8_LEV_MAX ? int_value : ADSR8_LEV_MAX;
	phase++;
	return int_value;
}

void ADSR8::begin_idle() {
	phase = 0;
	state = ADSRStateIdle;
}

void ADSR8::begin_attack() {
	phase = 0;
	state = ADSRStateAttack;
	compute_slope(value, ATK_LEVEL, atk_len);
}

void ADSR8::begin_decay() {
	phase = 0;
	state = ADSRStateDecay;
	compute_slope(value, sus_lev, dec_len);
}

void ADSR8::begin_sustain() {
	phase = 0;
	state = ADSRStateSustain;
}

void ADSR8::begin_sustain_adjust() {
	phase = 0;
	state = ADSRStateSustainAdjust;
	compute_slope(value, sus_lev, MIN_RECOMP_LEN);
}

void ADSR8::begin_release() {
	phase = 0;
	state = ADSRStateRelease;
	compute_slope(value, REL_LEVEL, rel_len);
}

	