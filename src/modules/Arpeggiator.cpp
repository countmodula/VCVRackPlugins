//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Super Arpeggiator
//	Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/SlewLimiter.hpp"
#include "../inc/Utility.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Arpeggiator
#define PANEL_FILE "Arpeggiator.svg"

#define ARP_NUM_STEPS 8

// using custom rows to accommodate the row of status light at the top of the module
const int CUSTOM_ROWS5[5] = {
	85,
	148,
	211,
	274,
	337
};	

struct Arpeggiator : Module {
	enum ParamIds {
		LENGTH_PARAM,
		MODE_PARAM,
		SORT_PARAM,
		GLIDE_PARAM,
		OCTAVE_PARAM,
		NOTE_PARAM,
		HOLD_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		GATE_INPUT,
		CV_INPUT,
		CLOCK_INPUT,
		RESET_INPUT,
		HOLD_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATE_OUTPUT,
		CV_OUTPUT,
		ACCENT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(STEP_LIGHTS, ARP_NUM_STEPS * 2),
		ENUMS(CV_LIGHTS, PORT_MAX_CHANNELS * 2),
		POLY_LIGHT,
		OCTAVE_PARAM_LIGHT,
		NOTE_PARAM_LIGHT,
		HOLD_PARAM_LIGHT,
		ENUMS(PROGRAMME_LIGHTS, 5 * ARP_NUM_STEPS),
		ENUMS(OCTAVE_LIGHTS, 3 * ARP_NUM_STEPS),
		ENUMS(MODIFY_LIGHTS, 2 * ARP_NUM_STEPS),
		NUM_LIGHTS
	};

	enum PatternIds {
		PATTERN_UP,
		PATTERN_DOWN,
		PATTERN_REPEAT,
		PATTERN_REST,
		PATTERN_SKIP,
		NUM_PATTERNS
	};
	
	enum OctaveIds {
		OCTAVE_NONE,
		OCTAVE_DOWN,
		OCTAVE_UP,
		NUM_OCTAVES
	};

	enum NoteOrderIds {
		DESC_ORDER,
		INPUT_ORDER,
		ASC_ORDER
	};
	
	enum DirectionIds {
		UP_MODE,
		DOWN_MODE,
		PENDULUM_MODE,
		RANDOM_MODE,
		MIDDLEOUT_MODE,
		OUTSIDEIN_MODE,
		FFB_MODE,
		PROGRAMME_MODE,
		NUM_MODES
	};
	
	GateProcessor gpClock;
	GateProcessor gpGate[PORT_MAX_CHANNELS];
	GateProcessor gpAnyGate;
	
	GateProcessor gpReset;
	GateProcessor gpHold;
	dsp::PulseGenerator pgClock;

	float cvList[PORT_MAX_CHANNELS] = {};
	float cvListHeld[PORT_MAX_CHANNELS] = {};
	int noteCount = 0;

	int patternLength = ARP_NUM_STEPS;
	int patternCount = 0;
	
	bool gate = false;
	int numGates = 0;
	int numCVs = 0;	
	int maxChannels = -1;
	bool hold = false;
	bool waitingForGate = false;
	
	bool gateOut = false;
	bool prevGateout = false;
	bool skip = false;
	float cv = 0.0f;
	bool accentOut = false;
	bool reset = false;

	float glideTime = 0.0f;
	float octaveOut = 0.0f;
	int currentDirection = UP_MODE;
	bool polyOutputs = false;
	
	int sort = -1;
	int mode = UP_MODE;
	bool noteProcessingEnabled = true;
	bool octaveProcessingEnabled = true;
	
	int octave[ARP_NUM_STEPS] = {};
	int pattern[ARP_NUM_STEPS] = {};
	bool glide[ARP_NUM_STEPS] = {};
	bool accent[ARP_NUM_STEPS] = {};
	float octaves[NUM_OCTAVES] = {-1.0f, 0.0f, 1.0f};
	
	LagProcessor slew[PORT_MAX_CHANNELS];

