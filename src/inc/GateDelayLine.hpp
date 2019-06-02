//----------------------------------------------------------------------------
//	/^M^\	Count Modula - Multi-tapped Gate Delay Module
//	A shift register style gate delay offering a number of tapped gate outputs 
//----------------------------------------------------------------------------
#pragma once

#include <queue>
#include <deque>
#include "GateProcessor.hpp"

struct GateDelayLine {
	std::deque<unsigned long> bitDeque{std::deque<unsigned long>(1024, 0.0)};
	std::queue<unsigned long> bitStack{std::queue<unsigned long>(bitDeque)};
	
	unsigned long gateOutputs;
	float time;
	float delay;
	
	GateProcessor gate;
	
	GateDelayLine() {
		gateOutputs = 0;
		time = 0.0f;
		delay = 0.001f;
	}
	
	// processes the given gate value and delay time, clocking the delay as required
	bool process(float gateValue, float delayTime) 
	{
		// Determine gate level at input
		gate.set(gateValue);
		
		// make the delay time sensible
		delay = clamp(delayTime, 0.001f, 10.0f);
		
		// number of ticks required to achieve the delay time required
		float timePerTick = delay / 8192.0f;
		
		// calculate elapsed time since last tick
		time += engineGetSampleTime();
		
		// if we've exceed the required time, clock the delay line
		if (time >= timePerTick) {
			enqueue(gate.state());	
			time = 0.0f;
		}
		
		// give us back the input gate value
		return gate.high();
	}
	
	// add the given gate value to the queue and return the delayed gate value 
	void enqueue(bool gateInput) {
		unsigned long b = 0;
		
		// grab the top value and remove it - this is out output value with the last but being the most delayed
		gateOutputs = b = bitStack.front();
		bitStack.pop();

		//  shift the old bits across - they'll keep passing through until they drop off
		b = b << 1;

		// set the new input bit if we need to
		if (gateInput)
			b |= 0x01;

		// add it back into the stack
		bitStack.push(b);
	}
	
	// grabs the output value for the given tap in the delay line
	bool tapValue(unsigned int tapNo) {
		unsigned long bitmask = 1 << (tapNo - 1);
		return (gateOutputs & bitmask);
	}
	
	// gets the currently input gate value
	bool gateValue() {
		return gate.state();
	}
	
	void reset() {
		// fill the queue with nothing.
		for (int i = 0; i < 1024; i++) {
			bitStack.pop();
			bitStack.push(0L);
		}
	}
};

