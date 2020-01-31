//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Euclidean algorithm
//  Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
using namespace std;

#define EUCLID_SEQ_MAX_LEN	96

struct EuclideanAlgorithm {
	bool buffer[EUCLID_SEQ_MAX_LEN] = {};
	bool prevBuffer[EUCLID_SEQ_MAX_LEN] = {};

	bool hasChanged;
	int currentlength ;
	int currentHits;
	int currentShift;

	EuclideanAlgorithm() {
		reset();
	}
	
	// reset to the default values
	void reset() {
		currentlength = 0;
		currentHits = 0;
		currentShift = 0;
	}

	// retrieves the rhythm value from the current pattern for the given index taking the current shift position value into account
	bool pattern(int index) {

		int i = index;
		
		// if index is beyond the current length, we want nothing.
		if (i >= currentlength)
			return false;

		if (i < 0)
			i = currentlength - 1;

		i += currentShift;
		if (i >= currentlength)
			i -= currentlength;

		if (i < 0)
			i += currentlength;

		return buffer[i];
	}

	// sets the rhythm pattern using the given parameters
	// returns true if a change has been made
	bool set(int numHits, int totalLen, int shift) {
		
		hasChanged = false;
		
		// set the shift position
		if (shift != currentShift) {
			hasChanged = true;
			currentShift = shift;
			if (currentShift >= currentlength)
				currentShift = currentlength - 1;
			else if (currentShift <= -currentlength)
				currentShift = -(currentlength - 1);
		}
		
		// only recalculate if the length or number of hits are different to last time
		if (currentHits != numHits || currentlength != totalLen) {
			
			hasChanged = true;

			currentlength = totalLen;
			currentHits = numHits;
			
			// sanity check #1
			if (totalLen < 1 || numHits < 1) {
				for (int i = 0; i < EUCLID_SEQ_MAX_LEN; i++)
					buffer[i] = false;
				
				// nothing else to do
				return hasChanged;
			}

			// sanity check #2
			if (totalLen > EUCLID_SEQ_MAX_LEN)
				totalLen = EUCLID_SEQ_MAX_LEN;


			// sanity check #3
			if (currentHits > totalLen)
				currentHits = totalLen;

			// set up the initial pattern - all hits at the start
			int numRem = totalLen - currentHits;
			for (int i = 0; i < EUCLID_SEQ_MAX_LEN; i++)
				buffer[i] = (i < currentHits);

			int cNumHits = currentHits;
			int cNumRem = numRem;
			int cHitSpan = 1;
			int cRemSpan = 1;
			int cHitPos = 0;
			int cRemPos = 0;
			int nNumHits = 0;
			int nHitSpan = 1;
			int nRemSpan = 1;

			bool done = false;

			int p = 0; // current position in the bit pattern
			int h = 0; // hit counter
			int r = 0; // remainder counter

			while (cNumRem > 0) {
				// backup current bit pattern
				std::copy(buffer, buffer + totalLen, prevBuffer);

				p = 0;
				h = cNumHits;
				r = cNumRem;
				cHitPos = 0;
				cRemPos = cNumHits * cHitSpan;
				nNumHits = 0;
				done = false;

				while (p < totalLen) {
					if (h > 0) {
						for (int i = 0; i < cHitSpan; i++)
							buffer[p++] = prevBuffer[cHitPos++];

						h--;

						if (!done) {
							if (r == 1) {
								nNumHits = cNumRem;
								nHitSpan += cRemSpan;
								nRemSpan = cHitSpan;
								done = true;
							}
							else if (h == 0) {
								nNumHits = cNumHits;
								nHitSpan += cRemSpan;
								done = true;
							}
						}
					}

					if (r > 0) {
						for (int i = 0; i < cRemSpan; i++)
							buffer[p++] = prevBuffer[cRemPos++];

						r--;

						if (!done) {
							if (h == 0) {
								nNumHits = cNumHits;
								nHitSpan = cHitSpan;
								done = true;
							}
							else if (r == 0) {
								nNumHits = cNumRem;
								nHitSpan += cRemSpan;
								nRemSpan = cHitSpan;
								done = true;
							}
						}
					}
				}

				// reset the number of hit and remainder sequences
				cNumHits = nNumHits;
				cHitSpan = nHitSpan;


				// reset the individual sequence widths
				cRemSpan = nRemSpan;
				cNumRem = (totalLen - (cNumHits * cHitSpan)) / cRemSpan;

				// if either number of sequences is 1, we're done
				if (cNumHits == 1 || cNumRem <= 1)
					break;
			}
		}
		
		return hasChanged;
	}
};