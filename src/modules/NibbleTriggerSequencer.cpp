//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Nibble Trigger Sequencer
//	Trigger sequencer based on binary nibble patterns
//	Copyright (C) 2022  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/FrequencyDivider.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/Utility.hpp"

#include "../inc/OctetTriggerSequencerExpanderMessage.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME NibbleTriggerSequencer
#define PANEL_FILE "NibbleTriggerSequencer.svg"

struct NibbleTriggerSequencer : Module {
	enum ParamIds {
		PATTERN_A1_PARAM,
		PATTERN_A5_PARAM,
		PATTERN_B1_PARAM,
		PATTERN_B5_PARAM,
		PATTERN_A1_CV_PARAM,
		PATTERN_A5_CV_PARAM,
		PATTERN_B1_CV_PARAM,
		PATTERN_B5_CV_PARAM,		
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		RESET_INPUT,
		RUN_INPUT,
		PATTERN_A1_CV_INPUT,
		PATTERN_A5_CV_INPUT,
		PATTERN_B1_CV_INPUT,
		PATTERN_B5_CV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		TRIG_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		TRIG_LIGHT,
		ENUMS(PATTERN_A_LIGHTS, 16),
		ENUMS(PATTERN_B_LIGHTS, 16),
		NUM_LIGHTS
	};

	// avalable pattern modes
	enum ChainedPatternModes {
		CHAINED_MODE_B_OFF,
		CHAINED_MODE_B_FOLLOW,
		CHAINED_MODE_B_INVERSE,
		NUM_PATTERN_MODES
	};
	
	// available output modes
	enum OutputModes {
		OUTPUT_MODE_CLOCK,
		OUTPUT_MODE_GATE,
		OUTPUT_MODE_TRIGGER,
		NUM_OUTPUT_MODES
	};
	
	GateProcessor gateClock;
	GateProcessor gateReset;
	GateProcessor gateRun;
	GateProcessor gateChain;
	dsp::PulseGenerator pulseGenClock;
	dsp::PulseGenerator pulseGenTrigA;
	dsp::PulseGenerator pulseGenTrigB;

	// count to bit mappping
	STEP_MAP;
	
	// integer to scale value mapping
	const float cvScaleMap[4] = { 25.5f, 12.8f, 6.4f, 3.2f};

	int startUpCounter = 0;	
	int count = 0;
	int selectedPatternA = DEFAULT_PATTERN_B;
	int selectedPatternB = DEFAULT_PATTERN_B;
	int actualPatternA = DEFAULT_PATTERN_B;
	int actualPatternB = DEFAULT_PATTERN_B;
	int nextPatternA = DEFAULT_PATTERN_B;
	int nextPatternB = DEFAULT_PATTERN_B;
	bool running = false;

	bool playingChannelB = false;

	bool gate = false;
	
	int outputMode= OUTPUT_MODE_CLOCK;
	int processCount = 8;
	
	OctetTriggerSequencerExpanderMessage rightMessages[2][1]; // messages to right module (expander)
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	NibbleTriggerSequencer() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(PATTERN_A1_PARAM, 0.0f, 15.0f, 8.0f, "Pattern A1-4 select");
		configParam(PATTERN_A5_PARAM, 0.0f, 15.0f, 8.0f, "Pattern A5-8 select");
		configParam(PATTERN_B1_PARAM, 0.0f, 15.0f, 8.0f, "Pattern B1-4 select");
		configParam(PATTERN_B5_PARAM, 0.0f, 15.0f, 8.0f, "Pattern B5-8 select");

		configParam(PATTERN_A1_CV_PARAM, -1.0f, 1.0f, 0.0f, "Pattern A1-4 CV Amount");
		configParam(PATTERN_A5_CV_PARAM, -1.0f, 1.0f, 0.0f, "Pattern A5-8 CV Amount");
		configParam(PATTERN_B1_CV_PARAM, -1.0f, 1.0f, 0.0f, "Pattern B1-4 CV Amount");
		configParam(PATTERN_B5_CV_PARAM, -1.0f, 1.0f, 0.0f, "Pattern B5-8 CV Amount");

