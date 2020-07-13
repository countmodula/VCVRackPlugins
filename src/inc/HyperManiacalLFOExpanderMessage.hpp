//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Hyper Maniacal LFO expander message
//	For passing LDO details from the Hyper Maniacal LFO to the outpu expander
//  Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------

// utility macros 
#define isExpanderModule(x) x->model == modelHyperManiacalLFOExpander
#define isExpandableModule(x) x->model == modelHyperManiacalLFO

struct HyperManiacalLFOExpanderMessage {
	float sin[8] = {};
	float saw[8] = {};
	float tri[8] = {};
	float sqr[8] = {};
	float lig[8] = {};
	float frq[8] = {};
	bool unipolar;
};

