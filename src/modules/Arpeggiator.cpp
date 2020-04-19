//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Hyper Maniacal LFO Module
//  Oscillator section based on the VCV VCO by Andrew Belt
//  Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/Utility.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Arpeggiator
#define PANEL_FILE "Arpeggiator.svg"

#define ARP_NUM_STEPS 8

// using custom rows to accommodate the row of status light at the top of the module
const int CUSTOM_ROWS5[5] = {
	74,
	137,
	200,
	263,
	326
};	

const float CUSTOM_ROWS6[6] = {
	74.0f,
	124.4f,
	174.8f,
	225.2f,
	275.6f,
	326.0f
};	

struct Arpeggiator : Module {
	enum ParamIds {
		LENGTH_PARAM,
		MODE_PARAM,
		SORT_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		GATE_INPUT,
		CV_INPUT,
		CLOCK_INPUT,
		RESET_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATE_OUTPUT,
		CV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(STEP_LIGHTS, ARP_NUM_STEPS * 2),
		ENUMS(CV_LIGHTS, PORT_MAX_CHANNELS * 2),
		NUM_LIGHTS
	};

	enum PatternIds {
		PATTERN_UP,
		PATTERN_DOWN,
		PATTERN_REPEAT,
		PATTERN_REST,
		NUM_PATTERNS
	};
	
	enum OctaveIds {
		OCTAVE_NONE,
		OCTAVE_DOWN,
		OCTAVE_UP,
		NUM_OCTAVES
	};

	enum NoteOrderIds {
		HILOW_ORDER,
		INPUT_ORDER,
		LOWHI_ORDER
	};
	
	enum DirectionIds {
		UP_MODE,
		DOWN_MODE,
		PENDULUM_MODE,
		RANDOM_MODE,
		PROGRAMME_MODE,
		NUM_MODES		
	};
	
	GateProcessor gpClock;
	GateProcessor gpGate[PORT_MAX_CHANNELS];
	GateProcessor gpReset;

	float cvList[PORT_MAX_CHANNELS] = {};
	int noteCount = 0;

	int patternLength = ARP_NUM_STEPS;
	int patternCount = 0;
	
	bool gateOut = false;
	bool prevGateout = false;
	bool skip = false;
	float cv = 0.0f;
	bool reset = false;
	
	int currentDirection = UP_MODE;
	
	int octave[ARP_NUM_STEPS] = {};
	int pattern[ARP_NUM_STEPS] = {};
	
	float octaves[NUM_OCTAVES] = {-1.0f, 0.0f, 1.0f};
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	Arpeggiator() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		for (int i = 0; i < ARP_NUM_STEPS; i++) {
			octave[i] = 1;
		}

		configParam(LENGTH_PARAM, 1.0f, 8.0f, 1.0f, "Pattern length");
		configParam(MODE_PARAM, 0.0f, 4.0f, 3.0f, "Arpeggiator mode");
		configParam(SORT_PARAM, 0.0f, 2.0f, 1.0f, "Sort order");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	void onReset() override {
		gpClock.reset();
		gpReset.reset();
		
		for (int i = 0; i < PORT_MAX_CHANNELS; i++)
			gpGate[i].reset();
		
		for (int i = 0; i < ARP_NUM_STEPS; i++) {
			pattern[i] = 0;
			octave[i] = 1;
		}
	}

	json_t *dataToJson() override {
		json_t *root = json_object();
		
		json_object_set_new(root, "moduleVersion", json_integer(1));
			
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		json_t *pat = json_array();
		json_t *oct = json_array();
	
		for (int i = 0; i < ARP_NUM_STEPS; i++) {
			json_array_insert_new(pat, i, json_integer(pattern[i]));
			json_array_insert_new(oct, i, json_integer(octave[i]));
		}
		
		json_object_set_new(root, "pattern", pat);
		json_object_set_new(root, "octave", oct);		

		return root;
	}

	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
			
