//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Sequencer channel message
//	For passing sequence details to and from the standard 8/16 step sequencers 
//  and the sequencer channel expander module and gate expander modules
//  Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------

// utility macros 
#if SEQ_NUM_STEPS == 8
	#define isExpanderModule(x) x->model == modelSequencerChannel8 || x->model == modelSequencerGates8 || x->model == modelSequencerTriggers8 
	#define isExpandableModule(x) x->model == modelSequencer8 || x->model == modelSequencerChannel8 || x->model == modelSequencerGates8 || x->model == modelSequencerTriggers8 
#endif


#if SEQ_NUM_STEPS == 16
	#define isExpanderModule(x) x->model == modelSequencerChannel16 || x->model == modelSequencerGates16 || x->model == modelSequencerTriggers16 
	#define isExpandableModule(x) x->model == modelSequencer16 || x->model == modelSequencerChannel16 || x->model == modelSequencerGates16 || x->model == modelSequencerTriggers16 
#endif

struct SequencerChannelMessage {
	
	SequencerChannelMessage() {
		initialise();
	}
	
	int counter = 0;
	int length = SEQ_NUM_STEPS;
	bool clockState = false;
	bool runningState = false;
	int channel = 0;
	bool hasMaster = false;

	void set (int count, int len, bool cState, bool rState, int chan, bool isMaster) {
		counter = count;
		length = len;
		clockState = cState;
		runningState = rState;
		channel = chan;
		hasMaster = isMaster;
	}
	
	void initialise() {
		counter = 0;
		length = SEQ_NUM_STEPS;
		clockState = false;
		runningState = false;
		channel = 0;
		hasMaster = false;
	}
};

