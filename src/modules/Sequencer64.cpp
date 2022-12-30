//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - 64 Step Sequencer Module
//	A 64 Step CV/Gate sequencer
//	Copyright (C) 2022  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

#define SEQ_NUM_STEPS 64
#define SEQ_NUM_PATTERNS 4
#define SEQ_NUM_COLS 8

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Sequencer64
#define PANEL_FILE "Sequencer64.svg"

struct Sequencer64 : Module {

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
		ENUMS(STEP_PARAM_LIGHTS, SEQ_NUM_STEPS),
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
	
	int count = 0;
	int length = SEQ_NUM_STEPS;
	int direction = FORWARD;
	int directionMode = FORWARD;
	bool oneShot = false;
	bool oneShotEnded = false;
	float moduleVersion = 1.1f;

	float cv = 0.0f;
	bool prevGate = false;
	bool running = false;
		
	float lengthCVScale = (float)(SEQ_NUM_STEPS - 1);
	
	int processCount = 8;
	
	float scale = 1.0f;
	int lengthParam = 64;
	int directionParam = FORWARD;
	float addressParam = 10.0f;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	Sequencer64() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// length, direction and address params
		configParam(LENGTH_PARAM, 1.0f, (float)(SEQ_NUM_STEPS), (float)(SEQ_NUM_STEPS), "Length");
		configSwitch(DIRECTION_PARAM, 0.0f, 8.0f, 0.0f, "Direction", {"Forward", "Pendulum", "Reverse", "Random", "Forward 1-Shot", "Pendulum 1-Shot", "Reverse 1-Shot", "Random 1-Shot", "Voltage addressed"});
		configParam(ADDR_PARAM, 0.0f, 10.0f, 0.0f, "Address");
		
		// cv/gate params
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			configSwitch(STEP_PARAMS + s, 0.0f, 1.0f, 1.0f, rack::string::f("Step %d select", s + 1), {"Off", "On"});
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
		
		processCount = 8;
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
			moduleVersion = json_number_value(version);			

		if (currentStep)
			count = json_integer_value(currentStep);
		
		if (dir)
			direction = json_integer_value(dir);
		
		if (clk)
			gateClock.preset(json_boolean_value(clk));

		if (run) 
			gateRun.preset(json_boolean_value(run));
		
		running = gateRun.high();
		
		processCount = 8;
		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"		
		
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
		lengthParam = 64;
		processCount = 8;
		directionParam = FORWARD;
		addressParam = 10.0f;
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
		// reset input
		gateReset.set(inputs[RESET_INPUT].getVoltage());
		
		// run input
		gateRun.set(inputs[RUN_INPUT].getNormalVoltage(10.0f));

		// clock
		gateClock.set(inputs[CLOCK_INPUT].getVoltage());

		// grab common params
		if (++processCount > 8) {
			processCount = 0;
			
			// determine which scale to use
			scale = params[RANGE_SW_PARAM].getValue();
			directionParam = (int)(params[DIRECTION_PARAM].getValue());
			lengthParam = (int)(params[LENGTH_PARAM].getValue());
			addressParam = params[ADDR_PARAM].getValue();
		}
		
		// sequence length - jack overrides knob
		if (inputs[LENGTH_INPUT].isConnected()) {
			// scale the input such that 10V = step 16, 0V = step 1
			length = (int)(clamp(lengthCVScale/10.0f * inputs[LENGTH_INPUT].getVoltage(), 0.0f, lengthCVScale)) + 1;
		}
		else {
			length = lengthParam;
		}
		
