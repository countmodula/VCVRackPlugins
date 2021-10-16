//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - 8 Channel Gate Sequencer Module
//  Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
struct STRUCT_NAME : Module {

	enum ParamIds {
		ENUMS(STEP_PARAMS, GATESEQ_NUM_STEPS * GATESEQ_NUM_ROWS),
		LENGTH_PARAM,
		ENUMS(MUTE_PARAMS, GATESEQ_NUM_ROWS),
		DIRECTION_PARAM,
		ADDR_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		RUN_INPUT,
		CLOCK_INPUT,
		RESET_INPUT,
		LENGTH_INPUT,
		DIRECTION_INPUT,
		ADDRESS_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(GATE_OUTPUTS, GATESEQ_NUM_ROWS),
		ENUMS(TRIG_OUTPUTS, GATESEQ_NUM_ROWS),
		END_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(STEP_LIGHTS, GATESEQ_NUM_STEPS * GATESEQ_NUM_ROWS),
		ENUMS(GATE_LIGHTS, GATESEQ_NUM_ROWS),
		ENUMS(TRIG_LIGHTS, GATESEQ_NUM_ROWS),
		ENUMS(LENGTH_LIGHTS, GATESEQ_NUM_STEPS),
		ENUMS(DIRECTION_LIGHTS, 5),
		ONESHOT_LIGHT,
		END_LIGHT,
		ENUMS(STEP_PARAM_LIGHTS, GATESEQ_NUM_STEPS * GATESEQ_NUM_ROWS),
		ENUMS(MUTE_PARAM_LIGHTS, GATESEQ_NUM_ROWS),
		NUM_LIGHTS
	};
	
	enum Directions {
		FORWARD,
		PENDULUM,
		REVERSE,
		RANDOM,
		FORWARD_ONESHOT,
		PENDULUM_ONESHOT,
		REVERSE_ONESHOT,
		RANDOM_ONESHOT,
		ADDRESSED
	};

	GateProcessor gateClock;
	GateProcessor gateReset;
	GateProcessor gateRun;
	dsp::PulseGenerator pgClock;

	int startUpCounter = 0;	
	int count = 0;
	int length = GATESEQ_NUM_STEPS;
	int direction = FORWARD;
	int directionMode = FORWARD;
	bool oneShot = false;
	bool oneShotEnded = false;
	bool running = false;
	
	float lengthCVScale = (float)(GATESEQ_NUM_STEPS);
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	STRUCT_NAME() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// length, direction and address params
		configParam(LENGTH_PARAM, 1.0f, (float)(GATESEQ_NUM_STEPS), (float)(GATESEQ_NUM_STEPS), "Length");
		configSwitch(DIRECTION_PARAM, 0.0f, 8.0f, 0.0f, "Direction", {"Forward", "Pendulum", "Reverse", "Random", "Forward oneshot", "Pendulum oneshot", "Reverse oneshot", "Random oneshot", "Voltage addressed"});
		configParam(ADDR_PARAM, 0.0f, 10.0f, 0.0f, "Address");
		
		std::string trackName;
		for (int r = 0; r < GATESEQ_NUM_ROWS; r++) {
			trackName = "Track " + std::to_string(r + 1);
			
			// row lights and switches
			for (int s = 0; s < GATESEQ_NUM_STEPS; s++) {
				configSwitch(STEP_PARAMS + (r * GATESEQ_NUM_STEPS) + s, 0.0f, 1.0f, 0.0f, trackName + " step " + std::to_string(s + 1), {"Off", "On"});
			}
			
			// output lights, mute buttons and jacks
			configSwitch(MUTE_PARAMS + r, 0.0f, 1.0f, 0.0f, trackName + " mute", {"Off", "On"});
			configOutput(GATE_OUTPUTS + r, trackName + " gate");
			configOutput(TRIG_OUTPUTS + r, trackName + " trigger");
		}
			
		configInput(RUN_INPUT, "Run");
		configInput(CLOCK_INPUT, "Clock");
		configInput(RESET_INPUT, "Reset");
		configInput(LENGTH_INPUT, "Length CV");
		configInput(DIRECTION_INPUT, "Direction CV");
		configInput(ADDRESS_INPUT, "Address CV");
	
