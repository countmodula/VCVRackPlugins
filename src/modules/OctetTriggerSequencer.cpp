//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Octet Trigger Sequencer
//	Trigger sequencer based on binary octet patterns
//	Copyright (C) 2021  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/FrequencyDivider.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/Utility.hpp"

#include "../inc/OctetTriggerSequencerExpanderMessage.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME OctetTriggerSequencer
#define PANEL_FILE "OctetTriggerSequencer.svg"

struct OctetTriggerSequencer : Module {
	enum ParamIds {
		PATTERN_A_CV_PARAM,
		PATTERN_B_CV_PARAM,
		PATTERN_A_PARAM,
		PATTERN_B_PARAM,
		CHAIN_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		RESET_INPUT,
		RUN_INPUT,
		PATTERN_A_CV_INPUT,
		PATTERN_B_CV_INPUT,
		CHAIN_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		TRIG_A_OUTPUT,
		TRIG_B_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		TRIG_A_LIGHT,
		TRIG_B_LIGHT,
		ENUMS(PATTERN_A_LIGHTS, 16),
		ENUMS(PATTERN_B_LIGHTS, 16),
		CHAIN_PARAM_LIGHT,
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
	int selectedPatternA = DEFAULT_PATTERN_A;
	int selectedPatternB = DEFAULT_PATTERN_B;
	int actualPatternA = DEFAULT_PATTERN_A;
	int actualPatternB = DEFAULT_PATTERN_B;
	int nextPatternA = DEFAULT_PATTERN_A;
	int nextPatternB = DEFAULT_PATTERN_B;
	bool running = false;

	float pSelectA = 0.0f;
	float pSelectB = 0.0f;

	bool chained = false;
	bool prevChained = false;
	bool playingChannelB = false;
	int chainedPatternMode = 0;

	int cvScale[2] = {0, 0};

	bool gates[2] = {};
	
	int outputMode[2] = {OUTPUT_MODE_CLOCK, OUTPUT_MODE_CLOCK};
	int processCount = 8;
	
	OctetTriggerSequencerExpanderMessage rightMessages[2][1]; // messages to right module (expander)
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	OctetTriggerSequencer() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(PATTERN_A_CV_PARAM, -1.0f, 1.0f, 0.0f, "Pattern A CV amount", " %", 0.0f, 100.0f, 0.0f);
		configParam(PATTERN_B_CV_PARAM, -1.0f, 1.0f, 0.0f, "Pattern B CV amount", " %", 0.0f, 100.0f, 0.0f);
		configParam(PATTERN_A_PARAM, 0.0f, 255.0f, (float)DEFAULT_PATTERN_A, "Pattern A select");
		configParam(PATTERN_B_PARAM, 0.0f, 255.0f, (float)DEFAULT_PATTERN_B, "Pattern B select");
		configButton(CHAIN_PARAM, "Chain patterns");

		configInput(CLOCK_INPUT, "Clock");
		configInput(RESET_INPUT, "Reset");
		configInput(RUN_INPUT, "Run");
		configInput(PATTERN_A_CV_INPUT, "Pattern A CV");
		configInput(PATTERN_B_CV_INPUT, "Pattern B CV");
		configInput(CHAIN_INPUT, "Chain CV");

		configOutput(TRIG_A_OUTPUT, "Channel A");
		configOutput(TRIG_B_OUTPUT, "Channel B");

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
		json_object_set_new(root, "chainedState", json_boolean(chained));
		json_object_set_new(root, "currentStep", json_integer(count));
		json_object_set_new(root, "chainedPatternMode", json_integer(chainedPatternMode));
		json_object_set_new(root, "scaleA", json_integer(cvScale[CHANNEL_A]));
		json_object_set_new(root, "scaleB", json_integer(cvScale[CHANNEL_B]));
		json_object_set_new(root, "outputModeA", json_integer(outputMode[CHANNEL_A]));
		json_object_set_new(root, "outputModeB", json_integer(outputMode[CHANNEL_B]));

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

		json_t *chs = json_object_get(root, "chainedState");
		if (chs)
			chained = json_boolean_value(chs);
		
