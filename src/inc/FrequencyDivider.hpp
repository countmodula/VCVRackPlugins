//------------------------------------------------------------------------
//  /^M^\ Count Modula Plugin for VCV Rack - Frequency Divider
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#pragma once
#include "GateProcessor.hpp"

#define COUNT_UP 1
#define COUNT_DN 2

struct FrequencyDivider {
	int count = 0;
	int N = 0;
	int maxN = 20;
	int countMode = COUNT_DN;
	
	bool phase = false;
	GateProcessor gate;

	// process the given clock value and return the current divider state
	bool process(float clk) {

		// process the clock;
		gate.set(clk);
		
		if (gate.anyEdge()) {
				count++;

				// for count up mode, flip the phase and reset the count at the end of the count
				if ((countMode == COUNT_UP && count == N))
					phase = !phase;
				
				if (count >= N) {
					// we've hit the counter
					count = 0;
				}
		
			// for count down mode, flip the phase and reset the count at the start
			if ((countMode == COUNT_DN && count == 0))
				phase = !phase;
		}
		
		return phase;
	}
	
	void setN (int in) {
		N = clamp(in, 1, maxN);
	}
	
	// set the counter mode to up or down. This controls whether the phase changes on the first clock or the last clock of the count
	void setCountMode(int mode) {
		switch(mode) {
			case COUNT_DN:
			case COUNT_UP:
				countMode = mode;
				break;
		}
	}
	
	// set the maximum division value - limited to 1-64 as I don't see the point in larger divisions at the moment
	void setMaxN(int max) {
		maxN = max;
		if (maxN < 1)
			maxN = 1;
		else if (maxN > 64)
			maxN = 64;
	}
	
	// reset the counter
	void reset () {
		countMode = COUNT_DN;
		count = -1;
		N = 0;
		phase = false;
		gate.reset();
	}
};

struct FrequencyDividerOld {
	int count = 0;
	int N = 0;
	int maxN = 20;
	int countMode = COUNT_UP;
	
	bool phase = false;
	GateProcessor gate;
	
	bool process(float clk) {

		// process the clock;
		gate.set(clk);
		
		if (N == 0) {
			// divide by 1...
			count = 0;
			phase = gate.state();
		}
		else {
			// divide by 2 or more, advance clock on the leading edge
			if (gate.leadingEdge()) {
				if (countMode == COUNT_DN) {
					// decrement the counter
					count--;

					if (count <= 0) {
						// we've hit the counter, flip the phase and reset the count
						count = N;
						phase = !phase;
					}
				}
				else {
					// increment the counter
					count++;

					if (count >= N) {
						// we've hit the counter, flip the phase and reset the count
						count = 0;
						phase = !phase;
					}
				}
			}
		}
		
		return phase;
	}
	
	// set N based on the given voltage and current CV scale
	void setN (float in) {
		// scale the 0 - 10V input to 0 to the maximum division value
		N = (int)(clamp(in, 0.0f, 10.0f) * (((float)(maxN))/ 10.0f));
		if (N > maxN)
			N = maxN;
	}
	
	// set the counter mode to up or down. This controls whether the phase changes on the first clock or the last clock of the count
	void setCountMode(int mode) {
		switch(mode) {
			case COUNT_DN:
			case COUNT_UP:
				countMode = mode;
				break;
		}
	}
	
	// set the maximum division value - limited to 1-64 as I don't see the point in larger divisions at the moment
	// this controls the scale of the control voltage 0-10 = divide by 1 to this value
	void setMaxN(int max) {
		maxN = max;
		if (maxN < 0)
			maxN = 0;
		else if (maxN > 63)
			maxN = 63;
	}
	
	// reset the counter
	void reset () {
		count = 0;
		N = 0;
		phase = false;
		gate.reset();
	}
};

