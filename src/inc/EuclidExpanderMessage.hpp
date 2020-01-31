//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Euclid expander message
//	For passing sequence details to and from euclidean sequencer expander modules
//  Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------

#define EUCLID_EXP_NUM_STEPS 8

// utility macros 
#define isExpanderModule(x) x->model == modelEuclidExpanderCV
#define isExpandableModule(x) x->model == modelEuclid

struct EuclidExpanderMessage {
	bool beatGate;
	bool restGate;
	
	bool clockEdge;
	bool clock;
	
	bool trig;
	bool running;

	int beatCount;
	int restCount;
	int stepCount;
	
	int channel;
	bool hasMaster;

	EuclidExpanderMessage() {
		initialise();
	}
	
	void set(bool bGate, bool rGate, bool clkEdge, bool clk, bool tr, bool run, int bCount, int rCount, int sCount, int chan, bool master) {
		beatGate = bGate;
		restGate = rGate;
		clockEdge = clkEdge;
		clock = clk;
		trig = tr;
		running = run;
		beatCount = bCount;
		restCount = rCount;
		stepCount = sCount;
		channel = chan;
		hasMaster = master;
	}
	
	void initialise() {
		beatGate = false;
		beatCount = false;
		restGate = false;
		restCount = -1;
		trig = -1;
		running = -1;									
		clockEdge = false;
		clock = false;
		stepCount = -1;;
		
		channel = 0;
		hasMaster = false;
	}
	
};