		json_t *com = json_object_get(root, "chainedPatternMode");
		if (com)
			chainedPatternMode = clamp(json_integer_value(com), 0, 2);

		json_t *scaleA = json_object_get(root, "scaleA");
		if (scaleA)
			cvScale[CHANNEL_A] = clamp(json_integer_value(scaleA), 0, 3);

		json_t *scaleB = json_object_get(root, "scaleB");
		if (scaleA)
			cvScale[CHANNEL_B] = clamp(json_integer_value(scaleB), 0, 3);

		json_t *outputModeA = json_object_get(root, "outputModeA");
		if (outputModeA)
			outputMode[CHANNEL_A] = clamp(json_integer_value(outputModeA), 0, 3);
			
		json_t *outputModeB = json_object_get(root, "outputModeB");
		if (outputModeB)
			outputMode[CHANNEL_B] = clamp(json_integer_value(outputModeB), 0, 3);

		startUpCounter = 20;
		running = gateRun.high();
	}
	
	// set output to gate level with smoothed light display
	void setTriggerOutputs(bool state, int outputID, int lightID, int channelID, float currentSampleTime) {
		if (state) {
			lights[lightID].setBrightness(1.0f);
			outputs[outputID].setVoltage(10.0f);
			gates[channelID] = true;
		}
		else {
			lights[lightID].setSmoothBrightness(0.0f, currentSampleTime);
			outputs[outputID].setVoltage(0.0f);
			gates[channelID] = false;
		}
	}
	
	// sets output to gate level and light has no smoothing
	void setGateOutputs(bool state, int outputID, int lightID, int channelID) {
		if (state) {
			lights[lightID].setBrightness(1.0f);
			outputs[outputID].setVoltage(10.0f);
			gates[channelID] = true;
		}
		else {
			lights[lightID].setBrightness(0.0f);
			outputs[outputID].setVoltage(0.0f);
			gates[channelID] = false;
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
		
		// process chain param/input
		gateChain.set(inputs[CHAIN_INPUT].getNormalVoltage(params[CHAIN_PARAM].getValue() * 10.0f));
		bool chainHigh = gateChain.high();
		params[CHAIN_PARAM].setValue(boolToLight(chainHigh));

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
				
				// what mode are we in?
				chained = chainHigh;

				// ensure we always start with channel A when chaining is first engaged
				if (!chained)
					playingChannelB = false; 
				
				if (++count > 8) {
					count = 1;
					
					if (chained)
						playingChannelB ^= true;
				}
			}
		}