		json_t *pat = json_object_get(root, "pattern");
		json_t *oct = json_object_get(root, "octave");

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
		}
	}
	
	void process(const ProcessArgs &args) override {

		// determine the pattern length
		patternLength = clamp((int)(params[LENGTH_PARAM].getValue()), 1, ARP_NUM_STEPS);

		// determine the mode
		int mode = (int)(params[MODE_PARAM].getValue());

		// process the gate and cv inputs
		bool gate = false;
		
		int numGates = inputs[GATE_INPUT].getChannels();
		int numCVs = 0;
		if (numGates > 0) {
			for (int c = 0; c < numGates; c++) {
				if (c < numGates) {
					if (gpGate[c].set(inputs[GATE_INPUT].getVoltage(c))) {
						gate = true;
						
						// add CV to the list for later
						cvList[numCVs++] = inputs[CV_INPUT].getVoltage(c);
					}
				}
			}
		}

		// preprocess the note order if required
		int sort = (int)(params[SORT_PARAM].getValue());
		switch (sort) {
			case LOWHI_ORDER:
				// sort into ascending order
				std::sort(cvList, cvList + PORT_MAX_CHANNELS);
				break;
			case HILOW_ORDER:
				// sort into descending order 
				std::sort(cvList, cvList + PORT_MAX_CHANNELS);
				std::reverse(cvList, cvList + PORT_MAX_CHANNELS);
				break;
			case INPUT_ORDER:
			default:
				// leave as input order
				break;
		}

		// process the clock input
		bool clockOut = gpClock.set(inputs[CLOCK_INPUT].getVoltage());

		// process the reset input
		gpReset.set(inputs[RESET_INPUT].getVoltage());
		if (gpReset.leadingEdge())
			reset = true;
		
		// quantize the gate outout to the clock leading edge
		if (gpClock.leadingEdge()) {
			gateOut = gate;
		}			

		// always start over with on a new gate signal
		if (gateOut && !prevGateout) {
			patternCount = patternLength;

			// always start pendulum mode on the up phase
			if (mode == PENDULUM_MODE)
				currentDirection = DOWN_MODE;

			if(mode == PROGRAMME_MODE)
				noteCount = numCVs - 1;
			else
				noteCount = currentDirection == UP_MODE ? numCVs - 1 : 0;
		}

		// process the selected arpeggiation mode
		if (mode == PROGRAMME_MODE) {
			// advance the pattern counter on every clock edge
			if (gateOut && gpClock.leadingEdge()) {
				patternCount++;
			}
			
			if (reset || patternCount >= patternLength)
				patternCount = 0;

			// advance the note counter based on the selected pattern on every clock edge
			if (gateOut && gpClock.leadingEdge()) {
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
			skip = false;
			
			// advance the clock in the appropriate direction
			if (gateOut && gpClock.leadingEdge()) {
				
				if(mode == PENDULUM_MODE)
					currentDirection = reset ? UP_MODE : currentDirection;
				else
					currentDirection = mode;
				
				switch (currentDirection) {
					case UP_MODE:
						if (reset)
								noteCount = 0;
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
				
				reset = false;
			}
		}
		
		// process the assembled CVs
		if (gateOut && gpClock.leadingEdge()) {
			if (numCVs > 0) {
				cv = cvList[noteCount] + (skip ? 0.0f : octaves[octave[patternCount]]);
			}
		}
		
		// set the lights
		for(int i = 0; i < PORT_MAX_CHANNELS ; i++) {
		
			if ( i < ARP_NUM_STEPS) {
				if (mode == PROGRAMME_MODE) {
					lights[STEP_LIGHTS + (i * 2)].setBrightness(boolToLight(gateOut && i == patternCount));
					lights[STEP_LIGHTS + (i * 2) + 1].setBrightness(boolToLight(i < patternLength));
				}
				else {
					lights[STEP_LIGHTS + (i * 2)].setBrightness(0.0f);
					lights[STEP_LIGHTS + (i * 2) + 1].setBrightness(0.0f);
				}
			}
			
			if (gateOut && clockOut && i == noteCount) {
				lights[CV_LIGHTS + (i * 2)].setBrightness(1.0f); // white is current position
				lights[CV_LIGHTS + (i * 2) + 1].setBrightness(0.0f); // blue is note data present 
			}
			else {
				lights[CV_LIGHTS + (i * 2)].setBrightness(0.0f); // white is current position
				lights[CV_LIGHTS + (i * 2) + 1].setBrightness(boolToLight(i < numCVs)); // blue is note data present 
			}
		}
		
		// output the gate and cv values
		outputs[GATE_OUTPUT].setVoltage(boolToGate(!skip && gateOut && clockOut));
		outputs[CV_OUTPUT].setVoltage(cv);
		
		// save for net time through - this will be used to determine the gate leading edge
		prevGateout = gateOut;
	}
};

struct PatternButton : OpaqueWidget {
	Arpeggiator* module;
	NVGcolor activeColor;
	NVGcolor inactiveColor;
	int value;
	int row = 0;
	
	void draw(const DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.0, 0.0, box.size.x, box.size.y, 3.0);
		if (module)
			nvgFillColor(args.vg, row < module->patternLength && module->pattern[row] == value ? activeColor : inactiveColor);
		else
			nvgFillColor(args.vg, inactiveColor);
		
		nvgFill(args.vg);

		nvgStrokeWidth(args.vg, 1.2);
		nvgStrokeColor(args.vg, module ? module->bezelColor : SCHEME_BLACK);
		nvgStroke(args.vg);
	}

	void onDragStart(const event::DragStart& e) override {
		if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
			if (row < module->patternLength)
				module->pattern[row] = value;
		}
		OpaqueWidget::onDragStart(e);
	}
};

