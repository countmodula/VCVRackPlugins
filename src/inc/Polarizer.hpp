//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Polarizer
//	Basic polarizing functionality
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#pragma once
struct Polarizer {
	float positiveLevel = 0.0f;
	float negativeLevel = 0.0f;
	
	float process(float in, float manual, float cv, float cvLevel) {

		float multiplier = manual + (cv * cvLevel / 2.0f);
		float out = in * multiplier;
		out = clamp(out, -10.0f, 10.0f); 
			
		positiveLevel = multiplier > 0.0f ? fminf(multiplier, 1.0f) : 0.0f;
		negativeLevel = multiplier < 0.0f ? fminf(-multiplier, 1.0f) : 0.0f;

		return out;		
	}
	
	void reset() {
		positiveLevel = 0.0f;
		negativeLevel = 0.0f;
	}
};