		// direction - jack overrides the switch
		if (inputs[DIRECTION_INPUT].isConnected()) {
			float dirCV = clamp(inputs[DIRECTION_INPUT].getVoltage(), 0.0f, 8.99f);
			directionMode = (int)floor(dirCV);
		}
		else
			directionMode = directionParam;

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
						float v = clamp(inputs[ADDRESS_INPUT].getNormalVoltage(10.0f), 0.0f, 10.0f) * addressParam;
						count = 1 + (int)((length) * v /100.0f);
						break;
				}
				
				if (count > length)
					count = length;
			}
		}
		
		bool gate = false, trig = false;
		

		// process the step switches, cv and set the length/active step lights etc
		for (int c = 0; c < SEQ_NUM_STEPS; c++) {

			// set step and length lights here
			bool stepActive = (c + 1 == count);
			lights[STEP_LIGHTS + c].setBrightness(boolToLight(stepActive));
			lights[LENGTH_LIGHTS + c].setBrightness(boolToLight(c < length));

			// process the gate and CV for the current step
			if (stepActive) {
				if(params[STEP_PARAMS + c].getValue() > 0.5f) {
					gate = trig = running;
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
		if (gate) {
			outputs[GATE_OUTPUT].setVoltage(10.0f);
			lights[GATE_LIGHT].setBrightness(1.0f);	
		}
		else {
			outputs[GATE_OUTPUT].setVoltage(0.0f);
			lights[GATE_LIGHT].setBrightness(0.0f);
		}
		
		if (oneShotEnded) {
			outputs[END_OUTPUT].setVoltage(10.0f);
			lights[END_LIGHT].setBrightness(1.0f);
		}
		else {
			outputs[END_OUTPUT].setVoltage(0.0f);
			lights[END_LIGHT].setBrightness(0.0f);
		}
		
		if (trig && clock) {
			outputs[TRIG_OUTPUT].setVoltage(10.0f);
			lights[TRIG_LIGHT].setBrightness(10.0f);
		}
		else {
			outputs[TRIG_OUTPUT].setVoltage(0.0f);
			lights[TRIG_LIGHT].setBrightness(0.0f);
		}
		
		outputs[CV_OUTPUT].setVoltage(cv * scale);
		outputs[CVI_OUTPUT].setVoltage(-cv * scale);
		
		prevGate = gate;
	}
};

struct Sequencer64Widget : ModuleWidget {

	std::string panelName;
	
	Sequencer64Widget(Sequencer64 *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"	

		// screws
		#include "../components/stdScrews.hpp"	

		// run input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1]), module, Sequencer64::RUN_INPUT));

		// reset input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2]), module, Sequencer64::RESET_INPUT));
		
		// clock input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 15, STD_ROWS8[STD_ROW1]), module, Sequencer64::CLOCK_INPUT));
				
		// CV input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 15, STD_ROWS8[STD_ROW2]), module, Sequencer64::LENGTH_INPUT));

		// direction and address input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW5]), module, Sequencer64::DIRECTION_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, Sequencer64::ADDRESS_INPUT));
		
		// length & direction params
		addParam(createParamCentered<RotarySwitch<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 22.5, STD_HALF_ROWS8(STD_ROW3)), module, Sequencer64::LENGTH_PARAM));
		addParam(createParamCentered<RotarySwitch<BlueKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 15, STD_ROWS8[STD_ROW5]), module, Sequencer64::DIRECTION_PARAM));
		addParam(createParamCentered<Potentiometer<WhiteKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 15, STD_ROWS8[STD_ROW8]), module, Sequencer64::ADDR_PARAM));

		// step lights, switches and pots
		int row = STD_ROW1;
		int col = STD_COL5;
		int x = 1;
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[col], STD_ROWS8[row] - 19), module, Sequencer64::LENGTH_LIGHTS + s));
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[col] + 15, STD_ROWS8[row] - 19), module, Sequencer64::STEP_LIGHTS + s));
			switch (x) {
				case 1:
					addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<RedLight>>>(Vec(STD_COLUMN_POSITIONS[col] - 15, STD_ROWS8[row]), module, Sequencer64:: STEP_PARAMS + s, Sequencer64:: STEP_PARAM_LIGHTS + s) );
					addParam(createParamCentered<Potentiometer<SmallKnob<RedKnob>>>(Vec(STD_COLUMN_POSITIONS[col] + 15, STD_ROWS8[row]), module, Sequencer64:: CV_PARAMS + s));
					break;
				case 5:
					addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<YellowLight>>>(Vec(STD_COLUMN_POSITIONS[col] - 15, STD_ROWS8[row]), module, Sequencer64:: STEP_PARAMS + s, Sequencer64:: STEP_PARAM_LIGHTS + s) );
					addParam(createParamCentered<Potentiometer<SmallKnob<YellowKnob>>>(Vec(STD_COLUMN_POSITIONS[col] + 15, STD_ROWS8[row]), module, Sequencer64:: CV_PARAMS + s));
					break;
				default:
					addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[col] - 15, STD_ROWS8[row]), module, Sequencer64:: STEP_PARAMS + s, Sequencer64:: STEP_PARAM_LIGHTS + s) );
					addParam(createParamCentered<Potentiometer<SmallKnob<GreenKnob>>>(Vec(STD_COLUMN_POSITIONS[col] + 15, STD_ROWS8[row]), module, Sequencer64:: CV_PARAMS + s));
					break;
			}
			
			if (++x > SEQ_NUM_COLS)
				x = 1;
			
			col += 2;
			if (col > STD_COL19) {
				col = STD_COL5;
				row += 1;
			}
			
		}
		
		// direction and one-shot light
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_HALF_ROWS8(STD_ROW5) + 3), module, Sequencer64::DIRECTION_LIGHTS));
		addChild(createLightCentered<SmallLight<YellowLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_HALF_ROWS8(STD_ROW5) + 17), module, Sequencer64::DIRECTION_LIGHTS + 1));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_HALF_ROWS8(STD_ROW5) + 31), module, Sequencer64::DIRECTION_LIGHTS + 2));
		addChild(createLightCentered<SmallLight<BlueLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_HALF_ROWS8(STD_ROW5) + 45), module, Sequencer64::DIRECTION_LIGHTS + 3));
		addChild(createLightCentered<SmallLight<WhiteLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_HALF_ROWS8(STD_ROW5) + 59), module, Sequencer64::DIRECTION_LIGHTS + 4));
		
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 22.5, STD_HALF_ROWS8(STD_ROW5) + 10), module, Sequencer64::ONESHOT_LIGHT));

		// determine where the final column of controls goes
		int lastCol = STD_COL21;
		
		// range control
		addParam(createParamCentered<RotarySwitch<GreyKnob>>(Vec(STD_COLUMN_POSITIONS[lastCol] + 15, STD_HALF_ROWS8(STD_ROW5)), module, Sequencer64::RANGE_SW_PARAM));
			
		// hold mode control
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[lastCol] + 15, STD_ROWS8[STD_ROW4]), module, Sequencer64::HOLD_PARAM));
			
		// output lights and jacks
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[lastCol] + 15, STD_HALF_ROWS8(STD_ROW1)-20), module, Sequencer64::GATE_LIGHT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[lastCol] + 15, STD_HALF_ROWS8(STD_ROW1)), module, Sequencer64::GATE_OUTPUT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[lastCol] + 15, STD_ROWS8[STD_ROW3]-20), module, Sequencer64::TRIG_LIGHT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[lastCol] + 15, STD_ROWS8[STD_ROW3]), module, Sequencer64::TRIG_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[lastCol] + 15, STD_ROWS8[STD_ROW7]), module, Sequencer64::CV_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[lastCol] + 15, STD_ROWS8[STD_ROW8]), module, Sequencer64::CVI_OUTPUT));
		
		// one-shot end
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 15, STD_HALF_ROWS8(STD_ROW6)), module, Sequencer64::END_OUTPUT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 4, STD_HALF_ROWS8(STD_ROW6) - 20), module, Sequencer64::END_LIGHT));
	}
	

	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"
	
	struct InitMenuItem : MenuItem {
		Sequencer64Widget *widget;
		bool triggerInit = true;
		bool cvInit = true;
		int range = -1;
		
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
			if (range < 0) {
				// all of them
				for (int c = 0; c < SEQ_NUM_STEPS; c++) {
					if (triggerInit)
						widget->getParam(Sequencer64::STEP_PARAMS + c)->getParamQuantity()->reset();
					
					if (cvInit)
						widget->getParam(Sequencer64::CV_PARAMS + c)->getParamQuantity()->reset();
				}
			}
			else {
				// selected range only
				int c = range - 1;
				for (int i = 0; i < SEQ_NUM_COLS; i++) {
					// triggers/gates
					if (triggerInit)
						widget->getParam(Sequencer64::STEP_PARAMS + c)->getParamQuantity()->reset();
					
					// cv knobs
					if (cvInit)
						widget->getParam(Sequencer64::CV_PARAMS + c)->getParamQuantity()->reset();
					
					c++;
				}
			}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);
		}
	};	
	
	// initialize option menu item
	struct InitOptionMenu : MenuItem {
		Sequencer64Widget *widget;
		int range = -1;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			// CV only init
			InitMenuItem *initCVMenuItem = createMenuItem<InitMenuItem>("CV Only");
			initCVMenuItem->widget = widget;
			initCVMenuItem->triggerInit = false;
			initCVMenuItem->range = range;
			
			menu->addChild(initCVMenuItem);

			// gate/trigger only init
			InitMenuItem *initTrigMenuItem = createMenuItem<InitMenuItem>("Gates/Triggers Only");
			initTrigMenuItem->widget = widget;
			initTrigMenuItem->cvInit = false;
			initTrigMenuItem->range = range;
			menu->addChild(initTrigMenuItem);

			// CV/gate/trigger only init
			InitMenuItem *initCVTrigMenuItem = createMenuItem<InitMenuItem>("CV/Gates/Triggers Only");
			initCVTrigMenuItem->widget = widget;
			initCVTrigMenuItem->range = range;
			menu->addChild(initCVTrigMenuItem);

			return menu;
		}
	};
	
		// initialize menu item
	struct InitMenu : MenuItem {
		Sequencer64Widget *widget;		
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			// all steps
			InitOptionMenu *initMenuItem = createMenuItem<InitOptionMenu>("All rows", RIGHT_ARROW);
			initMenuItem->widget = widget;
			initMenuItem->range = -1;
			menu->addChild(initMenuItem);

			// specific steps
			std::ostringstream  buffer;
			int r = 1;
			int x = SEQ_NUM_COLS - 1;
			for (int s = 1; s < SEQ_NUM_STEPS; s += SEQ_NUM_COLS) {
				buffer.str("");
				buffer << "Row " << r++ << " (steps " << s << "-" << s + x << ")";
				InitOptionMenu *m = createMenuItem<InitOptionMenu>(buffer.str(), RIGHT_ARROW);
				m->widget = widget;
				m->range = s;
				menu->addChild(m);
			}

			return menu;
		}
	};
	
	struct RandMenuItem : MenuItem {
		Sequencer64Widget *widget;
		bool triggerRand = true;
		bool cvRand = true;
		int range = -1;
			
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
			if (range < 0) {
				// all of them
				for (int c = 0; c < SEQ_NUM_STEPS; c++) {
					if (triggerRand)
						widget->getParam(Sequencer64::STEP_PARAMS + c)->getParamQuantity()->randomize();
					
					if (cvRand)
						widget->getParam(Sequencer64::CV_PARAMS + c)->getParamQuantity()->randomize();
				}
			}
			else {
				// selected range only
				int c = range -1;
				for (int i = 0; i < SEQ_NUM_COLS; i++) {
					if (triggerRand)
						widget->getParam(Sequencer64::STEP_PARAMS + c)->getParamQuantity()->randomize();
					
					if (cvRand)
						widget->getParam(Sequencer64::CV_PARAMS + c)->getParamQuantity()->randomize();
					
					c++;
				}
			}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};	
	
	// randomize menu item
	struct RandOptionMenu : MenuItem {
		Sequencer64Widget *widget;
		int range = -1;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			// CV only init
			RandMenuItem *randCVMenuItem = createMenuItem<RandMenuItem>("CV Only");
			randCVMenuItem->widget = widget;
			randCVMenuItem->triggerRand = false;
			randCVMenuItem->range = range;
			menu->addChild(randCVMenuItem);

			// gate/trigger only init
			RandMenuItem *randTrigMenuItem = createMenuItem<RandMenuItem>("Gates/Triggers Only");
			randTrigMenuItem->widget = widget;
			randTrigMenuItem->cvRand = false;
			randTrigMenuItem->range = range;
			menu->addChild(randTrigMenuItem);

			// CV/gate/trigger only init
			RandMenuItem *randCVTrigMenuItem = createMenuItem<RandMenuItem>("CV/Gates/Triggers Only");
			randCVTrigMenuItem->widget = widget;
			randCVTrigMenuItem->range = range;
			menu->addChild(randCVTrigMenuItem);
			
			return menu;	
		}
	};
	
	// randomize menu item
	struct RandMenu : MenuItem {
		Sequencer64Widget *widget;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			// CV only random
			RandOptionMenu *randMenuItem = createMenuItem<RandOptionMenu>("All rows", RIGHT_ARROW);
			randMenuItem->widget = widget;
			menu->addChild(randMenuItem);

			// specific steps
			std::ostringstream  buffer;
			int r = 1;
			int x = SEQ_NUM_COLS -1;
			for (int s = 1; s < SEQ_NUM_STEPS; s += SEQ_NUM_COLS) {
				buffer.str("");
				buffer << "Row " << r++ << " (steps " << s << "-" << s + x << ")";
				RandOptionMenu *m = createMenuItem<RandOptionMenu>(buffer.str(), RIGHT_ARROW);
				m->widget = widget;
				m->range = s;
				menu->addChild(m);
			}
			
			return menu;
		}
	};
	
	// clone menu item
	struct CloneMenuItem : MenuItem {
		Sequencer64Widget *widget;
		bool triggerDupe = true;
		bool cvDupe = true;
		int source = -1;
		int dest = -1;
	
		void onAction(const event::Action &e) override {
		
			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			if (!triggerDupe && cvDupe)
				h->name = "clone cv";
			else if (triggerDupe && !cvDupe)
				h->name = "clone gates/triggers";
			else
				h->name = "clone cv/gates/triggers";
			
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();

			if (dest > 0) {
				// duplicating into specific row
				int s = source -1;
				int d = dest - 1;
				for (int c = 0; c < SEQ_NUM_COLS; c++) {
					if (triggerDupe) {
						float v = widget->getParam(Sequencer64::STEP_PARAMS + s)->getParamQuantity()->getValue();
						widget->getParam(Sequencer64::STEP_PARAMS + d)->getParamQuantity()->setValue(v);
					}
					
					if (cvDupe) {
						float v = widget->getParam(Sequencer64::CV_PARAMS + s)->getParamQuantity()->getValue();
						widget->getParam(Sequencer64::CV_PARAMS + d)->getParamQuantity()->setValue(v);
					}
					
					d++;
					s++;
				}
			}
			else {
				// duplicating into all rows, start at row 1 and loop through
				dest = 1;
				
				while (dest < SEQ_NUM_STEPS) {
					if (dest != source) {
						int s = source -1;
						int d = dest - 1;

						for (int i = 0; i < SEQ_NUM_COLS; i++) {
							if (triggerDupe) {
								float v = widget->getParam(Sequencer64::STEP_PARAMS + s)->getParamQuantity()->getValue();
								widget->getParam(Sequencer64::STEP_PARAMS + d)->getParamQuantity()->setValue(v);
							}
							
							if (cvDupe) {
								float v = widget->getParam(Sequencer64::CV_PARAMS + s)->getParamQuantity()->getValue();
								widget->getParam(Sequencer64::CV_PARAMS + d)->getParamQuantity()->setValue(v);
							}
							s++;
							d++;
						}
					}
					dest += SEQ_NUM_COLS;
				}
			}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};

	// clone option menu item
	struct CloneOptionMenu : MenuItem {
		Sequencer64Widget *widget;
		int source = -1;
		int dest = -1;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			// CV only random
			CloneMenuItem *dupeCVMenuItem = createMenuItem<CloneMenuItem>("CV Only");
			dupeCVMenuItem->widget = widget;
			dupeCVMenuItem->triggerDupe = false;
			dupeCVMenuItem->source = source;
			dupeCVMenuItem->dest = dest;
			menu->addChild(dupeCVMenuItem);

			// gate/trigger only random
			CloneMenuItem *dupeTrigMenuItem = createMenuItem<CloneMenuItem>("Gates/Triggers Only");
			dupeTrigMenuItem->widget = widget;
			dupeTrigMenuItem->cvDupe = false;
			dupeTrigMenuItem->source = source;
			dupeTrigMenuItem->dest = dest;
			menu->addChild(dupeTrigMenuItem);	
			
			// gate/trigger only random
			CloneMenuItem *dupeCVTrigMenuItem = createMenuItem<CloneMenuItem>("CV and Gates/Triggers");
			dupeCVTrigMenuItem->widget = widget;
			dupeCVTrigMenuItem->source = source;
			dupeCVTrigMenuItem->dest = dest;
			menu->addChild(dupeCVTrigMenuItem);	
			
			return menu;
		}
	};

	// clone menu
	struct CloneDestMenu : MenuItem {
		Sequencer64Widget *widget;
		int source = -1;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			std::ostringstream  buffer;
			int r = 1;
			int x = SEQ_NUM_COLS -1;
			for (int d = 1; d < SEQ_NUM_STEPS; d += SEQ_NUM_COLS) {
				if (d != source) {
					buffer.str("");
					buffer << "Into row " << r << " (steps " << d << "-" << d + x << ")";
					CloneOptionMenu *m = createMenuItem<CloneOptionMenu>(buffer.str(), RIGHT_ARROW);
					m->widget = widget;
					m->source = source;
					m->dest = d;
					menu->addChild(m);
				}
				r++;
			}
			
			CloneOptionMenu *m = createMenuItem<CloneOptionMenu>("Into all other rows", RIGHT_ARROW);
			m->widget = widget;
			m->source = source;
			m->dest = -1;
			menu->addChild(m);

			return menu;
		}
	};

	// clone menu
	struct CloneSourceMenu : MenuItem {
		Sequencer64Widget *widget;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			std::ostringstream  buffer;
			int r = 1;
			int x = SEQ_NUM_COLS -1;
			for (int s = 1; s < SEQ_NUM_STEPS; s += SEQ_NUM_COLS) {
				buffer.str("");
				buffer << "Row " << r++ << " (steps " << s << "-" << s + x << ")";
				CloneDestMenu *m = createMenuItem<CloneDestMenu>(buffer.str(), RIGHT_ARROW);
				m->widget = widget;
				m-> source = s;
				menu->addChild(m);
			}
			
			return menu;	
		}
	};
	
	// pattern menu item
	struct PatternMenuItem : MenuItem {
		Sequencer64Widget *widget;
		int patternId = 0;
		int dest = -1;
	
		const float patterns[SEQ_NUM_PATTERNS][SEQ_NUM_COLS] = {	{ 0, 0, 0, 0, 0, 0, 0, 0 },
																	{ 1, 0, 0, 0, 1, 0, 0, 0 },
																	{ 1, 0, 1, 0, 1, 0, 1, 0 },
																	{ 1, 0, 0, 0, 1, 0, 1, 0 }
		};	
	
		void onAction(const event::Action &e) override {
		
			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = "trigger pattern";
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();

			if (dest > 0) {
				// setting specific row
				int d = dest - 1;
				for (int c = 0; c < SEQ_NUM_COLS; c++) {
					widget->getParam(Sequencer64::STEP_PARAMS + d)->getParamQuantity()->setValue(patterns[patternId][c]);
					d++;
				}
			}
			else {
				// setting all rows
				for (int d = 0; d < SEQ_NUM_STEPS; d++){
					widget->getParam(Sequencer64::STEP_PARAMS + d)->getParamQuantity()->setValue(patterns[patternId][d % SEQ_NUM_COLS]);
				}
			}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};
	
	// pattern option menu item
	struct PatternOptionMenu : MenuItem {
		Sequencer64Widget *widget;
		int dest = -1;
		
		const std::string patternNames [SEQ_NUM_PATTERNS] = {	"All off",
																"X - - - X - - -",
																"X - X - X - X -",
																"X - - - X - X -"
		};
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			PatternMenuItem *offMenuItem = createMenuItem<PatternMenuItem>(patternNames[0]);
			offMenuItem->widget = widget;
			offMenuItem->dest = dest;
			offMenuItem->patternId = 0;
			menu->addChild(offMenuItem);

			for (int i = 1; i < SEQ_NUM_PATTERNS; i++) {
				PatternMenuItem *m = createMenuItem<PatternMenuItem>(patternNames[i]);
				m->widget = widget;
				m->dest = dest;
				m->patternId = i;
				menu->addChild(m);
			}
			
			return menu;
		}
	};	
	
	// pattern menu
	struct PatternMenu : MenuItem {
		Sequencer64Widget *widget;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			// all rows
			PatternOptionMenu *patternMenuItem = createMenuItem<PatternOptionMenu>("All rows", RIGHT_ARROW);
			patternMenuItem->widget = widget;
			menu->addChild(patternMenuItem);

			// specific steps
			std::ostringstream  buffer;
			int r = 1;
			int x = SEQ_NUM_COLS -1;
			for (int s = 1; s < SEQ_NUM_STEPS; s += SEQ_NUM_COLS) {
				buffer.str("");
				buffer << "Row " << r++ << " (steps " << s << "-" << s + x << ")";
				PatternOptionMenu *m = createMenuItem<PatternOptionMenu>(buffer.str(), RIGHT_ARROW);
				m->widget = widget;
				m->dest = s;
				menu->addChild(m);
			}
			
			return menu;
		}
	};
	
	void appendContextMenu(Menu *menu) override {
		Sequencer64 *module = dynamic_cast<Sequencer64*>(this->module);
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

		// add randomize menu item
		CloneSourceMenu *dupeMenuItem = createMenuItem<CloneSourceMenu>("Clone", RIGHT_ARROW);
		dupeMenuItem->widget = this;
		menu->addChild(dupeMenuItem);
		
		// add set pattern menu item
		PatternMenu *patMenuItem = createMenuItem<PatternMenu>("Pattern", RIGHT_ARROW);
		patMenuItem->widget = this;
		menu->addChild(patMenuItem);
	}

	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}	
};

Model *modelSequencer64 = createModel<Sequencer64, Sequencer64Widget>("Sequencer64");
