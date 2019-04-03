/*
 *	Sequencer.h
 */
#ifndef SEQ8_H
#define SEQ8_H

#include <FixedPoints.h>
#include <FixedPointsCommon.h>

#define SEQ8_LEN_MAX 2147483647
#define SEQ8_LEV_MAX 255
#define SEQ8_LEV_MIN 0

// Allow user redefinition of max step array size
#ifndef SEQ8_MAX_STEPS
#define SEQ8_MAX_STEPS 512
#endif

// Defaults
#define SEQ8_DFLT_GLIDE 16
#define SEQ8_DFLT_VALUE 0
#define SEQ8_DFLT_LEN 1

using SQ9x22 = SFixed<9, 22>;

// Sequencer class
class SEQ8 {

	// Sequencer step type
	typedef struct seq_step_t {
		SQ9x22 value;				// Destination value
		int32_t length;				// Step length
		// Constructors
		seq_step_t() : value(SQ9x22(SEQ8_DFLT_VALUE)), length(SEQ8_DFLT_LEN) {}
		seq_step_t(uint8_t val, int32_t len) 
		: value(SQ9x22(val)), length(len) {}
	} seq_step_t;

public:

	// Constructor/destructor
	SEQ8();
	~SEQ8();

	// Set uniform step length (does not overwrite individual step lengths)
	void set_step_length(uint32_t length);
	void set_glide_length(uint32_t length);

	// Add a step with the current length or specific length
	void append_step(uint8_t value);
	void append_step(uint8_t value, int32_t length);

	// Set value/length of a specific step
	void set_step(uint16_t idx, uint8_t value);
	void set_step(uint16_t idx, uint8_t value, int32_t length);

	// Clear steps (does not actually erase existing steps, just ignores them)
	void clear()			{ n_steps = 0; }

	// Activate/deactivate
	void gate(bool is_high)	{ gated = is_high; }

	// Reset sequencer to step 0
	void reset() 			{ phase = len; step_idx = n_steps; }

	// Main render method
	uint16_t render();

	// Setter for user callback on end of sequence
	void set_eos_handler(void (*handler)(void *), void *userdata) {
		eos_handler = handler;
		eos_userdata = userdata;
	}

	// Get number of steps
	uint16_t num_steps()	{ return n_steps; }

	// Directly settable parameter(s)
	bool uniform_step;	// Whether to use individual step length or uniform length

protected:

	void compute_slope(SQ9x22 x0, SQ9x22 x1, int32_t len);

	void next();

	bool gated;		// Whether the sequencer gate is set high

	SQ9x22 value;				// Current value
	SQ9x22 slope;				// Current slope
	int16_t int_value;			// Current value, casted and constrained to [0, 255]
	int32_t	steplen;			// Uniform length for all steps (if used)			

	seq_step_t steps[SEQ8_MAX_STEPS];		// Step array
	uint16_t n_steps;						// Number of steps added
	uint16_t step_idx;						// Current step index			
	
	uint32_t phase;		// Current phase in current step
	int32_t len;		// Length of current step
	int32_t glidelen;	// Glide time

	void (*eos_handler)(void *);	// User callback for end of sequence
	void *eos_userdata;				// - its userdata
};

#endif