		// no need to check for pattern change, and update display at audio rate
		if (++processCount > 8) {
			processCount = 0;

			// determine the currently selected pattern
			pSelectA = params[PATTERN_A_CV_PARAM].getValue() * inputs[PATTERN_A_CV_INPUT].getNormalVoltage(0.0f) * cvScaleMap[cvScale[CHANNEL_A]];
			pSelectB = params[PATTERN_B_CV_PARAM].getValue() * inputs[PATTERN_B_CV_INPUT].getNormalVoltage(0.0f) * cvScaleMap[cvScale[CHANNEL_B]];
			nextPatternA = clamp((int)(params[PATTERN_A_PARAM].getValue() + pSelectA), 0, 255);
			nextPatternB = clamp((int)(params[PATTERN_B_PARAM].getValue() + pSelectB), 0, 255);

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

				if (chained) {
					stepA &= (!playingChannelB);
					stepB &= playingChannelB;
				}
				
				lights[PATTERN_A_LIGHTS + stepLed].setBrightness(boolToLight(stepA));
				lights[PATTERN_B_LIGHTS + stepLed].setBrightness(boolToLight(stepB));

				led += 2;
				stepLed += 2;
			}
		}
		
		// sync the next pattern change to the clock
		if (clockEdge) {
			if (chained) {
				actualPatternA = nextPatternA;
				actualPatternB = nextPatternB;
				selectedPatternA = playingChannelB ? nextPatternB : nextPatternA;

				switch (chainedPatternMode) {
					case CHAINED_MODE_B_OFF:
						selectedPatternB = 0;
						break;
					case CHAINED_MODE_B_FOLLOW:
						selectedPatternB = selectedPatternA;
						break;
					case CHAINED_MODE_B_INVERSE:
						selectedPatternB = ~selectedPatternA;
						break;
				}
			}
			else {
				actualPatternA = selectedPatternA = nextPatternA;
				actualPatternB = selectedPatternB = nextPatternB;
			}
		}

		// display and update of output A. Occurs every sample due to smoothed LEDs when in trigger mode so they can be seen
		switch (outputMode[CHANNEL_A]){
			case OUTPUT_MODE_CLOCK:
				setGateOutputs(running && clockHigh && (selectedPatternA & stepMap[count]) > 0, TRIG_A_OUTPUT, TRIG_A_LIGHT, CHANNEL_A);
				break;
			case OUTPUT_MODE_GATE:
				setGateOutputs(running && (selectedPatternA & stepMap[count]) > 0, TRIG_A_OUTPUT, TRIG_A_LIGHT, CHANNEL_A);
				break;
			case OUTPUT_MODE_TRIGGER:
				if (running && clockEdge && (selectedPatternA & stepMap[count]) > 0) {
					pulseGenTrigA.trigger(1e-3f);
					setTriggerOutputs(true, TRIG_A_OUTPUT, TRIG_A_LIGHT, CHANNEL_A, args.sampleTime);
				}
				else
					setTriggerOutputs(pulseGenTrigA.process(args.sampleTime), TRIG_A_OUTPUT, TRIG_A_LIGHT, CHANNEL_A, args.sampleTime);

				break;
		}

		// display and update of output B. Occurs every sample due to smoothed LEDs when in trigger mode so they can be seen
		switch (outputMode[CHANNEL_B]){
			case OUTPUT_MODE_CLOCK:
				setGateOutputs(running && clockHigh && (selectedPatternB & stepMap[count]) > 0, TRIG_B_OUTPUT, TRIG_B_LIGHT, CHANNEL_B);
				break;
			case OUTPUT_MODE_GATE:
				setGateOutputs(running && (selectedPatternB & stepMap[count]) > 0, TRIG_B_OUTPUT, TRIG_B_LIGHT, CHANNEL_B);
				break;
			case OUTPUT_MODE_TRIGGER:
				if (running && clockEdge && (selectedPatternB & stepMap[count]) > 0) {
					pulseGenTrigB.trigger(1e-3f);
					setTriggerOutputs(true, TRIG_B_OUTPUT, TRIG_B_LIGHT, CHANNEL_B, args.sampleTime);
				}
				else
					setTriggerOutputs(pulseGenTrigB.process(args.sampleTime), TRIG_B_OUTPUT, TRIG_B_LIGHT, CHANNEL_B, args.sampleTime);

				break;
		}
		
		// set up details for the expander
		if (rightExpander.module) {
			if (isExpanderModule(rightExpander.module)) {
				OctetTriggerSequencerExpanderMessage *messageToExpander = (OctetTriggerSequencerExpanderMessage*)(rightExpander.module->leftExpander.producerMessage);
				messageToExpander->set(count, clockEdge, actualPatternA, actualPatternB, 1, true, playingChannelB, chained, chainedPatternMode, processCount, gates[CHANNEL_A], gates[CHANNEL_B]);
				
				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}		
	}
};

struct OctetTriggerSequencerWidget : ModuleWidget {

	std::string panelName;
	
	// custom columns for odd sized panel
	const int COLUMN_POSITIONS[3] = {
		30,
		75,
		120
	};
	
