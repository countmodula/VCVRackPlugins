//----------------------------------------------------------------------------
//	Count Modula - Slew Limiter
//	Basic Up/Down Slew limiting functionality based on the Befaco Slew Limiter
//	By Andrew Belt.
//----------------------------------------------------------------------------
#pragma once

struct LagProcessor {

	float out = 0.0f;

	float process(float in, float shape, float rise, float fall) {

		// minimum and maximum slopes in volts per second
		const float slewMin = 0.1;
		const float slewMax = 10000.0;

		
		// Amount of extra slew per voltage difference
		const float shapeScale = 1/10.0;

		// Rise
		if (in > out) {
			float slew = slewMax * powf(slewMin / slewMax, rise);
			out += slew * crossfade(1.0f, shapeScale * (in - out), shape) * APP->engine->getSampleTime();
			if (out > in)
				out = in;
		}
		// Fall
		else if (in < out) {
			float slew = slewMax * powf(slewMin / slewMax, fall);
			out -= slew * crossfade(1.0f, shapeScale * (out - in), shape) * APP->engine->getSampleTime();
			if (out < in)
				out = in;
		}

		return out;
	}
	
	void reset() {
		out = 0.0f;
	}
};
