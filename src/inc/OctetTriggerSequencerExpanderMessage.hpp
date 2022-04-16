//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Octet Trigger Sequencer expander message
//	For passing sequence details to and from the Octet trigger sequencer 
//  and the octet sequencer expander modules
//  Copyright (C) 2021  Adam Verspaget
//----------------------------------------------------------------------------

// utility macros 
#define isExpanderModule(x) x->model == modelOctetTriggerSequencerCVExpander || x->model == modelOctetTriggerSequencerGateExpander 
#define isExpandableModule(x) x->model == modelOctetTriggerSequencer || x->model == modelNibbleTriggerSequencer || x->model == modelOctetTriggerSequencerCVExpander || x->model == modelOctetTriggerSequencerGateExpander

#define CHANNEL_A 0
#define CHANNEL_B 1

#define DEFAULT_PATTERN_A 170
#define DEFAULT_PATTERN_B 136

// count to bit mappping
#define STEP_MAP const int stepMap[9] = {0, 128, 64, 32, 16, 8, 4, 2, 1}

struct OctetTriggerSequencerExpanderMessage {
	
	OctetTriggerSequencerExpanderMessage() {
		initialise();
	}
	
	int counter = 0;
	bool clockEdge = false;
	int selectedPatternA = 0;
	int selectedPatternB = 0;
	int channel = 0;
	bool hasMaster = false;
	bool playingChannelB =false;
	bool chained = false;
	int chainedPatternMode = 0;
	int processCount = 0;
	bool gateA = false;
	bool gateB = false;

	void set (int count, bool clkEdge, int patternA, int patternB, int chan, bool isMaster, bool playingB, bool chain, int patternMode, int pCount, bool gA, bool gB) {
		counter = count;
		clockEdge = clkEdge;
		selectedPatternA = patternA;
		selectedPatternB = patternB;
		channel = chan;
		hasMaster = isMaster;
		playingChannelB = playingB;
		chained = chain;
		chainedPatternMode = patternMode;
		processCount = pCount;
		gateA = gA;
		gateB = gB;
	}
	
	void initialise() {
		counter = 0;
		clockEdge = false;
		selectedPatternA = 0;
		selectedPatternB = 0;
		channel = 0;
		hasMaster = false;
		playingChannelB =false;
		chained = false;
		chainedPatternMode = 0;
		processCount = 0;
		gateA = false;
		gateB = false;		
	}
};

