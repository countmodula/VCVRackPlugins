//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Standard Sequencer Engine
//	Copyright (C) 2020  Adam Verspaget
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
		ENUMS(TRIGGER_PARAMS, SEQ_NUM_STEPS),
		ENUMS(GATE_PARAMS, SEQ_NUM_STEPS),
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
		ENUMS(TRIGGER_PARAM_LIGHTS, SEQ_NUM_STEPS),
		ENUMS(GATE_PARAM_LIGHTS, SEQ_NUM_STEPS),
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

	int moduleVersion = 2;

	float cv = 0.0f;
	bool prevGate = false;
	bool running = false;
		
	float lengthCVScale = (float)(SEQ_NUM_STEPS - 1);
	
	SequencerChannelMessage rightMessages[2][1]; // messages to right module (expander)
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	STRUCT_NAME() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// length, direction and address params
		configParam(LENGTH_PARAM, 1.0f, (float)(SEQ_NUM_STEPS), (float)(SEQ_NUM_STEPS), "Length");
		configSwitch(DIRECTION_PARAM, 0.0f, 8.0f, 0.0f, "Direction", {"Forward", "Pendulum", "Reverse", "Random", "Forward 1-Shot", "Pendulum 1-Shot", "Reverse 1-Shot", "Random 1-Shot", "Voltage addressed"});
		configParam(ADDR_PARAM, 0.0f, 10.0f, 0.0f, "Address");
		
		// cv/gate params
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			configSwitch(STEP_PARAMS + s, 0.0f, 2.0f, 1.0f, rack::string::f("Step %d select", s + 1), {"Gate", "Off", "Trigger"}); // for previous version compatibility
			configSwitch(TRIGGER_PARAMS + s, 0.0f, 1.0f, 1.0f, rack::string::f("Step %d trigger select", s + 1), {"Off", "On"});
			configSwitch(GATE_PARAMS + s, 0.0f, 1.0f, 0.0f, rack::string::f("Step %d gate select", s + 1), {"Off", "On"});
			configParam(CV_PARAMS + s, 0.0f, 1.0f, 0.0f, rack::string::f("Step %d value", s + 1), " V", 0.0f, 8.0f, 0.0f);
		}
		
		// range switch
		configSwitch(RANGE_SW_PARAM, 1.0f, 8.0f, 8.0f, "Scale", {"1 Volt", "2 Volts", "3 Volts", "4 Volts", "5 Volts", "6 Volts", "7 Volts", "8 Volts"} );
		
		// hold mode switch
		configSwitch(HOLD_PARAM, 0.0f, 2.0f, 1.0f, "Sample and hold mode", {"Trigger", "Off", "Gate"});
		
		configInput(RUN_INPUT, "Run");
		configInput(CLOCK_INPUT, "Clock");
		configInput(RESET_INPUT, "Reset");
		configInput(LENGTH_INPUT, "Length CV");
		configInput(DIRECTION_INPUT, "Direction CV");
		
		configInput(ADDRESS_INPUT, "Voltage addressed CV");
		inputInfos[ADDRESS_INPUT]->description = "When connected, this input is attenuated by the Address knob";		
		
		configOutput(GATE_OUTPUT, "Gate");
		configOutput(TRIG_OUTPUT, "Trigger");
		configOutput(CV_OUTPUT, "CV");
		configOutput(CVI_OUTPUT, "Inverted CV");
		
		configOutput(END_OUTPUT, "1-Shot end");
		outputInfos[END_OUTPUT]->description = "Gate output, goes high at the end of the 1-Shot cycle";
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
		
		// expander
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];		
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(moduleVersion));
		json_object_set_new(root, "currentStep", json_integer(count));
		json_object_set_new(root, "direction", json_integer(direction));
		json_object_set_new(root, "clockState", json_boolean(gateClock.high()));
		json_object_set_new(root, "runState", json_boolean(gateRun.high()));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}
	
	void dataFromJson(json_t* root) override {
		
		json_t *version = json_object_get(root, "moduleVersion");
		json_t *currentStep = json_object_get(root, "currentStep");
		json_t *dir = json_object_get(root, "direction");
		json_t *clk = json_object_get(root, "clockState");
		json_t *run = json_object_get(root, "runState");

		if (version) 
			moduleVersion = json_integer_value(version);			
		else {
			// no version in file is really old version.
			moduleVersion = 0;
		}
	
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
		
		// conversion to new step select switches
		if (moduleVersion < 2) {
			for(int i = 0; i < SEQ_NUM_STEPS; i++) {
				int x = (int)(params[STEP_PARAMS + i].getValue());

				params[GATE_PARAMS + i].setValue(x == 0 ? 1.0f : 0.0f);
				params[TRIGGER_PARAMS + i].setValue(x == 2 ? 1.0f : 0.0f);
			}
			
			moduleVersion = 2;
		}

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
				if(params[TRIGGER_PARAMS + c].getValue() > 0.5f) {
					trig = running;
				}
				
				if(params[GATE_PARAMS + c].getValue() > 0.5f) {
					gate = running;
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

	std::string panelName;
	
	WIDGET_NAME(STRUCT_NAME *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"	

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
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[s > 7 ? STD_COL8 : STD_COL4] + 15, STD_ROWS8[STD_ROW1 + (s % 8)] - 8), module, STRUCT_NAME::LENGTH_LIGHTS + s));
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[s > 7 ? STD_COL8 : STD_COL4] + 15, STD_ROWS8[STD_ROW1 + (s % 8)] + 5), module, STRUCT_NAME::STEP_LIGHTS + s));
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

			addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<RedLight>>>(Vec(STD_COLUMN_POSITIONS[s > 7 ? STD_COL8 : STD_COL4] - 5, STD_ROWS8[STD_ROW1 + (s % 8)]), module, STRUCT_NAME:: TRIGGER_PARAMS + s, STRUCT_NAME:: TRIGGER_PARAM_LIGHTS + s));
			addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[s > 7 ? STD_COL9 : STD_COL5] + 5, STD_ROWS8[STD_ROW1 + (s % 8)]), module, STRUCT_NAME:: GATE_PARAMS + s, STRUCT_NAME:: GATE_PARAM_LIGHTS + s));

			addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[s > 7 ? STD_COL10 : STD_COL6] + 15, STD_ROWS8[STD_ROW1 + (s % 8)]), module, STRUCT_NAME:: CV_PARAMS + s));
		}

		// determine where the final column of controls goes
		int lastCol = (SEQ_NUM_STEPS > 8 ? STD_COL12 : STD_COL8);
		
		// range control
		addParam(createParamCentered<RotarySwitch<GreyKnob>>(Vec(STD_COLUMN_POSITIONS[lastCol] + 15, STD_HALF_ROWS8(STD_ROW5)), module, STRUCT_NAME::RANGE_SW_PARAM));
			
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
		int which;

		std::string undoLabels[4] = {"initialize cv", "initialize gates", "initialize triggers", "initialize cv/gates/triggers"};
	
		void onAction(const event::Action &e) override {

			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = undoLabels[which];
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();
		
			// step controls
			switch (which) {
				case 0: // cv only
					for (int c = 0; c < SEQ_NUM_STEPS; c++) {
						widget->getParam(STRUCT_NAME::CV_PARAMS + c)->getParamQuantity()->reset();
					}
					break;					
				case 1: // gates only
					for (int c = 0; c < SEQ_NUM_STEPS; c++) {
						widget->getParam(STRUCT_NAME::GATE_PARAMS + c)->getParamQuantity()->reset();
					}
					break;				
				case 2: // triggers
					for (int c = 0; c < SEQ_NUM_STEPS; c++) {
						widget->getParam(STRUCT_NAME::TRIGGER_PARAMS + c)->getParamQuantity()->reset();
					}
					break;				
				case 3: // cv/gates/triggers
					for (int c = 0; c < SEQ_NUM_STEPS; c++) {
						widget->getParam(STRUCT_NAME::CV_PARAMS + c)->getParamQuantity()->reset();
						widget->getParam(STRUCT_NAME::GATE_PARAMS + c)->getParamQuantity()->reset();
						widget->getParam(STRUCT_NAME::TRIGGER_PARAMS + c)->getParamQuantity()->reset();
					}
					break;					
			}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};	
	
	// initialize menu item
	struct InitMenu : MenuItem {
		WIDGET_NAME *widget;		
		std::string initLabels[4] = {"CV only", "Gates only", "Triggers only", "CV/Gates/Triggers only"};
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			for (int i = 0; i < 4; i++) {
				InitMenuItem *initMenuItem = createMenuItem<InitMenuItem>(initLabels[i]);
				initMenuItem->widget = widget;
				initMenuItem->which = i;
				menu->addChild(initMenuItem);
			}

			return menu;	
		}
	};		
	
	void doRandom(bool triggerRand, bool cvRand)
	{
		// history - current settings
		history::ModuleChange *h = new history::ModuleChange;
		if (!triggerRand && cvRand)
			h->name = "randomize cv";
		else if (triggerRand && !cvRand)
			h->name = "randomize gates/triggers";
		else
			h->name = "randomize cv/gates/triggers";
		
		h->moduleId = this->module->id;
		h->oldModuleJ = this->toJson();

		// step controls
		for (int c = 0; c < SEQ_NUM_STEPS; c++) {
			// triggers/gates
			if (triggerRand)
				this->getParam(STRUCT_NAME::STEP_PARAMS + c)->getParamQuantity()->randomize();
			
			if (cvRand)
				this->getParam(STRUCT_NAME::CV_PARAMS + c)->getParamQuantity()->randomize();
		}

		// history - new settings
		h->newModuleJ = this->toJson();
		APP->history->push(h);	
	}
	
	
	std::string undoRandLabels[4] = {"randomize cv", "randomize gates", "randomize triggers", "randomize cv/gates/triggers"};
	
	void doRandom(int which) {
		// history - current settings
		history::ModuleChange *h = new history::ModuleChange;
		h->name = undoRandLabels[which];
		h->moduleId = this->module->id;
		h->oldModuleJ = this->toJson();

		// step controls
		switch (which) {
			case 0: // cv only
				for (int c = 0; c < SEQ_NUM_STEPS; c++) {
					this->getParam(STRUCT_NAME::CV_PARAMS + c)->getParamQuantity()->randomize();
				}
				break;					
			case 1: // gates only
				for (int c = 0; c < SEQ_NUM_STEPS; c++) {
					this->getParam(STRUCT_NAME::GATE_PARAMS + c)->getParamQuantity()->randomize();
				}
				break;				
			case 2: // triggers
				for (int c = 0; c < SEQ_NUM_STEPS; c++) {
					this->getParam(STRUCT_NAME::TRIGGER_PARAMS + c)->getParamQuantity()->randomize();
				}
				break;				
			case 3: // cv/gates/triggers
				for (int c = 0; c < SEQ_NUM_STEPS; c++) {
					this->getParam(STRUCT_NAME::CV_PARAMS + c)->getParamQuantity()->randomize();
					this->getParam(STRUCT_NAME::GATE_PARAMS + c)->getParamQuantity()->randomize();
					this->getParam(STRUCT_NAME::TRIGGER_PARAMS + c)->getParamQuantity()->randomize();
				}
				break;					
		}

		// history - new settings
		h->newModuleJ = this->toJson();
		APP->history->push(h);	
	}

	// randomize selection menu item
	struct RandMenuItem : MenuItem {
		WIDGET_NAME *widget;
		int which;

		void onAction(const event::Action &e) override {
			widget->doRandom(which);
		}
	};
	
	// randomize menu item
	struct RandMenu : MenuItem {
		WIDGET_NAME *widget;
		std::string randLabels[4] = {"CV only", "Gates only", "Triggers only", "CV/Gates/Triggers only"};
		std::string randKeys[4] = {"Shitf+Ctrl+C", "Shitf+Ctrl+G", "Shitf+Ctrl+T", "Shitf+Ctrl+R"};

		Menu *createChildMenu() override {
			Menu *menu = new Menu;
			
			for (int i = 0;i < 4; i++) {
				// CV only random
				RandMenuItem *randMenuItem = createMenuItem<RandMenuItem>(randLabels[i], randKeys[i]);
				randMenuItem->widget = widget;
				randMenuItem->which = i;
				menu->addChild(randMenuItem);
			}
		
			return menu;
		}
	};

	
	// expander addition menu item
	#include "../inc/AddExpanderMenuItem.hpp"
	struct ExpanderMenu : MenuItem {
		STRUCT_NAME *module;
		Vec position;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

	
			
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
				
		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel("Sequence"));
		
		// add initialize menu item
		InitMenu *initMenuItem = createMenuItem<InitMenu>("Initialize", RIGHT_ARROW);
		initMenuItem->widget = this;
		menu->addChild(initMenuItem);

		// add randomize menu item
		RandMenu *randMenuItem = createMenuItem<RandMenu>("Randomize", RIGHT_ARROW);
		randMenuItem->widget = this;
		menu->addChild(randMenuItem);

		// add expander menu items
		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel("Expansion"));
	
		AddExpanderMenuItem *channelMenuItem = createMenuItem<AddExpanderMenuItem>("Add channel expander");
		channelMenuItem->module = module;
