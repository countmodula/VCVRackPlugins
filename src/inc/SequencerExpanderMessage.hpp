//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Sequencer expander message
//	For passing sequence details to and from sequencer expander modules
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------

#define SEQUENCER_EXP_MAX_CHANNELS 4

#define SEQUENCER_EXP_MASTER_MODULE_DEFAULT 0
#define SEQUENCER_EXP_MASTER_MODULE_BINARY 1
#define SEQUENCER_EXP_MASTER_MODULE_DUALSTEP 2
#define SEQUENCER_EXP_MASTER_MODULE_GATEDCOMPARATOR 3
#define SEQUENCER_EXP_MASTER_MODULE_TRIGGER8 4
#define SEQUENCER_EXP_MASTER_MODULE_TRIGGER16 5
#define SEQUENCER_EXP_MASTER_MODULE_BASIC 6
#define SEQUENCER_EXP_MASTER_MODULE_BURSTGENERATOR 7

#define SEQUENCER_EXP_NUM_TRIGGER_OUTS 8

// utility macros 
#define isExpanderModule(x) x->model == modelSequencerExpanderCV8 || x->model == modelSequencerExpanderOut8 || x->model == modelSequencerExpanderTrig8 || x->model == modelSequencerExpanderRM8 || x->model == modelSequencerExpanderLog8 || x->model == modelSequencerExpanderTSG
#define isExpandableModule(x) x->model == modelTriggerSequencer8 || x->model == modelStepSequencer8 || x->model == modelBinarySequencer || x->model == modelBasicSequencer8 || x->model == modelBurstGenerator || x->model == modelGatedComparator

struct SequencerExpanderMessage {
	
	// define the identifiers to be used in the channel management array
	enum expanderIdentifiers {
		CV8,
		OUT8,
		TRIG8,
		RM8,
		LOG8,
		NUM_EXPANDERS
	};	
	
	int channels[NUM_EXPANDERS] = {};
	int masterModule;
	
	// for sequencers with multiple channels
	int counters[SEQUENCER_EXP_MAX_CHANNELS] = {};
	bool clockStates[SEQUENCER_EXP_MAX_CHANNELS] = {};
	bool runningStates[SEQUENCER_EXP_MAX_CHANNELS] = {};
	
	// for the trigger sequencer expander
	bool gateStates[SEQUENCER_EXP_NUM_TRIGGER_OUTS] = {};
	
	// set the default values to be used if there is no left hand module
	void setDefaultValues() {
		setAllChannels(-1);
		
		masterModule = SEQUENCER_EXP_MASTER_MODULE_DEFAULT;
	}
	
	// set the value for all expander channels - used in the master modules
	void setAllChannels(int c)
	{
		for (int i = 0; i < NUM_EXPANDERS; i++)
			channels[i] = c;
	}
	
	// set the channel value for the given id
	void setChannel(int c, int id) {
		channels[id] = c;
	}
	
	// set the next channel for the given id
	void setNextChannel(int c, int id) {
		channels[id] = c;
		
		if (channels[id] >= 0)
			channels[id]++;
			
		// wrap the channels around in case anyone is crazy enough to add more expanders than we expect
		if (channels[id] >= SEQUENCER_EXP_MAX_CHANNELS)
			channels[id]  = 0;
	}
		
	// sets the clock state
	void setClockState(int c, bool cs) {
		clockStates[c] = cs;
	}
};

// custom channel indicator
struct CountModulaLightRGYB : GrayModuleLightWidget {
	CountModulaLightRGYB() {
		addBaseColor(SCHEME_RED);
		addBaseColor(SCHEME_GREEN);
		addBaseColor(SCHEME_YELLOW);
		addBaseColor(SCHEME_BLUE);
	}
};