	const int mapMidOut[16][16] = {	{ 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, 
									{ 0,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, 
									{ 1,  2,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, 
									{ 1,  2,  0,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, 
									{ 2,  3,  1,  4,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, 
									{ 2,  3,  1,  4,  0,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, 
									{ 3,  4,  2,  5,  1,  6,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1}, 
									{ 3,  4,  2,  5,  1,  6,  0,  7, -1, -1, -1, -1, -1, -1, -1, -1}, 
									{ 4,  5,  3,  6,  2,  7,  1,  8,  0, -1, -1, -1, -1, -1, -1, -1}, 
									{ 4,  5,  3,  6,  2,  7,  1,  8,  0,  9, -1, -1, -1, -1, -1, -1}, 
									{ 5,  6,  4,  7,  3,  8,  2,  9,  1, 10,  0, -1, -1, -1, -1, -1}, 
									{ 5,  6,  4,  7,  3,  8,  2,  9,  1, 10,  0, 11, -1, -1, -1, -1}, 
									{ 6,  7,  5,  8,  4,  9,  3, 10,  1, 11,  1, 12,  0, -1, -1, -1}, 
									{ 6,  7,  5,  8,  4,  9,  3, 10,  1, 11,  1, 12,  0, 13, -1, -1}, 
									{ 7,  8,  6,  9,  5, 10,  4, 11,  3, 12,  2, 13,  1, 14,  0, -1}, 
									{ 7,  8,  6,  9,  5, 10,  4, 11,  3, 12,  2, 13,  1, 14,  0, 15}};

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	std::string stepLabels[ARP_NUM_STEPS] = {"Step 1 ", "Step 2 ", "Step 3 ", "Step 4 ", "Step 5 ", "Step 6 ", "Step 7 ", "Step 8 "};
	std::string programmeLabels[5] = {"Next note", "Previous note", "Repeat note", "Rest", "Skip note"};
	std::string octaveLabels[3] = {"octave shift down", "note octave shift", "octave shift up"};
	std::string modifyLabels[2] = {"glide", "accent"};

	Arpeggiator() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		for (int i = 0; i < ARP_NUM_STEPS; i++) {
			octave[i] = 1;
		}

		configParam(LENGTH_PARAM, 1.0f, 8.0f, 1.0f, "Pattern length");
		configSwitch(MODE_PARAM, 0.0f, 7.0f, 0.0f, "Sequence", {"Forward", "Reveres", "Pendulum", "Random", "Middle-out", "Outside-in", "Skip-N-Back", "Programme"});
		configSwitch(SORT_PARAM, 0.0f, 2.0f, 1.0f, "Note presort order", {"Descending", "Unsorted", "Ascending"});
		configParam(GLIDE_PARAM, 0.0f, 1.0f, 0.0f, "Glide", "", 0.0f, 10.0f, 0.0f);
		
		configSwitch(OCTAVE_PARAM, 0.0f, 1.0f, 1.0f, "Octave processing", {"Off", "On"});
		configSwitch(NOTE_PARAM, 0.0f, 1.0f, 1.0f, "Modifier processing", {"Off", "On"});
		configSwitch(HOLD_PARAM, 0.0f, 1.0f, 0.0f, "Hold", {"Off", "On"});

		configInput(GATE_INPUT, "Gate");
		configInput(CV_INPUT, "CV");
		configInput(CLOCK_INPUT, "Clock");
		configInput(RESET_INPUT, "Reset");
		configInput(HOLD_INPUT, "Hold");

		configOutput(GATE_OUTPUT, "Gate");
		configOutput(CV_OUTPUT, "CV");
		configOutput(ACCENT_OUTPUT, "Accent");

		int pl = PROGRAMME_LIGHTS;
		int ol = OCTAVE_LIGHTS;
		int ml = MODIFY_LIGHTS;
		for (int s = 0; s < ARP_NUM_STEPS;s ++) {
			for (int i = 0; i < 5; i++)
				configLight(pl++ , stepLabels[s] +  programmeLabels[i]);
		
			for (int i = 0; i < 3; i++)
				configLight(ol++ , stepLabels[s] + octaveLabels[i]);

			for (int i = 0; i < 2; i++)
				configLight(ml++ ,  stepLabels[s] + modifyLabels[i]);
		}
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	void onReset() override {
		gpClock.reset();
		gpReset.reset();
		gpHold.reset();
		gpAnyGate.reset();
	
		for (int i = 0; i < PORT_MAX_CHANNELS; i++) {
			gpGate[i].reset();
			cvList[i] = 0.0f;
			cvListHeld[i] = 0.0f;
			slew[i].reset();
		}
		
		for (int i = 0; i < ARP_NUM_STEPS; i++) {
			pattern[i] = 0;
			octave[i] = 1;
			glide[i] = false;
			accent[i] = false;
		}
		
		hold = false;
		gate =  false;
		numCVs = 0;
		waitingForGate = false;
	}
	
	void onRandomize() override {
		// randomize the pattern, octave, glide and accent buttons
		for (int i = 0; i < ARP_NUM_STEPS; i++) {
			pattern[i] = (int)(random::uniform() * 3.999);
			octave[i] = (int)(random::uniform() * 2.999);
			glide[i] = random::uniform() > 0.5f;
			accent[i] = random::uniform() > 0.5f;
		}
	}	

	json_t *dataToJson() override {
		json_t *root = json_object();
		
		json_object_set_new(root, "moduleVersion", json_integer(2));
			
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		json_t *pat = json_array();
		json_t *oct = json_array();
		json_t *gld = json_array();
		json_t *acc = json_array();
		json_t *cvl = json_array();
	
		for (int i = 0; i < ARP_NUM_STEPS; i++) {
			json_array_insert_new(pat, i, json_integer(pattern[i]));
			json_array_insert_new(oct, i, json_integer(octave[i]));
			json_array_insert_new(gld, i, json_boolean(glide[i]));
			json_array_insert_new(acc, i, json_boolean(accent[i]));	
			json_array_insert_new(cvl, i, json_real(cvListHeld[i]));
		}

		json_object_set_new(root, "numCVs", json_integer(numCVs));
		json_object_set_new(root, "hold", json_boolean(hold));
		json_object_set_new(root, "gate", json_boolean(gate));
		json_object_set_new(root, "nc", json_integer(noteCount));
		json_object_set_new(root, "pc", json_integer(patternCount));
		json_object_set_new(root, "polyOutputs", json_boolean(polyOutputs));	
		json_object_set_new(root, "pattern", pat);
		json_object_set_new(root, "octave", oct);		
		json_object_set_new(root, "glide", gld);		
		json_object_set_new(root, "accent", acc);
		json_object_set_new(root, "cvList", cvl);
		
		return root;
	}

	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
			
		json_t *ncv = json_object_get(root, "numCVs");
		json_t *hld = json_object_get(root, "hold");
		json_t *gte = json_object_get(root, "gate");
		json_t *nc = json_object_get(root, "nc");
		json_t *pc = json_object_get(root, "pc");
		json_t *po = json_object_get(root, "polyOutputs");
		
		json_t *pat = json_object_get(root, "pattern");
		json_t *oct = json_object_get(root, "octave");
		json_t *gld = json_object_get(root, "glide");
		json_t *acc = json_object_get(root, "accent");
		json_t *cvl = json_object_get(root, "cvList");

		for (int i = 0; i < ARP_NUM_STEPS; i++) {
			if (pat) {
				json_t *v = json_array_get(pat, i);
				if (v)
					pattern[i] = json_integer_value(v);
			}
			
			if (oct) {
				json_t *v = json_array_get(oct, i);
				if (v)
					octave[i] = json_integer_value(v);
			}

			if (gld) {
				json_t *v = json_array_get(gld, i);
				if (v)
					glide[i] = json_boolean_value(v);
			}

			if (acc) {
				json_t *v = json_array_get(acc, i);
				if (v)
					accent[i] = json_boolean_value(v);
			}
			
			if (cvl) {
				json_t *v = json_array_get(cvl, i);
				if (v)
					cvList[i] = cvListHeld[i] = json_real_value(v);
			}
		}
		
		if (ncv)
			numCVs = json_integer_value(ncv);

		if (hld)
			hold = json_boolean_value(hld);

		if (gte)
			gate = json_boolean_value(gte);
		
		if (nc)
			noteCount = json_integer_value(nc);
		
		if (pc)
			patternCount = json_integer_value(pc);
		
		if (po)
			polyOutputs = json_boolean_value(po);
		
	}
	
	void process(const ProcessArgs &args) override {

		// determine the pattern length
		patternLength = clamp((int)(params[LENGTH_PARAM].getValue()), 1, ARP_NUM_STEPS);

		// determine the sort and sequence modes
		int prevSort = sort;
		mode = (int)(params[MODE_PARAM].getValue());
		sort = (int)(params[SORT_PARAM].getValue());

		// which functiions are turned on?
		noteProcessingEnabled = params[NOTE_PARAM].getValue() > 0.5f;
		octaveProcessingEnabled = params[OCTAVE_PARAM].getValue() > 0.5f;
		
		// process the clock input
		bool clockOut = gpClock.set(inputs[CLOCK_INPUT].getVoltage());
		bool clockEdge = gpClock.leadingEdge();
		
		// wrangle the clock edge if we need to
		gpAnyGate.set(inputs[GATE_INPUT].getVoltageSum());
		if (gpAnyGate.low()) {
			if (clockEdge) {
				// clock has arrived and there's no gate signal, delay the clock edge until a gate arrives or we reach the end of the delay period
				pgClock.trigger(1e-4f);
				clockEdge = false;
				waitingForGate = true;
			}
			else {
				// not a clock edge, so we must check for the end of the delay time
				bool x = waitingForGate;
				waitingForGate = pgClock.process(args.sampleTime);
				
				if (x && !waitingForGate)
					clockEdge = true;				
			}
		}
		else if (waitingForGate && gpAnyGate.leadingEdge()) {
			// if within cooey of the clock edge, gate edge is treated as a clock edge.
			clockEdge = true;
			waitingForGate = false;
			pgClock.reset();
		}
		
		// handle the hold input - this overrides the button
		if (inputs[HOLD_INPUT].isConnected()) {
			hold = gpHold.set(inputs[HOLD_INPUT].getVoltage());
			params[HOLD_PARAM].setValue(hold ? 1.0f : 0.0f);
		}
		else {
			hold = gpHold.set(params[HOLD_PARAM].getValue() > 0.5f ? 10.0f : 0.0f);
		}

		// are we in hold?
		if (hold || !clockEdge) {
			// yes - use the gate and cv data we already have
			if (gate && prevSort != sort) {
				for (int c = 0; c < numCVs; c++) {
					// add CV to the list for later
					cvList[c] = cvListHeld[c];
				}
			}
		}
		else {
			// no - process the gate and cv inputs
			gate = false;
			numGates = inputs[GATE_INPUT].getChannels();
			numCVs = 0;
			if (numGates > 0) {
				for (int c = 0; c < numGates; c++) {
					if (gpGate[c].set(inputs[GATE_INPUT].getVoltage(c))) {
						gate = true;
						
						// add CV to the list for later
						cvListHeld[numCVs] = cvList[numCVs] = inputs[CV_INPUT].getVoltage(c);
						numCVs++;
					}
				}
			}
		}

		// preprocess the note order if required
		switch (sort) {
			case ASC_ORDER:
				// sort into ascending order
				std::sort(cvList, cvList + numCVs);
				break;
			case DESC_ORDER:
				// sort into descending order 
				std::sort(cvList, cvList + numCVs);
				std::reverse(cvList, cvList + numCVs);
				break;
			case INPUT_ORDER:
			default:
				// leave as input order
				break;
		}

		// process the reset input
		gpReset.set(inputs[RESET_INPUT].getVoltage());
		if (gpReset.leadingEdge())
			reset = true;
		
		// quantize the gate output to the clock leading edge
		if (clockEdge)
			gateOut = gate;
		
		// always start over with on a new gate signal
		if (gateOut && !prevGateout) {
			patternCount = patternLength;

			// always start pendulum mode on the up phase
			if (mode == PENDULUM_MODE)
				currentDirection = DOWN_MODE;
			else if (mode == FFB_MODE)
				currentDirection = UP_MODE;
				
			if(mode == PROGRAMME_MODE)
				noteCount = numCVs - 1;
			else
				noteCount = currentDirection == UP_MODE ? numCVs - 1 : 0;
		}

		// advance the pattern counter on every clock edge
		if (gateOut && clockEdge)
			patternCount++;
		
		if (reset || patternCount >= patternLength)
			patternCount = 0;

		// process the selected arpeggiation mode
		if (mode == PROGRAMME_MODE) {

			// advance the note counter based on the selected pattern on every clock edge
			if (gateOut && clockEdge) {
				switch  (pattern[patternCount]) {
					case PATTERN_DOWN:
						//go back a note
						if (reset)
							noteCount = numCVs - 1;
						else
							noteCount--;
						
						skip = false;
						break;
					case PATTERN_REPEAT:
						if (reset)
							noteCount = 0;

						// stay on the same note
						skip = false;
						break;
					case PATTERN_SKIP:
						if (reset)
							noteCount = 0;
						
						// skip to the next note
						noteCount = noteCount + 2;
						
						// wrap around if we exceed the number of notes
						if (noteCount >= numCVs)
							noteCount -= numCVs;
						
						break;
						
					case PATTERN_REST:
						if (reset)
							noteCount = 0;

						// stay on the same note and don't output a gate
						skip = true;
						break;
					case PATTERN_UP:
					default:
						// move forward a note
						if (reset)
							noteCount = 0;
						else
							noteCount++;
						
						skip = false;
						break;
				}
				
				reset = false;
			}
			
			// wrap around to the beginning if we've exceeded he number of available notes
			if (noteCount < 0)
				noteCount = numCVs - 1;
			
			// reset the note counter back to the start where required (wrap around or new chord being played)
			if (noteCount >= numCVs)
				noteCount = 0;
		}
		else {
			// the following are not relevant to these modes
			skip = false;
			
			// advance the clock in the appropriate direction
			if (gateOut && clockEdge) {
				
				switch (mode) {
					case FFB_MODE:
					case PENDULUM_MODE:
						currentDirection = reset ? UP_MODE : currentDirection;
						break;
					case MIDDLEOUT_MODE:
						currentDirection = UP_MODE;
						break;
					case OUTSIDEIN_MODE:
							currentDirection = DOWN_MODE;
							break;
					default:
						currentDirection = mode;
						break;
				}
				
				switch (currentDirection) {
					case UP_MODE:
						if (reset)
							noteCount = 0;
						else if (mode == FFB_MODE) {
							noteCount = noteCount + 2;
							
							if (noteCount >= numCVs)
								noteCount = noteCount - numCVs;
							
							currentDirection = DOWN_MODE;
						}
						else if (++noteCount >= numCVs) {
							if (mode == PENDULUM_MODE) {
								noteCount = numCVs - 1;
								currentDirection = DOWN_MODE;
							}
							else
								noteCount = 0;
						}
						break;
					case DOWN_MODE:
						if (reset)
							noteCount = numCVs - 1;
						else if (mode == FFB_MODE) {
							if (noteCount >= numCVs)
								noteCount = numCVs;
							
							if (--noteCount < 0)
								noteCount = numCVs - 1;
							
							currentDirection = UP_MODE;
						}
						else if (--noteCount < 0) {
							if (mode == PENDULUM_MODE) {
								noteCount = 0;
								currentDirection = UP_MODE;
							}
							else
								noteCount = numCVs - 1;
						}						
						break;
					case RANDOM_MODE:
						noteCount = (int)(random::uniform() * numCVs);
						currentDirection = UP_MODE; // must do this so the switch back to pendulum works
						break;
				}
				
				if (noteCount >= numCVs)
					noteCount = numCVs - 1;
				
				if (noteCount < 0)
					noteCount = 0;					
				
				reset = false;
			}
		}

		// process the octave and glide functions
		octaveOut = octaveProcessingEnabled ? octaves[octave[patternCount]] : 0.0f;
		accentOut = noteProcessingEnabled && accent[patternCount] && gate; //using gate not gateOut so the accent stays high for the duration of the step
		bool doGlide = noteProcessingEnabled && glide[patternCount] && gate;
		glideTime = doGlide ? params[GLIDE_PARAM].getValue() * 0.5f : 0.0f;
		
		// finally remap to corrct note if we're in a mapped mode
		int noteToUse = noteCount;
		if (numCVs > 0 && (mode == MIDDLEOUT_MODE || mode == OUTSIDEIN_MODE))
			noteToUse =  mapMidOut[numCVs-1][noteToUse];
		
		// process the assembled CVs
		if (gateOut && clockEdge) {
			if (numCVs > 0)
				cv = cvList[noteToUse] + octaveOut;
		}
	
		// set the lights
		for(int i = 0; i < PORT_MAX_CHANNELS ; i++) {
			if ( i < ARP_NUM_STEPS) {
				lights[STEP_LIGHTS + (i * 2)].setBrightness(boolToLight(gateOut && i == patternCount));
				lights[STEP_LIGHTS + (i * 2) + 1].setBrightness(boolToLight(i < patternLength && !(gateOut && i == patternCount)));
			}
			
			if (gateOut && clockOut && i == noteToUse) {
				lights[CV_LIGHTS + (i * 2)].setBrightness(1.0f); // white is current position
				lights[CV_LIGHTS + (i * 2) + 1].setBrightness(0.0f); // blue is note data present 
			}
			else {
				lights[CV_LIGHTS + (i * 2)].setBrightness(0.0f); // white is current position
				lights[CV_LIGHTS + (i * 2) + 1].setBrightness(boolToLight(i < numCVs)); // blue is note data present 
			}
		}
		
		lights[POLY_LIGHT].setBrightness(boolToLight(polyOutputs));
		
		// apply glide
		float cvOut = cv;
		
		// output the gate and cv values
		if (polyOutputs) {
			maxChannels = std::max(maxChannels, numCVs);

			outputs[GATE_OUTPUT].setChannels(maxChannels);
			outputs[CV_OUTPUT].setChannels(maxChannels);
			outputs[ACCENT_OUTPUT].setChannels(maxChannels);
			
			for (int i = 0; i < maxChannels; i++) {
				if (i == noteToUse) {
					// do glide here
					if (doGlide)
						cvOut = slew[noteToUse].process(cv, 1.0f, glideTime, glideTime, args.sampleTime);
			
					outputs[CV_OUTPUT].setVoltage(cvOut, noteToUse);
				}
				else {
					// do glide here
					if (doGlide) {
						outputs[CV_OUTPUT].setVoltage(slew[i].process(outputs[CV_OUTPUT].getVoltage(), 1.0f, glideTime, glideTime, args.sampleTime));
					}
				}
				outputs[GATE_OUTPUT].setVoltage(boolToGate(!skip && gateOut && clockOut && i == noteToUse), i);
				outputs[ACCENT_OUTPUT].setVoltage(boolToGate(accentOut && i == noteToUse), i);
			}
		}
		else {
			outputs[GATE_OUTPUT].setChannels(1);
			outputs[CV_OUTPUT].setChannels(1);
			outputs[ACCENT_OUTPUT].setChannels(1);
			
			if (doGlide)
				cvOut = slew[0].process(cv, 1.0f, glideTime, glideTime, args.sampleTime);
			
			outputs[GATE_OUTPUT].setVoltage(boolToGate(!skip && gateOut && clockOut), 0);
			outputs[CV_OUTPUT].setVoltage(cvOut, 0);
			outputs[ACCENT_OUTPUT].setVoltage(boolToGate(accentOut), 0);
		}
		// save for next time through - this will be used to determine the gate leading edge
		prevGateout = gateOut;
	}
};

// custom tool tip for the arpeggiator "touch screen" buttons
struct ArpeggiatorTouchTooltip : ui::Tooltip {
	ModuleLightWidget* lightWidget;

	// hack to make the mouse-overs look like button tooltips.
	void step() override {
		if (lightWidget->module) {
			engine::LightInfo* lightInfo = lightWidget->getLightInfo();
			if (!lightInfo)
				return;
			// Label
			text = lightInfo->getName();
			
			// Description
			std::string description = lightInfo->getDescription();
			if (description != "") {
				text += description;
			}

		}
		Tooltip::step();
		// Position at bottom-right of parameter
		box.pos = lightWidget->getAbsoluteOffset(lightWidget->box.size).round();
		// Fit inside parent (copied from Tooltip.cpp)
		assert(parent);
		box = box.nudge(parent->box.zeroPos());
	}
};

// special "touch screen" style buttons
struct ArpeggiatorTouchButton : ModuleLightWidget {
	NVGcolor activeColor;
	NVGcolor inactiveColor;
	ui::Tooltip* tooltip = NULL;
	int value;
	int row = 0;
	bool isLit = false;
	bool isActive = true;
	
	
	~ArpeggiatorTouchButton() {
		destroyTooltip();
	}
	
	void drawBackground(const DrawArgs &args) override {
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.0, 0.0, box.size.x, box.size.y, 1.0);
		nvgFillColor(args.vg, inactiveColor);
		nvgFill(args.vg);

		// add border
		if (isLit || !isActive) {
			nvgStrokeWidth(args.vg, 1.0);
			nvgStrokeColor(args.vg, SCHEME_BLACK);
		}
		else {
			nvgStrokeWidth(args.vg, 0.5);
			nvgStrokeColor(args.vg, activeColor);
		}

		nvgStroke(args.vg);
	}
	
	void drawLight(const DrawArgs& args) override {
		nvgGlobalTint(args.vg, color::WHITE);
		
		color = isLit ? activeColor : inactiveColor;
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.0, 0.0, box.size.x, box.size.y, 1.0);
		nvgFillColor(args.vg, color);
		nvgFill(args.vg);

		if (isLit) {
			nvgStrokeWidth(args.vg, 1.0);
			nvgStrokeColor(args.vg, SCHEME_BLACK);
		}
		else if (isActive) {
			nvgStrokeWidth(args.vg, 0.5);
			nvgStrokeColor(args.vg, activeColor);
		}
		else {
			nvgStrokeWidth(args.vg, 1.0);
			nvgStrokeColor(args.vg, SCHEME_BLACK);
		}
		nvgStroke(args.vg);
	}
	
	// hack to customise the tool tips
	void createTooltip() {
		if (!settings::tooltips)
			return;
		if (this->tooltip)
			return;
		// If the LightInfo is null, don't show a tooltip
		if (!getLightInfo())
			return;
		ArpeggiatorTouchTooltip* tooltip = new ArpeggiatorTouchTooltip;
		tooltip->lightWidget = this;
		APP->scene->addChild(tooltip);
		this->tooltip = tooltip;
	}
	
	void destroyTooltip() {
		if (!this->tooltip)
			return;
		APP->scene->removeChild(this->tooltip);
		delete this->tooltip;
		this->tooltip = NULL;
	}	
	
	// hack to customise the tooltips
	void onEnter(const EnterEvent& e) override {
		this->createTooltip();
	}
	
	void onLeave(const LeaveEvent& e) override {
		this->destroyTooltip();
	}	
	
	void step() override{
		if (module) {
			LightInfo* l = getLightInfo();
			
			if (l) {
				// hack to customise the tooltips
				l->description = isActive ? isLit ? ": On" : ": Off" : ": Inactive";
			}
		}
		ModuleLightWidget::step();
	}
};

struct PatternButton : ArpeggiatorTouchButton {
	void draw(const DrawArgs& args) override {

		if (module) {
			Arpeggiator *m = (Arpeggiator*)module;
			
			isActive = m->mode == Arpeggiator::PROGRAMME_MODE && row < m->patternLength;
			isLit = isActive && m->pattern[row] == value;
		}
		else {
			isActive = true;
		}

		ModuleLightWidget::draw(args);
	}	
	
	void onButton(const event::Button& e) override {
		if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
			e.stopPropagating();
			
			if (e.action ==GLFW_PRESS) {
				Arpeggiator *m = (Arpeggiator*)module;
				if (row < m->patternLength && m->mode == Arpeggiator::PROGRAMME_MODE) {
					// click when lit toggles the button set to the default of 0
					if (m->pattern[row] == value)
						m->pattern[row] = 0;
					else 
						m->pattern[row] = value;
				}
			}

			e.consume(this);
		}
		
		ModuleLightWidget::onButton(e);
	}
};

struct OctaveButton : ArpeggiatorTouchButton {
	void draw(const DrawArgs& args) override {
		if (module) {
			Arpeggiator *m = (Arpeggiator*)module;
			isActive = m->octaveProcessingEnabled && row < m->patternLength;
			isLit = isActive && m->octave[row] == value;
		}
		else
			isActive = true;

		ModuleLightWidget::draw(args);
	}

	void onButton(const event::Button& e) override {
		if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
			e.stopPropagating();
			
			if (e.action ==GLFW_PRESS) {
				Arpeggiator *m = (Arpeggiator*)module;
				if (m->octaveProcessingEnabled && row < m->patternLength) {
					// click when lit toggles the button set back the default of 1
					if (m->octave[row] == value)
						m->octave[row] = 1;
					else
						m->octave[row] = value;
				}
			}
			
			e.consume(this);
		}
		
		ModuleLightWidget::onButton(e);
	}
};

struct GlideButton : ArpeggiatorTouchButton {
	void draw(const DrawArgs& args) override {

		if (module) {
			Arpeggiator *m = (Arpeggiator*)module;
			isActive = m->noteProcessingEnabled && row < m->patternLength;
			isLit = isActive && m->glide[row];
		}
		else
			isActive = true;

		ModuleLightWidget::draw(args);
	}

