//----------------------------------------------------------------------------
//	/^M^\ Count Modula - Sequencer expander message
//	For passing sequence details to and from sequencer expander modules
//----------------------------------------------------------------------------

#define SEQUENCER_EXP_MAX_CHANNELS 4

#define SEQUENCER_EXP_MASTER_MODULE_DEFAULT 0
#define SEQUENCER_EXP_MASTER_MODULE_BNRYSEQ 1
#define SEQUENCER_EXP_MASTER_MODULE_STEPSEQ 2

struct SequencerExpanderMessage {
	int channelCV;		// for CV expanders
	int channelOUT;		// for output expanders
	int channelTRIG;	// for gate/trigger expanders
	
	int masterModule;
	
	// for sequencers with multiple channels
	int counters[SEQUENCER_EXP_MAX_CHANNELS] = {};
	bool clockStates[SEQUENCER_EXP_MAX_CHANNELS] = {};
	bool runningStates[SEQUENCER_EXP_MAX_CHANNELS] = {};
	
	// sets the CV expander channel number
	void setCVChannel (int c) {
		channelCV = c;
	}

	// sets the output expander channel
	void setOutChannel (int c) {
		channelOUT = c;
	}
	
	// sets the trigger expander channel
	void setTrigChannel (int c) {
		channelTRIG = c;
	}
	
	// sets the channel for the next CV expander 
	void setNextCVChannel(int c) {
		channelCV = c;
		
		if (channelCV >= 0)
			channelCV++;
			
		// wrap the channels around in case anyone is crazy enough to add more expanders than we expect
		if (channelCV >= SEQUENCER_EXP_MAX_CHANNELS)
			channelCV  = 0;
	}
	
	// sets the channel for the next output expander
	void setNextOutChannel(int c) {
		channelOUT = c;
		
		if (channelOUT >= 0)
			channelOUT++;
		
		// wrap the channels around in case anyone is crazy enough to add more expanders than we expect
		if (channelOUT >= SEQUENCER_EXP_MAX_CHANNELS)
			channelOUT  = 0;		
	}	
	
	// sets the channel fro the next trigger expander 
	void setNextTrigChannel(int c) {
		channelTRIG = c;
		
		if (channelTRIG >= 0)
			channelTRIG++;
		
		// wrap the channels around in case anyone is crazy enough to add more expanders than we expect
		if (channelTRIG >= SEQUENCER_EXP_MAX_CHANNELS)
			channelTRIG  = 0;		
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