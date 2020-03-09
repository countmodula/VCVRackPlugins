//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Standard Sequencer Engine
//  Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------

struct STRUCT_NAME : Module {

	enum ParamIds {
		ENUMS(STEP_PARAMS, SEQ_NUM_STEPS),
		ENUMS(CV_PARAMS, SEQ_NUM_STEPS),
		LENGTH_PARAM,
		DIRECTION_PARAM,
		ADDR_PARAM,
		RANGE_SW_PARAM,
		HOLD_PARAM,		
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
		GATE_OUTPUT,
		TRIG_OUTPUT,
		END_OUTPUT,
		CV_OUTPUT,
		CVI_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(STEP_LIGHTS, SEQ_NUM_STEPS),
		GATE_LIGHT,
		TRIG_LIGHT,
		ENUMS(LENGTH_LIGHTS, SEQ_NUM_STEPS),
		ENUMS(DIRECTION_LIGHTS, 5),
		ONESHOT_LIGHT,
		END_LIGHT,
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
	int length = SEQ_NUM_STEPS;
	int direction = FORWARD;
	int directionMode = FORWARD;
	bool oneShot = false;
	bool oneShotEnded = false;
	float moduleVersion = 0.0f;
	
	float cv = 0.0f;
	bool prevGate = false;
	bool running = false;
		
	float lengthCVScale = (float)(SEQ_NUM_STEPS);
	
	SequencerChannelMessage rightMessages[2][1]; // messages to right module (expander)
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	STRUCT_NAME() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// length, direction and address params
		configParam(LENGTH_PARAM, 1.0f, (float)(SEQ_NUM_STEPS), (float)(SEQ_NUM_STEPS), "Length");
		configParam(DIRECTION_PARAM, 0.0f, 8.0f, 0.0f, "Direction");
		configParam(ADDR_PARAM, 0.0f, 10.0f, 0.0f, "Address");
		
		// cv/gate params
		char stepText[20];
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			sprintf(stepText, "Step %d select", s + 1);
			configParam(STEP_PARAMS + s, 0.0f, 2.0f, 1.0f, stepText);
			
			sprintf(stepText, "Step %d value", s + 1);
			configParam(CV_PARAMS + s, 0.0f, 1.0f, 0.0f, stepText, " V", 0.0f, 8.0f, 0.0f);
		}
		
		// range switch
		configParam(RANGE_SW_PARAM, 1.0f, 8.0f, 8.0f, "Scale");
		
		// hold mode switch
		configParam(HOLD_PARAM, 0.0f, 2.0f, 1.0f, "Sample and hold mode");
		
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
		
		// expander
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];		
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("1.0"));
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
		json_t *ver = json_object_get(root, "version");

		if (ver)
			moduleVersion = json_number_value(ver);		
		
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
		length = SEQ_NUM_STEPS;
		direction = FORWARD;
		directionMode = FORWARD;
		pgClock.reset();
		oneShot = false;
		oneShotEnded = false;
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
					count = SEQ_NUM_STEPS + 1;
					break;
			}
			
			direction = nextDir;
		}
		
		// process the clock trigger - we'll use this to allow the run input edge to act like the clock if it arrives shortly after the clock edge
		bool clockEdge = gateClock.leadingEdge();
		if (clockEdge)
			pgClock.trigger(1e-4f);
		else
			clockEdge = (pgClock.process(args.sampleTime) && gateRun.leadingEdge());
	
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
		
		bool gate = false, trig = false;
		
		// determine which scale to use
		float scale = params[RANGE_SW_PARAM].getValue();

		// process the step switches, cv and set the length/active step lights etc
		for (int c = 0; c < SEQ_NUM_STEPS; c++) {

			// set step and length lights here
			bool stepActive = (c + 1 == count);
			lights[STEP_LIGHTS + c].setBrightness(boolToLight(stepActive));
			lights[LENGTH_LIGHTS + c].setBrightness(boolToLight(c < length));

			// process the gate and CV for the current step
			if (stepActive) {
				switch ((int)(params[STEP_PARAMS + c].getValue())) {
					case 0:
						gate = running;
						break;
					case 2:
						trig = running;
						break;
				}
			
				// now grab the cv value
				switch ((int)(params[HOLD_PARAM].getValue())) {
					case 0: // on trig
						cv = trig ? params[CV_PARAMS + c].getValue(): cv;
						break;
					
					case 2: // on gate
						cv = (gate && ! prevGate) ? params[CV_PARAMS + c].getValue(): cv;
						break;
					case 1: // off
					default:
						cv = params[CV_PARAMS + c].getValue();
						break;
				}
			}
		}
		
		// now we can set the outputs and lights
		bool clock = gateClock.high();
		outputs[GATE_OUTPUT].setVoltage(boolToGate(gate));
		outputs[TRIG_OUTPUT].setVoltage(boolToGate(trig && clock));
		lights[GATE_LIGHT].setBrightness(boolToLight(gate));
		lights[TRIG_LIGHT].setBrightness(boolToLight(trig && clock));
		
		outputs[CV_OUTPUT].setVoltage(cv * scale);
		outputs[CVI_OUTPUT].setVoltage(-cv * scale);
		
		outputs[END_OUTPUT].setVoltage(boolToGate(oneShotEnded));
		lights[END_LIGHT].setBrightness(boolToLight(oneShotEnded));
		prevGate = gate;
		
		// set up details for the expander
		if (rightExpander.module) {
			if (isExpanderModule(rightExpander.module)) {
				
				SequencerChannelMessage *messageToExpander = (SequencerChannelMessage*)(rightExpander.module->leftExpander.producerMessage);
				messageToExpander->set(count, length, clock, running, 1, true);
				
				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}
	}
};