	void onButton(const event::Button& e) override {
		if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
			e.stopPropagating();
			
			if (e.action ==GLFW_PRESS) {
				Arpeggiator *m = (Arpeggiator*)module;
				if (m->noteProcessingEnabled && row < m->patternLength) {
					m->glide[row] = !m->glide[row];
				}
			}
			
			e.consume(this);
		}
		
		ModuleLightWidget::onButton(e);
	}
};

struct AccentButton : ArpeggiatorTouchButton {
	void draw(const DrawArgs& args) override {
		if (module) {
			Arpeggiator *m = (Arpeggiator*)module;
			isActive = m->noteProcessingEnabled && row < m->patternLength;
			isLit = isActive && m->accent[row];
		}
		else
			isActive = true;

		ModuleLightWidget::draw(args);
	}

	void onButton(const event::Button& e) override {
		if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
			e.stopPropagating();
			
			if (e.action ==GLFW_PRESS) {
				Arpeggiator *m = (Arpeggiator*)module;
				if (m->noteProcessingEnabled && row < m->patternLength) {
					m->accent[row] = !m->accent[row];
				}
			}
			
			e.consume(this);
		}
		
		ModuleLightWidget::onButton(e);
	}
};

struct ArpeggiatorTouchSeparator : ModuleLightWidget {
	Arpeggiator* module;
	NVGcolor activeColor;
	NVGcolor inactiveColor;