		configInput(CLOCK_INPUT, "Clock");
		configInput(RESET_INPUT, "Reset");
		configInput(RUN_INPUT, "Run");
		configInput(PATTERN_A1_CV_INPUT, "Pattern A1-4 CV");
		configInput(PATTERN_A5_CV_INPUT, "Pattern A5-8 CV");
		configInput(PATTERN_B1_CV_INPUT, "Pattern B1-4 CV");
		configInput(PATTERN_B5_CV_INPUT, "Pattern B5-8 CV");

		configOutput(TRIG_OUTPUT, "Output");


		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
			
		// expander
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];
	}

	void onReset() override {
		gateClock.reset();
		gateReset.reset();
		gateRun.reset();
		pulseGenClock.reset();
		pulseGenTrigA.reset();
		pulseGenTrigB.reset();
		
		playingChannelB = false;
		count = 0;
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(1));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"

		json_object_set_new(root, "clockState", json_boolean(gateClock.high()));
		json_object_set_new(root, "runState", json_boolean(gateRun.high()));
		json_object_set_new(root, "playingChannelB", json_boolean(playingChannelB));
		json_object_set_new(root, "currentStep", json_integer(count));

		json_object_set_new(root, "outputMode", json_integer(outputMode));

		return root;
	}

	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"

		json_t *currentStep = json_object_get(root, "currentStep");
		if (currentStep)
			count = json_integer_value(currentStep);

		json_t *clk = json_object_get(root, "clockState");
		if (clk)
			gateClock.preset(json_boolean_value(clk));

		json_t *run = json_object_get(root, "runState");
		if (run) 
			gateRun.preset(json_boolean_value(run));

		json_t *pcb = json_object_get(root, "playingChannelB");
		if (pcb)
			playingChannelB = json_boolean_value(pcb);

		json_t *om = json_object_get(root, "outputMode");
		if (om)
			outputMode = clamp(json_integer_value(om), 0, 3);

		startUpCounter = 20;
		running = gateRun.high();
	}
	
	// set output to gate level with smoothed light display
	void setTriggerOutputs(bool state, int outputID, int lightID, float currentSampleTime) {
		if (state) {
			lights[lightID].setBrightness(1.0f);
			outputs[outputID].setVoltage(10.0f);
			gate = true;
		}
		else {
			lights[lightID].setSmoothBrightness(0.0f, currentSampleTime);
			outputs[outputID].setVoltage(0.0f);
			gate  = false;
		}
	}
	
	// sets output to gate level and light has no smoothing
	void setGateOutputs(bool state, int outputID, int lightID) {
		if (state) {
			lights[lightID].setBrightness(1.0f);
			outputs[outputID].setVoltage(10.0f);
			gate = true;
		}
		else {
			lights[lightID].setBrightness(0.0f);
			outputs[outputID].setVoltage(0.0f);
			gate = false;
		}
	}
	
	void process(const ProcessArgs &args) override {

		// process reset input
		gateReset.set(inputs[RESET_INPUT].getVoltage());

		// wait a number of cycles before we use the clock and run inputs to allow them propagate correctly after startup
		if (startUpCounter > 0) {
			startUpCounter--;
		}
		else {
			// process run input
			gateRun.set(inputs[RUN_INPUT].getNormalVoltage(10.0f));

			// process clock input
			gateClock.set(inputs[CLOCK_INPUT].getVoltage());
		}
		
		bool runHigh = gateRun.high();
		bool clockHigh = gateClock.high();
		bool clockEdge = gateClock.leadingEdge();
		
		// process the clock trigger - we'll use this to allow the run input edge to act like the clock if it arrives shortly after the clock edge
		if (clockEdge)
			pulseGenClock.trigger(1e-4f);
		else if (pulseGenClock.process(args.sampleTime)) {
			// if within cooey of the clock edge, run or reset is treated as a clock edge.
			clockEdge = (gateRun.leadingEdge() || gateReset.leadingEdge());
		}
		
		if (!runHigh)
			running = false;

		// handle the reset - takes precedence over everything else
		if (gateReset.leadingEdge()) {
			count = 0;
			playingChannelB = false;
		}
		else {
			// advance count on positive clock edge or the run edge if it is close to the clock edge
			if (clockEdge && runHigh) {
				// flag that we are now actually running
				running = true;
				
				if (++count > 8) {
					count = 1;

					playingChannelB ^= true;
				}
			}
		}

		// no need to check for pattern change, and update display at audio rate
		if (++processCount > 8) {
			processCount = 0;

			// determine the currently selected pattern
			float cvSelectA1 = params[PATTERN_A1_CV_PARAM].getValue() * inputs[PATTERN_A1_CV_INPUT].getNormalVoltage(0.0f) * 1.6f;
			float cvSelectA5 = params[PATTERN_A5_CV_PARAM].getValue() * inputs[PATTERN_A5_CV_INPUT].getNormalVoltage(0.0f) * 1.6f;

			float cvSelectB1 = params[PATTERN_B1_CV_PARAM].getValue() * inputs[PATTERN_B1_CV_INPUT].getNormalVoltage(0.0f) * 1.6f;
			float cvSelectB5 = params[PATTERN_B5_CV_PARAM].getValue() * inputs[PATTERN_B5_CV_INPUT].getNormalVoltage(0.0f) * 1.6f;
			
			int pA1 = clamp((int)(params[PATTERN_A1_PARAM].getValue() + cvSelectA1), 0, 15) * 16;
			int pA5 = clamp((int)(params[PATTERN_A5_PARAM].getValue() + cvSelectA5), 0, 15);
			int pB1 = clamp((int)(params[PATTERN_B1_PARAM].getValue() + cvSelectB1), 0, 15) * 16;
			int pB5 = clamp((int)(params[PATTERN_B5_PARAM].getValue() + cvSelectB5), 0, 15);
			
			nextPatternA = pA1 + pA5;
			nextPatternB = pB1 + pB5;

			int led = 0;
			int stepLed = 1;
			bool activeA, activeB;
			bool stepA, stepB;

			// display the pattern and clock values
			for (int b = 1; b < 9; b++) {
				activeA = ((nextPatternA & stepMap[b]) > 0);
				activeB = ((nextPatternB & stepMap[b]) > 0);

				lights[PATTERN_A_LIGHTS + led].setBrightness(boolToLight(activeA));
				lights[PATTERN_B_LIGHTS + led].setBrightness(boolToLight(activeB));

				stepA = (count == b);
				stepB = (count == b);

				// handle chaining
				stepA &= (!playingChannelB);
				stepB &= playingChannelB;
				
				lights[PATTERN_A_LIGHTS + stepLed].setBrightness(boolToLight(stepA));
				lights[PATTERN_B_LIGHTS + stepLed].setBrightness(boolToLight(stepB));

				led += 2;
				stepLed += 2;
			}
		}
		
		// sync the next pattern change to the clock
		if (clockEdge) {
			actualPatternA = nextPatternA;
			actualPatternB = nextPatternB;
			selectedPatternA = playingChannelB ? nextPatternB : nextPatternA;
		}

		// display and update of output A. Occurs every sample due to smoothed LEDs when in trigger mode so they can be seen
		switch (outputMode){
			case OUTPUT_MODE_CLOCK:
				setGateOutputs(running && clockHigh && (selectedPatternA & stepMap[count]) > 0, TRIG_OUTPUT, TRIG_LIGHT);
				break;
			case OUTPUT_MODE_GATE:
				setGateOutputs(running && (selectedPatternA & stepMap[count]) > 0, TRIG_OUTPUT, TRIG_LIGHT);
				break;
			case OUTPUT_MODE_TRIGGER:
				if (running && clockEdge && (selectedPatternA & stepMap[count]) > 0) {
					pulseGenTrigA.trigger(1e-3f);
					setTriggerOutputs(true, TRIG_OUTPUT, TRIG_LIGHT, args.sampleTime);
				}
				else
					setTriggerOutputs(pulseGenTrigA.process(args.sampleTime), TRIG_OUTPUT, TRIG_LIGHT, args.sampleTime);

				break;
		}
		
		// set up details for the expander
		if (rightExpander.module) {
			if (isExpanderModule(rightExpander.module)) {
				OctetTriggerSequencerExpanderMessage *messageToExpander = (OctetTriggerSequencerExpanderMessage*)(rightExpander.module->leftExpander.producerMessage);
				messageToExpander->set(count, clockEdge, actualPatternA, actualPatternB, 1, true, playingChannelB, true, CHAINED_MODE_B_OFF, processCount, gate, false);
				
				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}
	}
};