		configOutput(END_OUTPUT, "1-shot end");
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(1));
		json_object_set_new(root, "currentStep", json_integer(count));
		json_object_set_new(root, "direction", json_integer(direction));
		json_object_set_new(root, "clockState", json_boolean(gateClock.high()));
		json_object_set_new(root, "runState", json_boolean(gateRun.high()));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}
	
	void dataFromJson(json_t* root) override {
		
		json_t *currentStep = json_object_get(root, "currentStep");
		json_t *dir = json_object_get(root, "direction");
		json_t *clk = json_object_get(root, "clockState");
		json_t *run = json_object_get(root, "runState");

		if (currentStep)
			count = json_integer_value(currentStep);
		
		if (dir)
			direction = json_integer_value(dir);
		
		if (clk)
			gateClock.preset(json_boolean_value(clk));

		if (run) 
			gateRun.preset(json_boolean_value(run));
		
		running = gateRun.high();
		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"		
		
		startUpCounter = 20;
	}	
		
	
	void onReset() override {
		gateClock.reset();
		gateReset.reset();
		gateRun.reset();
		count = 0;
		length = GATESEQ_NUM_STEPS;
		direction = FORWARD;
		directionMode = FORWARD;
		pgClock.reset();
		oneShot = false;
		oneShotEnded = false;
	}

	void onRandomize() override {
		// don't randomize mute buttons
		for (int i = 0; i < GATESEQ_NUM_ROWS; i++) {
			params[MUTE_PARAMS + i].setValue(0.0f);
		}
	}	
	
	int recalcDirection() {
		
		switch (directionMode) {
			case PENDULUM_ONESHOT:
			case PENDULUM:
				return (direction == FORWARD ? REVERSE : FORWARD);
			case FORWARD:
			case FORWARD_ONESHOT:
			case REVERSE:
			case REVERSE_ONESHOT:			
			default:
				return directionMode;
		}
	}	
	
	void setDirectionLight() {

		// handle the one-shot modes here as we'll remap them later to make the processing logic simpler`
		oneShot = (directionMode >= FORWARD_ONESHOT && directionMode <= RANDOM_ONESHOT);
		if (!oneShot)
			oneShotEnded  = false;
		
		float red, yellow, green, blue, white;
		switch (directionMode) {
			case FORWARD:
			case FORWARD_ONESHOT:
				green = 1.0f;
				red = yellow = blue = white = 0.0f;
				directionMode = FORWARD;
				break;
			case PENDULUM:
			case PENDULUM_ONESHOT:
				yellow = 0.8f;
				red = green = blue = white = 0.0f;
				directionMode = PENDULUM;
				break;
			case REVERSE:
			case REVERSE_ONESHOT:
				red = 1.0f;
				yellow = green = blue = white = 0.0f;
				directionMode = REVERSE;
				break;
			case RANDOM:
			case RANDOM_ONESHOT:
				blue = 0.8f;
				red = yellow = green = white = 0.0f;
				directionMode = RANDOM;
				break;
			case ADDRESSED:
				white = 1.0f;
				red = yellow = green = blue = 0.0f;
				break;
			default:
				red = yellow = green = blue = white = 0.0f;
				break;
		}
		
		lights[DIRECTION_LIGHTS].setBrightness(green);
		lights[DIRECTION_LIGHTS + 1].setBrightness(yellow);
		lights[DIRECTION_LIGHTS + 2].setBrightness(red);
		lights[DIRECTION_LIGHTS + 3].setBrightness(blue);
		lights[DIRECTION_LIGHTS + 4].setBrightness(white);

		lights[ONESHOT_LIGHT].setBrightness(boolToLight(oneShot));
	}
	
	void process(const ProcessArgs &args) override {
		// grab all the common input values up front
		float f;
		
		// reset input
		f = inputs[RESET_INPUT].getVoltage();
		gateReset.set(f);
		
		// wait a number of cycles before we use the clock and run inputs to allow them propagate correctly after startup
		if (startUpCounter > 0) {
			startUpCounter--;
		}
		else {
			// run input
			f = inputs[RUN_INPUT].getNormalVoltage(10.0f);
			gateRun.set(f);

			// clock
			f =inputs[CLOCK_INPUT].getVoltage(); 
			gateClock.set(f);
		}
		
		// sequence length - jack overrides knob
		if (inputs[LENGTH_INPUT].isConnected()) {
			// scale the input such that 10V = step 16, 0V = step 1
			length = (int)(clamp(lengthCVScale/10.0f * inputs[LENGTH_INPUT].getVoltage(), 0.0f, lengthCVScale)) + 1;
		}
		else {
			length = (int)(params[LENGTH_PARAM].getValue());
		}
		
		// direction - jack overrides the switch
		if (inputs[DIRECTION_INPUT].isConnected()) {
			float dirCV = clamp(inputs[DIRECTION_INPUT].getVoltage(), 0.0f, 8.99f);
			directionMode = (int)floor(dirCV);
		}
		else
			directionMode = (int)(params[DIRECTION_PARAM].getValue());

		// set direction light and determine if we're in one-shot mode
		setDirectionLight();			

		// determine the direction for the next step
		int nextDir = recalcDirection();
		
		// switch non-pendulum mode right away
		if (directionMode != PENDULUM)
			direction = nextDir;
		
		// handle the direction change on reset
		if (gateReset.leadingEdge()) {
			
			// ensure we reset the one-shot end output
			oneShotEnded = false;
			
			// restart pendulum at forward stage
			if (directionMode == PENDULUM)
				direction  = nextDir = FORWARD;
			
			// reset the count according to the next direction
			switch (nextDir) {
				case FORWARD:
				case RANDOM:
					count = 0;
					break;
				case REVERSE:
					count = GATESEQ_NUM_STEPS + 1;
					break;
			}
			
			direction = nextDir;
		}
		
		// process the clock trigger - we'll use this to allow the run input edge to act like the clock if it arrives shortly after the clock edge
		bool clockEdge = gateClock.leadingEdge();
		if (clockEdge)
			pgClock.trigger(1e-4f);
		else if (pgClock.process(args.sampleTime)) {
			// if within cooey of the clock edge, run or reset is treated as a clock edge.
			clockEdge = (gateRun.leadingEdge() || gateReset.leadingEdge());
		}
		
		if (gateRun.low())
			running = false;
		
		// advance count on positive clock edge or the run edge if it is close to the clock edge
		if (clockEdge && gateRun.high()) {
			
			// flag that we are now actually running
			running = true;
			
			if (oneShot && oneShotEnded)
				count = 0;
			else {
				switch (direction) {
					case FORWARD:
						count++;
						
						if (count > length) {
							if (nextDir == FORWARD) {
								count = 1;
								// we stop here in one-shot mode
								if (oneShot) {
									oneShotEnded = true;
									count = 0;
								}
							}
							else {
								// in pendulum mode we change direction here
								count--;
								direction = nextDir;
							}
						}
						break;
						
					case REVERSE:
						count--;
						
						if (count < 1) {
							// we always stop here in one-shot mode
							if (oneShot) {
								oneShotEnded = true;
								count = 0;
							}
							else {
								if (nextDir == REVERSE)
									count = length;
								else {
									// in pendulum mode we change direction here
									count++;
									direction = nextDir;
								}
							}
						}
						break;
						
					case RANDOM:						
						if (oneShot && count >= length) {
							oneShotEnded =  true;
							count = 0;
						}
						
						if (!oneShotEnded)
							count = 1 + (int)(random::uniform() * length);
						
						// in random mode, set the direction right away.
						direction = nextDir;
						
						
						break;
					case ADDRESSED:
						float v = clamp(inputs[ADDRESS_INPUT].getNormalVoltage(10.0f), 0.0f, 10.0f) * params[ADDR_PARAM].getValue();
						count = 1 + (int)((length) * v /100.0f);
						break;
				}
				
				if (count > length)
					count = length;
			}
		}
	
		
		// process the step switches and set the length/active step lights etc
		bool gate[GATESEQ_NUM_STEPS] = {};
		for (int c = 0; c < GATESEQ_NUM_STEPS; c++) {

			// set step and length lights here
			bool stepActive = (c + 1 == count);
			lights[STEP_LIGHTS + c].setBrightness(boolToLight(stepActive));
			lights[LENGTH_LIGHTS + c].setBrightness(boolToLight(c < length));

			// process the gates for the current step
			if (stepActive) {
				for (int r = 0; r < GATESEQ_NUM_ROWS; r++)
					gate[r] = running && (params[STEP_PARAMS + (r * GATESEQ_NUM_STEPS) + c].getValue() > 0.5f) && (params[MUTE_PARAMS + r].getValue() < 0.5f);
			}
		}
		
		// now we can set the outputs and lights
		bool clock = gateClock.high();
		for (int r = 0; r < GATESEQ_NUM_ROWS; r++) {
			outputs[GATE_OUTPUTS + r].setVoltage(boolToGate(gate[r]));
			outputs[TRIG_OUTPUTS + r].setVoltage(boolToGate(gate[r] && clock));
			lights[GATE_LIGHTS + r].setBrightness(boolToLight(gate[r]));
			lights[TRIG_LIGHTS + r].setBrightness(boolToLight(gate[r] && clock));
		}
		
		outputs[END_OUTPUT].setVoltage(boolToGate(oneShotEnded));
		lights[END_LIGHT].setBrightness(boolToLight(oneShotEnded));
	}
};