	int value;
	int row = 0;
	int col = 0;
	
	void drawLight(const DrawArgs& args) override {
		
		nvgGlobalTint(args.vg, color::WHITE);
		
		if (module) {
			switch (col) {
				case 1:
					color = (module->mode == Arpeggiator::PROGRAMME_MODE && row < module->patternLength) ? activeColor : inactiveColor;
					break;
				case 2:
					color = (module->octaveProcessingEnabled && row < module->patternLength) ? activeColor : inactiveColor;
					break;
				case 3:
					color = (module->noteProcessingEnabled && row < module->patternLength) ? activeColor : inactiveColor;
					break;
				default:
					color = inactiveColor;
					break;
			}
		}
		else
			color = activeColor; // so it shows in the browser

		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.0, 0.0, box.size.x, box.size.y, 0.5);
		nvgFillColor(args.vg, color);
		nvgFill(args.vg);
	}
	
	void drawHalo(const DrawArgs& args) override {
		// Don't draw halo if rendering in a framebuffer, e.g. screenshots or Module Browser
		if (args.fb)
			return;

		const float halo = settings::haloBrightness;
		if (halo == 0.f)
			return;

		// If light is off, rendering the halo gives no effect.
		if (this->color.r == 0.f && this->color.g == 0.f && this->color.b == 0.f)
			return;

		float br = 30.0; // Blur radius
		float cr = 5.0; // Corner radius
		
		nvgBeginPath(args.vg);
		nvgRect(args.vg, -br, -br, this->box.size.x + 2 * br, this->box.size.y + 2 * br);
		NVGcolor icol = color::mult(color, halo);
		NVGcolor ocol = nvgRGBA(0, 0, 0, 0);
		nvgFillPaint(args.vg, nvgBoxGradient(args.vg, 0, 0, this->box.size.x, this->box.size.y, cr, br, icol, ocol));
		nvgFill(args.vg);
	}
};

