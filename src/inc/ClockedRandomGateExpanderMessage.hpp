//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Sequencer expander message
//	For passing sequence details to and from sequencer expander modules
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------

#define CRG_EXP_NUM_CHANNELS 8

// utility macros 
#define isExpanderModule(x) x->model == modelClockedRandomGateExpanderCV || x->model == modelClockedRandomGateExpanderLog
#define isExpandableModule(x) x->model == modelClockedRandomGates

struct ClockedRandomGateExpanderMessage {
	
	bool singleMode;
	bool isPolyphonic;
	
	int numPolyChannels = 1;
	
	bool gateStates[CRG_EXP_NUM_CHANNELS] =  {};
	bool clockStates[CRG_EXP_NUM_CHANNELS] =  {};
	bool triggerStates[CRG_EXP_NUM_CHANNELS] =  {};
};

