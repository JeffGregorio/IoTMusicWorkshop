#include "Sequencer.h"

SEQ8::SEQ8() :
gated(false), value(SQ9x22(SEQ8_DFLT_VALUE)), slope(SQ9x22(0)), int_value(SEQ8_DFLT_VALUE),
uniform_step(true), steplen(SEQ8_DFLT_LEN),
n_steps(0), step_idx(0), phase(0), len(SEQ8_DFLT_LEN), glidelen(SEQ8_DFLT_GLIDE) {
	for (int i = 0; i < SEQ8_MAX_STEPS; i++) {
		steps[i] = seq_step_t(SEQ8_DFLT_VALUE, SEQ8_DFLT_LEN);
	}
}

SEQ8::~SEQ8() {

}

void SEQ8::set_step_length(uint32_t length) {
	length = length < SEQ8_LEN_MAX ? length : SEQ8_LEN_MAX;
	steplen = length;
}

void SEQ8::set_glide_length(uint32_t length) {
	length = length < SEQ8_LEN_MAX ? length : SEQ8_LEN_MAX;
	glidelen = length;
}


void SEQ8::append_step(uint8_t value) {
	append_step(value, steplen);
}

void SEQ8::append_step(uint8_t value, int32_t length) {
	steps[n_steps++] = seq_step_t(value, length);
}

void SEQ8::set_step(uint16_t step_idx, uint8_t value) {
	set_step(step_idx, value, steplen);
}

void SEQ8::set_step(uint16_t idx, uint8_t value, int32_t length) {
	if (idx < 0 || idx >= n_steps)
		return;
	steps[idx].value = SQ9x22(value);
	steps[idx].length = length;
	// If we're modifying the current step, recompute the slope
	if (idx == step_idx) {
		compute_slope(value, steps[idx].value, glidelen);
		len = steps[idx].length - phase;
		len = len > 1 ? len : 1;
	}
}

uint16_t SEQ8::render() {
	if (n_steps == 0)
		return SEQ8_DFLT_VALUE;
	if (!gated)
		return int_value;
	if (phase >= len) 
		next();
	if (phase < glidelen) {
		value += slope;
		int_value = (int16_t)value.getInteger();
		int_value = int_value > SEQ8_LEV_MIN ? int_value : SEQ8_LEV_MIN;
		int_value = int_value < SEQ8_LEV_MAX ? int_value : SEQ8_LEV_MAX;
	}
	phase++;
	return int_value;
}


void SEQ8::next() {
	step_idx++;
	if (step_idx >= n_steps) {
		step_idx = 0;
		if (eos_handler)
			eos_handler(eos_userdata);
	}
	if (uniform_step)
		len = steplen;
	else
		len = steps[step_idx].length;		
	compute_slope(value, steps[step_idx].value, glidelen);
	phase = 0;
}

void SEQ8::compute_slope(SQ9x22 x0, SQ9x22 x1, int32_t len) {
	slope = x1 - x0;
	slope = static_cast<float>(slope) / (float)glidelen;
}