//----------------------------------------------------------------------------
//	Count Modula - Clock Oscillator
//	A version of the VCV Rack Fundamental LFO offering Square/Pulse only
//----------------------------------------------------------------------------
#pragma once
#include "dsp/digital.hpp"

struct ClockOscillator {
	float phase = 0.0f;
	float pw = 0.5f;
	float freq = 1.0f;

	SchmittTrigger resetTrigger;

	ClockOscillator() {}
	
	void setPitch(float pitch) {
		pitch = fminf(pitch, 10.0f);
		freq = powf(2.0f, pitch);
	}
	
	void setPulseWidth(float pw_) {
		const float pwMin = 0.01f;
		pw = clamp(pw_, pwMin, 1.0f - pwMin);
	}
	
	void reset() {
		phase = 0.0f;
		resetTrigger.reset();
	}
	
	void step(float dt) {
		float deltaPhase = fminf(freq * dt, 0.5f);
		phase += deltaPhase;
		if (phase >= 1.0f)
			phase -= 1.0f;
	}
	
	float sqr() {
		float sqr = (phase < pw) ? 1.0f : -1.0f;
		return sqr;
	}
};