struct WIDGET_NAME : ModuleWidget {

	std::string panelName;
	
	WIDGET_NAME(STRUCT_NAME *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	

		// run input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1]), module, STRUCT_NAME::RUN_INPUT));

		// reset input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2]), module, STRUCT_NAME::RESET_INPUT));
		
		// clock input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 15, STD_ROWS8[STD_ROW1]), module, STRUCT_NAME::CLOCK_INPUT));
				
		// CV input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 15, STD_ROWS8[STD_ROW2]), module, STRUCT_NAME::LENGTH_INPUT));

		// direction and address input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW5]), module, STRUCT_NAME::DIRECTION_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, STRUCT_NAME::ADDRESS_INPUT));
		
		// length & direction params
		addParam(createParamCentered<RotarySwitch<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 22.5, STD_HALF_ROWS8(STD_ROW3)), module, STRUCT_NAME::LENGTH_PARAM));
		addParam(createParamCentered<RotarySwitch<BlueKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 15, STD_ROWS8[STD_ROW5]), module, STRUCT_NAME::DIRECTION_PARAM));
		addParam(createParamCentered<Potentiometer<WhiteKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 15, STD_ROWS8[STD_ROW8]), module, STRUCT_NAME::ADDR_PARAM));

		// step lights
		for (int s = 0; s < GATESEQ_NUM_STEPS; s++) {
			addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + s] - 5, STD_ROWS8[STD_ROW1] - 15), module, STRUCT_NAME::STEP_LIGHTS + s));
			addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + s] + 5, STD_ROWS8[STD_ROW1] - 15), module, STRUCT_NAME::LENGTH_LIGHTS + s));
		}
		
		// direction and one-shot light
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_HALF_ROWS8(STD_ROW5) + 3), module, STRUCT_NAME::DIRECTION_LIGHTS));
		addChild(createLightCentered<SmallLight<YellowLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_HALF_ROWS8(STD_ROW5) + 17), module, STRUCT_NAME::DIRECTION_LIGHTS + 1));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_HALF_ROWS8(STD_ROW5) + 31), module, STRUCT_NAME::DIRECTION_LIGHTS + 2));
		addChild(createLightCentered<SmallLight<BlueLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_HALF_ROWS8(STD_ROW5) + 45), module, STRUCT_NAME::DIRECTION_LIGHTS + 3));
		addChild(createLightCentered<SmallLight<WhiteLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_HALF_ROWS8(STD_ROW5) + 59), module, STRUCT_NAME::DIRECTION_LIGHTS + 4));
		
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 22.5, STD_HALF_ROWS8(STD_ROW5) + 10), module, STRUCT_NAME::ONESHOT_LIGHT));
		
		// add everything row by row
		for (int r = 0; r < GATESEQ_NUM_ROWS; r++) {
			
			// row lights and switches
			for (int s = 0; s < GATESEQ_NUM_STEPS; s++) {
				addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + s], STD_ROWS8[STD_ROW1 + r]), module, STRUCT_NAME::STEP_PARAMS + (r * GATESEQ_NUM_STEPS) + s, STRUCT_NAME::STEP_PARAM_LIGHTS + (r * GATESEQ_NUM_STEPS) + s));
			}
			
			// output lights, mute buttons and jacks
			addParam(createParamCentered<CountModulaLEDPushButton<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + GATESEQ_NUM_STEPS] + 15, STD_ROWS8[STD_ROW1 + r]), module, STRUCT_NAME::MUTE_PARAMS + r, STRUCT_NAME::MUTE_PARAM_LIGHTS + r));
			
			addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + GATESEQ_NUM_STEPS + 1] + 15, STD_ROWS8[STD_ROW1 + r]-10), module, STRUCT_NAME::GATE_LIGHTS + r));
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + GATESEQ_NUM_STEPS + 1] + 15, STD_ROWS8[STD_ROW1 + r]+10), module, STRUCT_NAME::TRIG_LIGHTS + r));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + GATESEQ_NUM_STEPS + 2] + 15, STD_ROWS8[STD_ROW1 + r]), module, STRUCT_NAME::GATE_OUTPUTS + r));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + GATESEQ_NUM_STEPS + 4], STD_ROWS8[STD_ROW1 + r]), module, STRUCT_NAME::TRIG_OUTPUTS + r));
		}
		
		// one-shot end
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 15, STD_HALF_ROWS8(STD_ROW6)), module, STRUCT_NAME::END_OUTPUT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 4, STD_HALF_ROWS8(STD_ROW6) - 20), module, STRUCT_NAME::END_LIGHT));
	}
	
	struct InitMenuItem : MenuItem {
		WIDGET_NAME *widget;
		int channel = 0;
		
		void onAction(const event::Action &e) override {
			// text for history menu item
			char buffer[100];
			sprintf(buffer, "initialize channel %d", channel + 1);
			
			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = buffer;
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();
			
			// gates
			for (int c = 0; c < GATESEQ_NUM_STEPS; c++) {
				widget->getParam(STRUCT_NAME::STEP_PARAMS + (channel * GATESEQ_NUM_STEPS) + c)->getParamQuantity()->reset();
			}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};	
	
	struct RandMenuItem : MenuItem {
		WIDGET_NAME *widget;
		int channel = 0;
	
		void onAction(const event::Action &e) override {
			// text for history menu item
			char buffer[100];
			sprintf(buffer, "randomize channel %d", channel + 1);
			
			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = buffer;
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();
			
			///gates
			for (int c = 0; c < GATESEQ_NUM_STEPS; c++) {
				widget->getParam(STRUCT_NAME::STEP_PARAMS + (channel * GATESEQ_NUM_STEPS) + c)->getParamQuantity()->randomize();
			}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};
		
	struct PresetMenuItem : MenuItem {
		WIDGET_NAME *widget;
		int channel = 0;
		bool pattern[GATESEQ_NUM_STEPS] = {};
		
		void onAction(const event::Action &e) override {
			// text for history menu item
			std::ostringstream  buffer;
			buffer << "channel " << channel + 1 << " " << string::lowercase(text);
			
			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = buffer.str();
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();
			
			for (int c = 0; c < GATESEQ_NUM_STEPS; c++) {
				widget->getParam(STRUCT_NAME::STEP_PARAMS + (channel * GATESEQ_NUM_STEPS) + c)->getParamQuantity()->setValue(pattern[c] ? 1.0f : 0.0f);
			}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};		
		
	struct ChannelMenuItem : MenuItem {
		WIDGET_NAME *widget;
		int channel = 0;

		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			// initialize
			InitMenuItem *initTrigMenuItem = createMenuItem<InitMenuItem>("Initialize");
			initTrigMenuItem->channel = channel;
			initTrigMenuItem->widget = widget;
			menu->addChild(initTrigMenuItem);

			// randomize
			RandMenuItem *randTrigMenuItem = createMenuItem<RandMenuItem>("Randomize");
			randTrigMenuItem->channel = channel;
			randTrigMenuItem->widget = widget;
			menu->addChild(randTrigMenuItem);

			enum Presets {
				ON_THE_1,
				ON_THE_2,
				ON_THE_3,
				ON_THE_4,
				ODDS,
				EVENS,
				THE_LOT,
				NUM_PRESETS
			};
			
			bool presetPatterns[NUM_PRESETS][4] = {
				{ 1, 0, 0, 0 },
				{ 0, 1, 0, 0 },
				{ 0, 0, 1, 0 },
				{ 0, 0, 0, 1 },
				{ 1, 0, 1, 0 },
				{ 0, 1, 0, 1 },
				{ 1, 1, 1, 1 }
			};
			
			std::string presetNames [NUM_PRESETS] = {
				"On the One",
				"On the Two",
				"On the Three",
				"On the Four",
				"Odds",
				"Evens",
				"The lot"
			};					
			
			// presets
			for (int i = 0; i < NUM_PRESETS; i++) {
				PresetMenuItem *presetTrigMenuItem1 = createMenuItem<PresetMenuItem>(presetNames[i]);
				int k = 0;
				for (int j = 0; j < GATESEQ_NUM_STEPS; j++) {
					presetTrigMenuItem1->pattern[j] = presetPatterns[i][k++];
					if (k > 3)
						k = 0;
				}
				presetTrigMenuItem1->channel = channel;
				presetTrigMenuItem1->widget = widget;
				menu->addChild(presetTrigMenuItem1);
			}
			
			return menu;
		}
	};
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"
	
	void appendContextMenu(Menu *menu) override {
		STRUCT_NAME *module = dynamic_cast<STRUCT_NAME*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());

		// add the theme menu items
		#include "../themes/themeMenus.hpp"
		
		char textBuffer[100];
		for (int r = 0; r < GATESEQ_NUM_ROWS; r++) {
			
			sprintf(textBuffer, "Channel %d", r + 1);
			ChannelMenuItem *chMenuItem = createMenuItem<ChannelMenuItem>(textBuffer, RIGHT_ARROW);
			chMenuItem->channel = r;
			chMenuItem->widget = this;
			menu->addChild(chMenuItem);
		}
	}
	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}	
};

Model *MODEL_NAME = createModel<STRUCT_NAME, WIDGET_NAME>(MODULE_NAME);