struct NibbleTriggerSequencerWidget : ModuleWidget {

	std::string panelName;
	
	// custom columns for odd sized panel
	const int COLUMN_POSITIONS[7] = {
		20,
		40,
		60,
		80,
		100,
		120,
		130
	};
	
	NibbleTriggerSequencerWidget(NibbleTriggerSequencer *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"

		// screws
		#include "../components/stdScrews.hpp"	

		// clock and reset inputs
		addInput(createInputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL2], STD_ROWS6[STD_ROW5]), module, NibbleTriggerSequencer::RUN_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW5]), module, NibbleTriggerSequencer::CLOCK_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL2], STD_ROWS6[STD_ROW6]), module, NibbleTriggerSequencer::RESET_INPUT));


		// pattern knobs
		addParam(createParamCentered<Potentiometer<SmallKnob<GreenKnob>>>(Vec(COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1]), module, NibbleTriggerSequencer::PATTERN_A1_CV_PARAM));
		addParam(createParamCentered<Potentiometer<SmallKnob<BlueKnob>>>(Vec(COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, NibbleTriggerSequencer::PATTERN_A5_CV_PARAM));
		addParam(createParamCentered<Potentiometer<SmallKnob<YellowKnob>>>(Vec(COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, NibbleTriggerSequencer::PATTERN_B1_CV_PARAM));
		addParam(createParamCentered<Potentiometer<SmallKnob<OrangeKnob>>>(Vec(COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, NibbleTriggerSequencer::PATTERN_B5_CV_PARAM));
		addParam(createParamCentered<RotarySwitch<GreenKnob>>(Vec(COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW1]), module, NibbleTriggerSequencer::PATTERN_A1_PARAM));
		addParam(createParamCentered<RotarySwitch<BlueKnob>>(Vec(COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW2]), module, NibbleTriggerSequencer::PATTERN_A5_PARAM));
		addParam(createParamCentered<RotarySwitch<YellowKnob>>(Vec(COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW3]), module, NibbleTriggerSequencer::PATTERN_B1_PARAM));
		addParam(createParamCentered<RotarySwitch<OrangeKnob>>(Vec(COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW4]), module, NibbleTriggerSequencer::PATTERN_B5_PARAM));

		// cv inputs
		addInput(createInputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, NibbleTriggerSequencer::PATTERN_A1_CV_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, NibbleTriggerSequencer::PATTERN_A5_CV_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, NibbleTriggerSequencer::PATTERN_B1_CV_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, NibbleTriggerSequencer::PATTERN_B5_CV_INPUT));

		// trig output and light
		addOutput(createOutputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW6]), module, NibbleTriggerSequencer::TRIG_OUTPUT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(COLUMN_POSITIONS[STD_COL5] + 15, STD_ROWS6[STD_ROW6] - 19.5), module, NibbleTriggerSequencer::TRIG_LIGHT));

		// pattern display
		int led = 0;
		float pos1 = float(STD_ROWS6[STD_ROW1]) - 24.5f;
		float pos2 = float(STD_ROWS6[STD_ROW3]) - 24.5f;
		
		for (int j = 0; j < 2; j++) {
			for (int row = 0; row < 4; row++) {
				addChild(createLightCentered<MediumLightSquare<CountModulaSquareLight<CountModulaLightRG>>>(Vec(COLUMN_POSITIONS[STD_COL7], pos1), module, NibbleTriggerSequencer::PATTERN_A_LIGHTS + led));
				addChild(createLightCentered<MediumLightSquare<CountModulaSquareLight<CountModulaLightRG>>>(Vec(COLUMN_POSITIONS[STD_COL7], pos2), module, NibbleTriggerSequencer::PATTERN_B_LIGHTS + led));

				pos1 += 12.8f;
				pos2 += 12.8f;
				led += 2;
			}
			
			// skip a bit for leds 5-8
			pos1 += 3.5f;
			pos2 += 3.5f;
			
		}
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	//---------------------------------------------------------------------------------------------
	// output mode menu
	//---------------------------------------------------------------------------------------------
	// output mode menu item
	struct OutputModeMenuItem : MenuItem {
		NibbleTriggerSequencer *module;
		int mode;

		void onAction(const event::Action &e) override {
			module->outputMode = mode;
		}
	};
	
	// output mode menu
	struct OutputModeMenu : MenuItem {
		NibbleTriggerSequencer *module;

		const char *menuLabels[NibbleTriggerSequencer::NUM_OUTPUT_MODES] = {"Clocks", "Gates", "Triggers" };

		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			for (int i = 0; i < NibbleTriggerSequencer::NUM_OUTPUT_MODES; i++) {
				OutputModeMenuItem *modeMenuItem = createMenuItem<OutputModeMenuItem>(menuLabels[i], CHECKMARK(module->outputMode == i));
				modeMenuItem->module = module;
				modeMenuItem->mode = i;
				menu->addChild(modeMenuItem);
			}

			return menu;
		}
	};
	
	// expander addition menu item
	#include "../inc/AddExpanderMenuItem.hpp"	

	//---------------------------------------------------------------------------------------------
	// context menu
	//---------------------------------------------------------------------------------------------
	void appendContextMenu(Menu *menu) override {
		NibbleTriggerSequencer *module = dynamic_cast<NibbleTriggerSequencer*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"	
		
		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel("Settings"));
		
		// add the output mode menu
		OutputModeMenu *modeMenu = createMenuItem<OutputModeMenu>("Output mode", RIGHT_ARROW);
		modeMenu->module = module;
		menu->addChild(modeMenu);
		
		// add expander menu
		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel("Expansion"));
		
		AddExpanderMenuItem *trigMenuItem = createMenuItem<AddExpanderMenuItem>("Add CV expander");
		trigMenuItem->module = module;
		trigMenuItem->model = modelOctetTriggerSequencerCVExpander;
		trigMenuItem->position = box.pos;
		trigMenuItem->expanderName = "CV";
		menu->addChild(trigMenuItem);	
		
		AddExpanderMenuItem *gateMenuItem = createMenuItem<AddExpanderMenuItem>("Add gate expander");
		gateMenuItem->module = module;
		gateMenuItem->model = modelOctetTriggerSequencerGateExpander;
		gateMenuItem->position = box.pos;
		gateMenuItem->expanderName = "gate";
		menu->addChild(gateMenuItem);		
	}
	
	//---------------------------------------------------------------------------------------------
	// widget step
	//---------------------------------------------------------------------------------------------	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}	
};	

Model *modelNibbleTriggerSequencer = createModel<NibbleTriggerSequencer, NibbleTriggerSequencerWidget>("NibbleTriggerSequencer");