struct ArpeggiatorWidget : ModuleWidget {
	const NVGcolor BUTTON_GREEN = nvgRGB(0xae, 0xd1, 0x71);
	const NVGcolor activePatternColors[5] = {SCHEME_GREEN, SCHEME_RED, SCHEME_YELLOW, SCHEME_BLUE, SCHEME_PURPLE};
	const NVGcolor activeOctaveColors[3] = {SCHEME_RED, SCHEME_GREEN,SCHEME_YELLOW};
	const NVGcolor inactiveColor = nvgRGB(0x2a, 0x2a, 0x2a);

	std::string panelName;
	
	ArpeggiatorWidget(Arpeggiator *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"
		
		// screws
		#include "../components/stdScrews.hpp"	

		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1]), module, Arpeggiator::HOLD_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2]), module, Arpeggiator::CV_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW3]), module, Arpeggiator::GATE_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW4]), module, Arpeggiator::CLOCK_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW5]), module, Arpeggiator::RESET_INPUT));

		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW6]), module, Arpeggiator::CV_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW7]), module, Arpeggiator::GATE_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, Arpeggiator::ACCENT_OUTPUT));

		// pattern/octave buttons and lights
		float offset = 0.0f;
		int pLid = Arpeggiator::PROGRAMME_LIGHTS;
		int oLid = Arpeggiator::OCTAVE_LIGHTS;
		int mLid = Arpeggiator::MODIFY_LIGHTS;
		for (int i = 0; i < ARP_NUM_STEPS; i++) {
			addChild(createLightCentered<SmallLight<CountModulaLightRG>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] - 10, CUSTOM_ROWS5[STD_ROW1] + offset), module, Arpeggiator::STEP_LIGHTS + (i* 2)));

			// pattern buttons
			int k = 10;
			for (int j = 0; j < 5; j++) {
				PatternButton* patternButton = new PatternButton();
				patternButton->box.pos = Vec(STD_COLUMN_POSITIONS[STD_COL3] + k - 7, CUSTOM_ROWS5[STD_ROW1] - 7 + offset);
				patternButton->box.size = Vec(14, 14);
				patternButton->module = module;
				patternButton->row = i;
				patternButton->value = j;
				patternButton->activeColor = activePatternColors[j];
				patternButton->inactiveColor = inactiveColor;
				patternButton->firstLightId = pLid++;
				addChild(patternButton);
				k += 20;
			}
			
			// octave buttons
			k = 21;
			for (int j = 0; j < 3; j ++) {
				OctaveButton* octaveButton = new OctaveButton();
				octaveButton->box.pos = Vec(STD_COLUMN_POSITIONS[STD_COL6] + k, CUSTOM_ROWS5[STD_ROW1] - 7 + offset);
				octaveButton->box.size = Vec(14, 14);
				octaveButton->module = module;
				octaveButton->row = i;
				octaveButton->value = j;
				octaveButton->activeColor = activeOctaveColors[j];
				octaveButton->inactiveColor = inactiveColor;
				octaveButton->firstLightId = oLid++;
				addChild(octaveButton);
				k += 20;
			}
			
			// glide buttons
			GlideButton* glideButton = new GlideButton();
			glideButton->box.pos = Vec(STD_COLUMN_POSITIONS[STD_COL9] - 1, CUSTOM_ROWS5[STD_ROW1] - 7 + offset);
			glideButton->box.size = Vec(14, 14);
			glideButton->module = module;
			glideButton->row = i;
			glideButton->activeColor = SCHEME_GREEN;
			glideButton->inactiveColor = inactiveColor;
			glideButton->firstLightId = mLid++;
			addChild(glideButton);		
			
			// accent buttons
			AccentButton* accentButton = new AccentButton();
			accentButton->box.pos = Vec(STD_COLUMN_POSITIONS[STD_COL10] -3, CUSTOM_ROWS5[STD_ROW1] - 7 + offset);
			accentButton->box.size = Vec(14, 14);
			accentButton->module = module;
			accentButton->row = i;
			accentButton->activeColor = SCHEME_GREEN;
			accentButton->inactiveColor = inactiveColor;
			accentButton->firstLightId = mLid++;
			addChild(accentButton);
			
			// separators
			if (i > 0) {
				// pattern buttons
				ArpeggiatorTouchSeparator *patSep = new ArpeggiatorTouchSeparator();
				patSep->box.pos = Vec(STD_COLUMN_POSITIONS[STD_COL3] + 5, CUSTOM_ROWS5[STD_ROW1] - 14 + offset);
				patSep->box.size = Vec(90, 1);
				patSep->module = module;
				patSep->row = i;
				patSep->col = 1;
				patSep->activeColor = SCHEME_WHITE;
				patSep->inactiveColor = inactiveColor;
				addChild(patSep);
				
				// octave buttons
				ArpeggiatorTouchSeparator *octSep = new ArpeggiatorTouchSeparator();
				octSep->box.pos = Vec(STD_COLUMN_POSITIONS[STD_COL7] - 7, CUSTOM_ROWS5[STD_ROW1] - 14 + offset);
				octSep->box.size = Vec(50, 1);
				octSep->module = module;
				octSep->row = i;
				octSep->col = 2;
				octSep->activeColor = SCHEME_WHITE;
				octSep->inactiveColor = inactiveColor;
				addChild(octSep);
				
				// modify buttons
				ArpeggiatorTouchSeparator *modSep = new ArpeggiatorTouchSeparator();
				modSep->box.pos = Vec(STD_COLUMN_POSITIONS[STD_COL9], CUSTOM_ROWS5[STD_ROW1] - 14 + offset);
				modSep->box.size = Vec(40, 1);
				modSep->module = module;
				modSep->row = i;
				modSep->col = 3;
				modSep->activeColor = SCHEME_WHITE;
				modSep->inactiveColor = inactiveColor;
				addChild(modSep);
			}
			
			offset += 27.f;
		}
		
		// length, mode and note order switches
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], CUSTOM_ROWS5[STD_ROW5]), module, Arpeggiator::SORT_PARAM));
		addParam(createParamCentered<RotarySwitch<WhiteKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL6], CUSTOM_ROWS5[STD_ROW5]), module, Arpeggiator::MODE_PARAM));
		addParam(createParamCentered<RotarySwitch<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL8], CUSTOM_ROWS5[STD_ROW5]), module, Arpeggiator::LENGTH_PARAM));
		addParam(createParamCentered<Potentiometer<BlueKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL10], CUSTOM_ROWS5[STD_ROW5]), module, Arpeggiator::GLIDE_PARAM));
	
		// octave and note processing bypass buttons
		addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL4] + 8, STD_ROWS8[STD_ROW8] - 20), module, Arpeggiator::OCTAVE_PARAM, Arpeggiator::OCTAVE_PARAM_LIGHT));
		addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL4] + 8, STD_ROWS8[STD_ROW8] + 15), module, Arpeggiator::NOTE_PARAM, Arpeggiator::NOTE_PARAM_LIGHT));
		
		// hold button
		addParam(createParamCentered<CountModulaLEDPushButton<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 6, STD_ROWS8[STD_ROW1]), module, Arpeggiator::HOLD_PARAM, Arpeggiator::HOLD_PARAM_LIGHT));

		// cv status lights
		offset = 10.0f;
		for (int i = 0; i < PORT_MAX_CHANNELS; i++) {
			addChild(createLightCentered<SmallLight<CountModulaLightWB>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + offset, STD_ROWS8[STD_ROW1]), module, Arpeggiator::CV_LIGHTS + (i * 2)));
			
			offset += 13.6f;
		}
		
		//poly light
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW7]), module, Arpeggiator::POLY_LIGHT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"


	// poly/mono selection menu item
	struct PolyMenuItem : MenuItem {
		Arpeggiator *module;
		
		void onAction(const event::Action &e) override {
			module->polyOutputs = !module->polyOutputs;
		}
	};


	void appendContextMenu(Menu *menu) override {
		Arpeggiator *module = dynamic_cast<Arpeggiator*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
		
		PolyMenuItem *polyMenuItem = createMenuItem<PolyMenuItem>("Polyphonic Outputs", CHECKMARK(module->polyOutputs));
		polyMenuItem->module = module;
		menu->addChild(polyMenuItem);
	
	}	
	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}	
};

Model *modelArpeggiator = createModel<Arpeggiator, ArpeggiatorWidget>("Arpeggiator");
