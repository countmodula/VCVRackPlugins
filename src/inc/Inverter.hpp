//----------------------------------------------------------------------------
//	/^M^\ Count Modula - Logical inverter
//	Schmitt Trigger controlled logical inverter
//----------------------------------------------------------------------------
#pragma once

struct Inverter {
	dsp::SchmittTrigger i;
	dsp::SchmittTrigger e;
	
	bool isHigh = true;
	bool isEnabled = false;
	
	float process(float in) {
		return process(in, 10.0f);
	}
	
	float process(float in, float enable) {
		i.process(in);
		e.process(enable);
		
		isEnabled = e.isHigh();
		
		// invert or not based on the enable state
		isHigh  = isEnabled ? !i.isHigh() : i.isHigh();
		return isHigh ? 10.0f : 0.0f;
	}
	
	void reset() {
		i.reset();
		e.reset();
		isHigh = true;
		isEnabled = false;
	}
};