#if SEQ_NUM_STEPS == 8
		channelMenuItem->model = modelSequencerChannel8;
#elif SEQ_NUM_STEPS == 16
		channelMenuItem->model = modelSequencerChannel16;
#endif
		channelMenuItem->position = box.pos;
		channelMenuItem->expanderName = "channel";
		menu->addChild(channelMenuItem);
		
		AddExpanderMenuItem *gateMenuItem = createMenuItem<AddExpanderMenuItem>("Add gate expander");
		gateMenuItem->module = module;
#if SEQ_NUM_STEPS == 8
		gateMenuItem->model = modelSequencerGates8;
#elif SEQ_NUM_STEPS == 16
		gateMenuItem->model = modelSequencerGates16;
#endif
		gateMenuItem->position = box.pos;
		gateMenuItem->expanderName = "gate";
		menu->addChild(gateMenuItem);	
			
		AddExpanderMenuItem *trigMenuItem = createMenuItem<AddExpanderMenuItem>("Add trigger expander");
		trigMenuItem->module = module;
#if SEQ_NUM_STEPS == 8
		trigMenuItem->model = modelSequencerTriggers8;
#elif SEQ_NUM_STEPS == 16
		trigMenuItem->model = modelSequencerTriggers16;
#endif
		trigMenuItem->position = box.pos;
		trigMenuItem->expanderName = "trigger";
		menu->addChild(trigMenuItem);				
			
	}
	
	void onHoverKey(const event::HoverKey &e) override {
		if (e.action == GLFW_PRESS && (e.mods & RACK_MOD_MASK) == (RACK_MOD_CTRL | GLFW_MOD_SHIFT)) {
			
			switch (e.key) {
				case GLFW_KEY_C:
					doRandom(0);
					e.consume(this);
					break;
				case GLFW_KEY_G:
					doRandom(1);
					e.consume(this);
					break;						
				case GLFW_KEY_T:
					doRandom(2);
					e.consume(this);
					break;	
				case GLFW_KEY_R:
					doRandom(3);
					e.consume(this);
					break;				
			}
		}

		ModuleWidget::onHoverKey(e);
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