struct OctaveButton : OpaqueWidget {
	Arpeggiator* module;
	NVGcolor activeColor;
	NVGcolor inactiveColor;
	int value;
	int row = 0;
	
	void draw(const DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.0, 0.0, box.size.x, box.size.y, 3.0);
		if (module)
			nvgFillColor(args.vg, row < module->patternLength && module->octave[row] == value ? activeColor : inactiveColor);
		else
			nvgFillColor(args.vg, inactiveColor);
		
		nvgFill(args.vg);
		
		nvgStrokeWidth(args.vg, 1.2);
		nvgStrokeColor(args.vg, module ? module->bezelColor : SCHEME_BLACK);
		nvgStroke(args.vg);
	}

	void onDragStart(const event::DragStart& e) override {
		if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
			if (row < module->patternLength)
				module->octave[row] = value;
		}
		OpaqueWidget::onDragStart(e);
	}
};

struct ArpeggiatorWidget : ModuleWidget {

	const NVGcolor activePatternColors[4] = {SCHEME_RED, SCHEME_YELLOW, SCHEME_GREEN, SCHEME_BLUE};
	const NVGcolor activeOctaveColors[4] = {SCHEME_RED, SCHEME_GREEN,SCHEME_YELLOW};
	const NVGcolor inactiveColor = nvgRGB(0x5a, 0x5a, 0x5a);

	ArpeggiatorWidget(Arpeggiator *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Arpeggiator.svg")));

		// screws
		#include "../components/stdScrews.hpp"	

		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], CUSTOM_ROWS6[STD_ROW1]), module, Arpeggiator::CV_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], CUSTOM_ROWS6[STD_ROW2]), module, Arpeggiator::GATE_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], CUSTOM_ROWS6[STD_ROW3]), module, Arpeggiator::CLOCK_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], CUSTOM_ROWS6[STD_ROW4]), module, Arpeggiator::RESET_INPUT));

		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], CUSTOM_ROWS6[STD_ROW5]), module, Arpeggiator::CV_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], CUSTOM_ROWS6[STD_ROW6]), module, Arpeggiator::GATE_OUTPUT));

		// pattern/octave buttons and lights
		float offset = 0.0f;
		for (int i = 0; i < ARP_NUM_STEPS; i++) {
			addChild(createLightCentered<SmallLight<CountModulaLightRG>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] - 10, CUSTOM_ROWS5[STD_ROW1] + offset), module, Arpeggiator::STEP_LIGHTS + (i* 2)));

			// pattern buttons
			int k = 10;
			for (int j = 0; j < 4; j++) {
				PatternButton* patternButton = new PatternButton();
				patternButton->box.pos = Vec(STD_COLUMN_POSITIONS[STD_COL3] + k - 7, CUSTOM_ROWS5[STD_ROW1] - 7 + offset);
				patternButton->box.size = Vec(14, 14);
				patternButton->module = module;
				patternButton->row = i;
				patternButton->value = j;
				patternButton->activeColor = activePatternColors[j];
				patternButton->inactiveColor = inactiveColor;
				
				addChild(patternButton);		
				k += 20;
			}
			
			// octave buttons
			k = -2;
			for (int j = 0; j < 3; j ++) {
				OctaveButton* octaveButton = new OctaveButton();
				octaveButton->box.pos = Vec(STD_COLUMN_POSITIONS[STD_COL6] + k, CUSTOM_ROWS5[STD_ROW1] - 7 + offset);
				octaveButton->box.size = Vec(14, 14);
				octaveButton->module = module;
				octaveButton->row = i;
				octaveButton->value = j;
				octaveButton->activeColor = activeOctaveColors[j];
				octaveButton->inactiveColor = inactiveColor;
				
				addChild(octaveButton);		
				k += 20;
			}
			
			offset += 27.0f;
		}
		
		// length, mode and note order switches
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], CUSTOM_ROWS5[STD_ROW5]), module, Arpeggiator::SORT_PARAM));
		addParam(createParamCentered<CountModulaRotarySwitch5PosWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL5], CUSTOM_ROWS5[STD_ROW5]), module, Arpeggiator::MODE_PARAM));
		addParam(createParamCentered<CountModulaRotarySwitchRed>(Vec(STD_COLUMN_POSITIONS[STD_COL7], CUSTOM_ROWS5[STD_ROW5]), module, Arpeggiator::LENGTH_PARAM));
		
		// cv status lights
		offset = 0.0f;
		for (int i = 0; i < PORT_MAX_CHANNELS; i++) {
			addChild(createLightCentered<SmallLight<CountModulaLightWB>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + offset, STD_ROWS8[STD_ROW1] - 4), module, Arpeggiator::CV_LIGHTS + (i * 2)));
			
			offset += 12.0f;
		}
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		Arpeggiator *module = dynamic_cast<Arpeggiator*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
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