	OctetTriggerSequencerWidget(OctetTriggerSequencer *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	

		// clock and reset inputs
		addInput(createInputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, OctetTriggerSequencer::RUN_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL2], STD_ROWS6[STD_ROW5]), module, OctetTriggerSequencer::CLOCK_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW5]), module, OctetTriggerSequencer::RESET_INPUT));

		// pattern knobs
		addParam(createParamCentered<RotarySwitch<GreenKnob>>(Vec(COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, OctetTriggerSequencer::PATTERN_A_PARAM));
		addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, OctetTriggerSequencer::PATTERN_A_CV_PARAM));
		addParam(createParamCentered<RotarySwitch<YellowKnob>>(Vec(COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, OctetTriggerSequencer::PATTERN_B_PARAM));
		addParam(createParamCentered<Potentiometer<YellowKnob>>(Vec(COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, OctetTriggerSequencer::PATTERN_B_CV_PARAM));

		// chain button and input
		addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<GreenLight>>>(Vec(COLUMN_POSITIONS[STD_COL2], STD_ROWS6[STD_ROW3]), module, OctetTriggerSequencer::CHAIN_PARAM, OctetTriggerSequencer::CHAIN_PARAM_LIGHT));
		addInput(createInputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL2], STD_ROWS6[STD_ROW4]), module, OctetTriggerSequencer::CHAIN_INPUT));

		// cv inputs
		addInput(createInputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, OctetTriggerSequencer::PATTERN_A_CV_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, OctetTriggerSequencer::PATTERN_B_CV_INPUT));
		

		// trig outputs and lights
		addOutput(createOutputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, OctetTriggerSequencer::TRIG_A_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW6]), module, OctetTriggerSequencer::TRIG_B_OUTPUT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(COLUMN_POSITIONS[STD_COL1] + 19, STD_ROWS6[STD_ROW6] - 19.5), module, OctetTriggerSequencer::TRIG_A_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(COLUMN_POSITIONS[STD_COL3] + 19, STD_ROWS6[STD_ROW6] - 19.5), module, OctetTriggerSequencer::TRIG_B_LIGHT));

		// pattern display
		float pos = float(COLUMN_POSITIONS[STD_COL1]);
		int led = 0;
		for (int col = 0; col < 8; col++) {
			addChild(createLightCentered<MediumLightSquare<CountModulaSquareLight<CountModulaLightRG>>>(Vec(pos, STD_ROWS6[STD_ROW1] - 8), module, OctetTriggerSequencer::PATTERN_A_LIGHTS + led));
			addChild(createLightCentered<MediumLightSquare<CountModulaSquareLight<CountModulaLightRG>>>(Vec(pos, STD_ROWS6[STD_ROW1] + 8), module, OctetTriggerSequencer::PATTERN_B_LIGHTS + led));

			pos += 12.8f;
			led += 2;
		}
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	//---------------------------------------------------------------------------------------------
	// chained pattern menu
	//---------------------------------------------------------------------------------------------
	// chained pattern menu item sets how the channel b output functions when running in chained mode
	struct ChainedPatternModeMenuItem : MenuItem {
		OctetTriggerSequencer *module;
		int mode;
		
		void onAction(const event::Action &e) override {
			module->chainedPatternMode = mode;
		}
	};

	// chained pattern menu
	struct ChainedPatternModeMenu : MenuItem {
		OctetTriggerSequencer *module;

		const char *menuLabels[OctetTriggerSequencer::NUM_PATTERN_MODES] = {"No output", "Follow channel A", "Inverse of channel A" };

		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			for (int i = 0; i < OctetTriggerSequencer::NUM_PATTERN_MODES; i++) {
				ChainedPatternModeMenuItem *chainedMenuItem = createMenuItem<ChainedPatternModeMenuItem>(menuLabels[i], CHECKMARK(module->chainedPatternMode == i));
				chainedMenuItem->module = module;
				chainedMenuItem->mode = i;
				menu->addChild(chainedMenuItem);
			}

			return menu;
		}
	};

	//---------------------------------------------------------------------------------------------
	// CV input pad amount menu
	//---------------------------------------------------------------------------------------------
	// CV input pad menu item
	struct ScaleMenuItem : MenuItem {
		OctetTriggerSequencer *module;
		int channel;
		int scale;

		void onAction(const event::Action &e) override {
			if (channel > 1)
				module->cvScale[CHANNEL_A] = module->cvScale[CHANNEL_B]= scale;
			else
				module->cvScale[channel]= scale;
		}
	};

	// scale menu
	struct ScaleChannelMenu : MenuItem {
		OctetTriggerSequencer *module;
		int channel;
		
		const char *menuLabels[4] = {"0V - 10V = 0 - 255", "0V - 10V = 0 - 128", "0V - 10V = 0 - 64" , "0V - 10V = 0 - 32"};

		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			for (int i = 0; i < 4; i++) {
				ScaleMenuItem *scaleMenuItem = createMenuItem<ScaleMenuItem>(menuLabels[i], channel > CHANNEL_B ? "" : CHECKMARK(module->cvScale[channel]== i));
				scaleMenuItem->module = module;
				scaleMenuItem->channel = channel;
				scaleMenuItem->scale = i;
				menu->addChild(scaleMenuItem);
			}
			
			return menu;
		}
	};
	
		// CV scale menu
	struct ScaleMenu : MenuItem {
		OctetTriggerSequencer *module;

		const char *menuLabels[3] = {"Channel A", "Channel B", "Both"};

		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			for (int i = 0; i < 3; i++) {
				ScaleChannelMenu *scaleMenuItem = createMenuItem<ScaleChannelMenu>(menuLabels[i], RIGHT_ARROW);
				scaleMenuItem->module = module;
				scaleMenuItem->channel = i;
				menu->addChild(scaleMenuItem);
			}
			
			return menu;
		}
	};
	
	//---------------------------------------------------------------------------------------------
	// output mode menu
	//---------------------------------------------------------------------------------------------
	// output mode menu item
	struct OutputModeMenuItem : MenuItem {
		OctetTriggerSequencer *module;
		int channel;
		int mode;

		void onAction(const event::Action &e) override {
			if (channel > 1)
				module->outputMode[CHANNEL_A] = module->outputMode[CHANNEL_B]= mode;
			else
				module->outputMode[channel]= mode;
		}
	};
	
	// output mode menu
	struct OutputModeChannelMenu : MenuItem {
		OctetTriggerSequencer *module;
		int channel;
		
		const char *menuLabels[OctetTriggerSequencer::NUM_OUTPUT_MODES] = {"Clocks", "Gates", "Triggers" };

		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			for (int i = 0; i < OctetTriggerSequencer::NUM_OUTPUT_MODES; i++) {
				OutputModeMenuItem *modeMenuItem = createMenuItem<OutputModeMenuItem>(menuLabels[i], channel > CHANNEL_B ? "" : CHECKMARK(module->outputMode[channel] == i));
				modeMenuItem->module = module;
				modeMenuItem->mode = i;
				modeMenuItem->channel = channel;
				menu->addChild(modeMenuItem);
			}

			return menu;
		}
	};
	
	struct OutputModeMenu : MenuItem {
		OctetTriggerSequencer *module;
		
		const char *menuLabels[3] = {"Channel A", "Channel B", "Both"};

		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			for (int i = 0; i < 3; i++) {
				OutputModeChannelMenu *outputMenu = createMenuItem<OutputModeChannelMenu>(menuLabels[i], RIGHT_ARROW);
				outputMenu->module = module;
				outputMenu->channel = i;
				menu->addChild(outputMenu);
			}
			
			return menu;
		}
	};

	//---------------------------------------------------------------------------------------------
	// context menu
	//---------------------------------------------------------------------------------------------
	void appendContextMenu(Menu *menu) override {
		OctetTriggerSequencer *module = dynamic_cast<OctetTriggerSequencer*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
		
		// add the chained pattern mode menu
		ChainedPatternModeMenu *chainedMenu = createMenuItem<ChainedPatternModeMenu>("Channel B chained pattern mode", RIGHT_ARROW);
		chainedMenu->module = module;
		menu->addChild(chainedMenu);
		
		// add the scale menu
		ScaleMenu *scaleMenu = createMenuItem<ScaleMenu>("CV scale", RIGHT_ARROW);
		scaleMenu->module = module;
		menu->addChild(scaleMenu);
		
		// add the output mode menu
		OutputModeMenu *modeMenu = createMenuItem<OutputModeMenu>("Output mode", RIGHT_ARROW);
		modeMenu->module = module;
		menu->addChild(modeMenu);
	}
	
	//---------------------------------------------------------------------------------------------
	// wodget step
	//---------------------------------------------------------------------------------------------	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}	
};	

Model *modelOctetTriggerSequencer = createModel<OctetTriggerSequencer, OctetTriggerSequencerWidget>("OctetTriggerSequencer");
