//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Megalomaniac expander message
//	For passing control details from the Megalomaniac expander to the HMLFO
//  Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------

// utility macros 
#define isControllerModule(x) x->model == modelMegalomaniac
#define isControllableModule(x) x->model == modelHyperManiacalLFO

struct MegalomaniacControllerMessage {
	int selectedWaveform[6] = {};
	int selectedRange[6] = {};
	float mixLevel[6] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
	float fmValue[6] = {};
};

