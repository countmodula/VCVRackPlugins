//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Mixer Engine
//	Basic 4 input bipolar/unipolar mixer
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
struct MixerEngine {
	float overloadLevel = 0.0f;
	float mixLevel = 0.0f;
	bool limitToRails = true;
	
	float level1 = 0.0f;
	float level2 = 0.0f;
	float level3 = 0.0f;
	float level4 = 0.0f;
	
	float process (float input1, float input2, float input3, float input4, float inputLevel1, float inputLevel2, float inputLevel3, float inputLevel4, float outputLevel, bool bipolar) {

		// convert to unipolar
		if (bipolar) {
			level1 = (inputLevel1 * 2.0f) - 1.0f;
			level2 = (inputLevel2 * 2.0f) - 1.0f;
			level3 = (inputLevel3 * 2.0f) - 1.0f;
			level4 = (inputLevel4 * 2.0f) - 1.0f;
		}
		else {
			level1 = inputLevel1;
			level2 = inputLevel2;
			level3 = inputLevel3;
			level4 = inputLevel4;
		}
		
		float out = outputLevel * ((input1 * level1) +
									(input2 * level2) +
									(input3 * level3) +
									(input4 * level4));
		
		overloadLevel =  (fabs(out) > 10.0f) ? 1.0f : 0.0f;
		
		// TODO: saturation rather than hard limit although my hardware mixers just clip
		if (limitToRails)
			out = clamp(out, -12.0f, 12.0f);
		
		mixLevel = fminf(fabs(out) / 10.0f, 1.0f);

		return out;
	}
};
