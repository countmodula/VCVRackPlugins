//----------------------------------------------------------------------------
//	/^M^\ Count Modula - Mixer Engine
//	Basic 4 input bipolar/unipolar mixer
//----------------------------------------------------------------------------
struct MixerEngine {
	float overloadLevel = 0.0f;
	float mixLevel = 0.0f;
	bool limitToRails = true;
	
	float process (float input1, float input2, float input3, float input4, float inputLevel1, float inputLevel2, float inputLevel3, float inputLevel4, float outputLevel, float mode) {
	
		float level1 = inputLevel1;
		float level2 = inputLevel2;
		float level3 = inputLevel3;
		float level4 = inputLevel4;
		
		// convert to unipolar
		if (mode > 0.5) {
			level1 = (clamp(inputLevel1, 0.0f, 1.0f) * 2.0f) - 1.0f;
			level2 = (clamp(inputLevel2, 0.0f, 1.0f) * 2.0f) - 1.0f;
			level3 = (clamp(inputLevel3, 0.0f, 1.0f) * 2.0f) - 1.0f;
			level4 = (clamp(inputLevel4, 0.0f, 1.0f) * 2.0f) - 1.0f;
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