struct WIDGET_NAME : ModuleWidget {
	WIDGET_NAME(STRUCT_NAME *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" PANEL_FILE)));

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
		addParam(createParamCentered<CountModulaRotarySwitchRed>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 22.5, STD_HALF_ROWS8(STD_ROW3)), module, STRUCT_NAME::LENGTH_PARAM));
		addParam(createParamCentered<CountModulaRotarySwitchBlue>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 15, STD_ROWS8[STD_ROW5]), module, STRUCT_NAME::DIRECTION_PARAM));
		addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 15, STD_ROWS8[STD_ROW8]), module, STRUCT_NAME::ADDR_PARAM));

		// step lights
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[s > 7 ? STD_COL8 : STD_COL4], STD_ROWS8[STD_ROW1 + (s % 8)] - 19), module, STRUCT_NAME::LENGTH_LIGHTS + s));
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[s > 7 ? STD_COL9 : STD_COL5] + 15, STD_ROWS8[STD_ROW1 + (s % 8)]), module, STRUCT_NAME::STEP_LIGHTS + s));
		}
		
		// direction and one-shot light
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_HALF_ROWS8(STD_ROW5) + 3), module, STRUCT_NAME::DIRECTION_LIGHTS));
		addChild(createLightCentered<SmallLight<YellowLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_HALF_ROWS8(STD_ROW5) + 17), module, STRUCT_NAME::DIRECTION_LIGHTS + 1));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_HALF_ROWS8(STD_ROW5) + 31), module, STRUCT_NAME::DIRECTION_LIGHTS + 2));
		addChild(createLightCentered<SmallLight<BlueLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_HALF_ROWS8(STD_ROW5) + 45), module, STRUCT_NAME::DIRECTION_LIGHTS + 3));
		addChild(createLightCentered<SmallLight<WhiteLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_HALF_ROWS8(STD_ROW5) + 59), module, STRUCT_NAME::DIRECTION_LIGHTS + 4));
		
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 22.5, STD_HALF_ROWS8(STD_ROW5) + 10), module, STRUCT_NAME::ONESHOT_LIGHT));
		
		// step switches and pots
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			addParam(createParamCentered<CountModulaToggle3P90>(Vec(STD_COLUMN_POSITIONS[s > 7 ? STD_COL8 : STD_COL4] + 15, STD_ROWS8[STD_ROW1 + (s % 8)]), module, STRUCT_NAME:: STEP_PARAMS + s));
			addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[s > 7 ? STD_COL10 : STD_COL6] + 15, STD_ROWS8[STD_ROW1 + (s % 8)]), module, STRUCT_NAME:: CV_PARAMS + s));
		}

		// determine where the final column of controls goes
		int lastCol = (SEQ_NUM_STEPS > 8 ? STD_COL12 : STD_COL8);
		
		// range control
		addParam(createParamCentered<CountModulaRotarySwitchGrey>(Vec(STD_COLUMN_POSITIONS[lastCol] + 15, STD_HALF_ROWS8(STD_ROW5)), module, STRUCT_NAME::RANGE_SW_PARAM));
			
		// hold mode control
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[lastCol] + 15, STD_ROWS8[STD_ROW4]), module, STRUCT_NAME::HOLD_PARAM));
			
		// output lights and jacks
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[lastCol] + 15, STD_HALF_ROWS8(STD_ROW1)-20), module, STRUCT_NAME::GATE_LIGHT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[lastCol] + 15, STD_HALF_ROWS8(STD_ROW1)), module, STRUCT_NAME::GATE_OUTPUT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[lastCol] + 15, STD_ROWS8[STD_ROW3]-20), module, STRUCT_NAME::TRIG_LIGHT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[lastCol] + 15, STD_ROWS8[STD_ROW3]), module, STRUCT_NAME::TRIG_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[lastCol] + 15, STD_ROWS8[STD_ROW7]), module, STRUCT_NAME::CV_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[lastCol] + 15, STD_ROWS8[STD_ROW8]), module, STRUCT_NAME::CVI_OUTPUT));
		
		// one-shot end
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 15, STD_HALF_ROWS8(STD_ROW6)), module, STRUCT_NAME::END_OUTPUT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 4, STD_HALF_ROWS8(STD_ROW6) - 20), module, STRUCT_NAME::END_LIGHT));
	}
	

	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"
	
	struct InitMenuItem : MenuItem {
		WIDGET_NAME *widget;
		bool triggerInit = true;
		bool cvInit = true;
		
		void onAction(const event::Action &e) override {

		// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			if (!triggerInit && cvInit)
				h->name = "initialize cv";
			else if (triggerInit && !cvInit)
				h->name = "initialize gates/triggers";
			else
				h->name = "initialize cv/gates/triggers";
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();
		
			// step controls
			for (int c = 0; c < SEQ_NUM_STEPS; c++) {
				// triggers/gates
				if (triggerInit)
					widget->getParam(STRUCT_NAME::STEP_PARAMS + c)->reset();
				
				// cv knobs
				if (cvInit)
					widget->getParam(STRUCT_NAME::CV_PARAMS + c)->reset();
			}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};	
	
	// initialize menu item
	struct InitMenu : MenuItem {
		WIDGET_NAME *widget;		
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			// CV only init
			InitMenuItem *initCVMenuItem = createMenuItem<InitMenuItem>("CV Only");
			initCVMenuItem->widget = widget;
			initCVMenuItem->triggerInit = false;
			menu->addChild(initCVMenuItem);

			// gate/trigger only init
			InitMenuItem *initTrigMenuItem = createMenuItem<InitMenuItem>("Gates/Triggers Only");
			initTrigMenuItem->widget = widget;
			initTrigMenuItem->cvInit = false;
			menu->addChild(initTrigMenuItem);

			// CV/gate/trigger only init
			InitMenuItem *initCVTrigMenuItem = createMenuItem<InitMenuItem>("CV/Gates/Triggers Only");
			initCVTrigMenuItem->widget = widget;
			menu->addChild(initCVTrigMenuItem);

			return menu;	
		}
	};		
	
	struct RandMenuItem : MenuItem {
		WIDGET_NAME *widget;
		bool triggerRand = true;
		bool cvRand = true;
	
		void onAction(const event::Action &e) override {
		
			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			if (!triggerRand && cvRand)
				h->name = "randomize cv";
			else if (triggerRand && !cvRand)
				h->name = "randomize gates/triggers";
			else
				h->name = "randomize cv/gates/triggers";
			
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();

			// step controls
			for (int c = 0; c < SEQ_NUM_STEPS; c++) {
				// triggers/gates
				if (triggerRand)
					widget->getParam(STRUCT_NAME::STEP_PARAMS + c)->randomize();
				
				if (cvRand)
					widget->getParam(STRUCT_NAME::CV_PARAMS + c)->randomize();
			}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};	
	
	// randomize menu item
	struct RandMenu : MenuItem {
		WIDGET_NAME *widget;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			// CV only random
			RandMenuItem *randCVMenuItem = createMenuItem<RandMenuItem>("CV Only");
			randCVMenuItem->widget = widget;
			randCVMenuItem->triggerRand = false;
			menu->addChild(randCVMenuItem);

			// gate/trigger only random
			RandMenuItem *randTrigMenuItem = createMenuItem<RandMenuItem>("Gates/Triggers Only");
			randTrigMenuItem->widget = widget;
			randTrigMenuItem->cvRand = false;
			menu->addChild(randTrigMenuItem);	
			
			// gate/trigger only random
			RandMenuItem *randCVTrigMenuItem = createMenuItem<RandMenuItem>("CV/Gates/Triggers Only");
			randCVTrigMenuItem->widget = widget;
			menu->addChild(randCVTrigMenuItem);	
			
			return menu;	
		}
	};
	
	void appendContextMenu(Menu *menu) override {
		STRUCT_NAME *module = dynamic_cast<STRUCT_NAME*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());

		// add the theme menu items
		#include "../themes/themeMenus.hpp"
		
		// add initialize menu item
		InitMenu *initMenuItem = createMenuItem<InitMenu>("Initialize", RIGHT_ARROW);
		initMenuItem->widget = this;
		menu->addChild(initMenuItem);

		// add randomize menu item
		RandMenu *randMenuItem = createMenuItem<RandMenu>("Randomize", RIGHT_ARROW);
		randMenuItem->widget = this;
		menu->addChild(randMenuItem);

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
