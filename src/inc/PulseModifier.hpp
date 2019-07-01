//----------------------------------------------------------------------------
//	/^M^\ Count Modula - Pulse modifier
//	Modify the length of a pulse
//----------------------------------------------------------------------------

struct PulseModifier {
	// timer stop indicator - high value avoids double triggering if the time is increasing from the lowest value
	const float stop = 9999.0f;
	
	const float maxLength = 100.0f;
	const float minLength = 0.0f;
	
	float timer = stop;
	float length = minLength;
	
	float state = false;
	
	// process the gate based on the elapsed time
	bool process(float deltaTime) 
	{
		if (timer < length) {
			// we're counting up to the current length
			timer += deltaTime;
			state = true;
		}
		else {
			// we've reached the desired length of time - set to a high value to avoid double triggering if the time is increasing
			timer = stop;
			state = false;
		}
		
		return state;
	}
	
	// restart the timer
	void restart() {
		timer = 0.0f;
		state = true;
	}
	
	// set the gate time
	void set(float l) {
		length = fminf(fmaxf(minLength, l), maxLength);
	}
	
	// reset and stop the timer
	void reset() {
		timer = stop;
		state = false;
		length = minLength;
	}
	
	bool getState(){
		return state;
	}
};
