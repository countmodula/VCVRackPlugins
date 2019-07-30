//----------------------------------------------------------------------------
//	/^M^\ Count Modula - Clock Oscillator
//	A version of the VCV Rack Fundamental LFO offering Square/Pulse only
//----------------------------------------------------------------------------
#pragma once

struct ClockOscillator {
	float phase = 0.0f;
	float pw = 0.5f;
	float freq = 1.0f;

	ClockOscillator() {}
	
	void setPitch(float pitch) {
		pitch = fminf(pitch, 10.0f);
		freq = powf(2.0f, pitch);
	}
	
	void setPitchHigh(float pitch) {
		pitch = fminf(pitch, 14.5f);
		freq = powf(2.0f, pitch);
	}
	
	void setPulseWidth(float pw_) {
		const float pwMin = 0.01f;
		pw = clamp(pw_, pwMin, 1.0f - pwMin);
	}
	
	void reset() {
		phase = 0.0f;
	}
	
	void step(float dt) {
		float deltaPhase = fminf(freq * dt, 0.5f);
		phase += deltaPhase;
		if (phase >= 1.0f)
			phase -= 1.0f;
	}
	
	float sqr() {
		return (phase < pw) ? 1.0f : -1.0f;
	}
	
	bool high() {
		return (phase < pw);
	}
};