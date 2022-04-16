//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Standard switch Engine
//	Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------

struct STRUCT_NAME : Module {

	enum ParamIds {
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
#ifdef ONE_TO_N
		SIGNAL_INPUT,
#endif
#ifdef N_TO_ONE
		ENUMS(SIGNAL_INPUTS, SEQ_NUM_STEPS),
#endif
		NUM_INPUTS
	};
	enum OutputIds {
#ifdef ONE_TO_N
		ENUMS(SIGNAL_OUTPUTS, SEQ_NUM_STEPS),
#endif
#ifdef N_TO_ONE
		SIGNAL_OUTPUT,
#endif

		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(STEP_LIGHTS, SEQ_NUM_STEPS),
		ENUMS(LENGTH_LIGHTS, SEQ_NUM_STEPS),
		ENUMS(DIRECTION_LIGHTS, 5),
		NUM_LIGHTS
	};
	
	enum Directions {
		FORWARD,
		PENDULUM,
		REVERSE,
		RANDOM,
		ADDRESSED
	};
	
	enum SampleModes {
		TRACK_MODE,
		NORMAL_MODE,
		SAMPLE_MODE
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
	float moduleVersion = 0.0f;
	
	float cv = 0.0f;
	bool prevGate = false;
	bool running = false;
	
	float lengthCVScale = (float)(SEQ_NUM_STEPS);

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	STRUCT_NAME() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// length, direction and address params
		configParam(LENGTH_PARAM, 1.0f, (float)(SEQ_NUM_STEPS), (float)(SEQ_NUM_STEPS), "Length");
		configSwitch(DIRECTION_PARAM, 0.0f, 4.0f, 0.0f, "Direction", {"Forward", "Pendulum", "Reverse", "Random", "Voltage addressed"});
		configParam(ADDR_PARAM, 0.0f, 10.0f, 0.0f, "Address");
		
		// hold mode switch
		configSwitch(HOLD_PARAM, 0.0f, 2.0f, 1.0f, "Sample and hold mode", {"Track & Hold", "Through", "Sample & Hold"});
		
		configInput(RUN_INPUT, "Run");
		configInput(CLOCK_INPUT, "Clock");
		configInput(RESET_INPUT, "Reset");
		configInput(LENGTH_INPUT, "Length CV");
		configInput(DIRECTION_INPUT, "Direction CV");
		configInput(ADDRESS_INPUT, "Address CV");
		
#ifdef ONE_TO_N
		configInput(SIGNAL_INPUT, "Signal");
#endif
#ifdef N_TO_ONE
		configOutput(SIGNAL_OUTPUT, "Signal");
#endif

	for (int s = 0; s < SEQ_NUM_STEPS; s++) {
#ifdef ONE_TO_N		
			configOutput(SIGNAL_OUTPUTS + s, rack::string::f("%d", s+1));
#endif
#ifdef N_TO_ONE
			configInput(SIGNAL_INPUTS + s,rack::string::f("%d", s+1));
#endif
		}
		
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
	}

	int recalcDirection() {
		
		switch (directionMode) {
			case PENDULUM:
				return (direction == FORWARD ? REVERSE : FORWARD);
			case FORWARD:
			case REVERSE:
			default:
				return directionMode;
		}
	}	
	
	void setDirectionLight() {

		float red, yellow, green, blue, white;
		switch (directionMode) {
			case FORWARD:
				green = 1.0f;
				red = yellow = blue = white = 0.0f;
				directionMode = FORWARD;
				break;
			case PENDULUM:
				yellow = 0.8f;
				red = green = blue = white = 0.0f;
				directionMode = PENDULUM;
				break;
			case REVERSE:				red = 1.0f;
				yellow = green = blue = white = 0.0f;
				directionMode = REVERSE;
				break;
			case RANDOM:
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
			float dirCV = clamp(inputs[DIRECTION_INPUT].getVoltage(), 0.0f, 4.99f);
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
			
			switch (direction) {
				case FORWARD:
					count++;
					
					if (count > length) {
						if (nextDir == FORWARD)
							count = 1;
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
						if (nextDir == REVERSE)
							count = length;
						else {
							// in pendulum mode we change direction here
							count++;
							direction = nextDir;
						}
					}
					break;
					
				case RANDOM:
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
		
		// set step and length lights here
		for (int c = 0; c < SEQ_NUM_STEPS; c++) {
			lights[STEP_LIGHTS + c].setBrightness(boolToLight(c + 1 == count));
			lights[LENGTH_LIGHTS + c].setBrightness(boolToLight(c < length));
		}
		
		// what mode are we in?
		int mode;
		switch ((int)(params[HOLD_PARAM].getValue())) {
			case SAMPLE_MODE:
				mode =SAMPLE_MODE;
				break;
			case TRACK_MODE:
				mode = TRACK_MODE;
				break;
			case NORMAL_MODE:
			default:
				mode = NORMAL_MODE;
				break;
		}

		// process the switch function
#ifdef ONE_TO_N		
		if (count > 0 && inputs[SIGNAL_INPUT].isConnected()) {
			int numChans = inputs[SIGNAL_INPUT].getChannels();
			
			for (int i = 0; i < SEQ_NUM_STEPS; i++) {
				outputs[SIGNAL_OUTPUTS + i].setChannels(numChans);
				if (i + 1 == count) {
					if (mode != SAMPLE_MODE || clockEdge) {
						for (int c = 0; c < numChans; c++)
							outputs[SIGNAL_OUTPUTS + i].setVoltage(inputs[SIGNAL_INPUT].getVoltage(c),c);
					}
				}
				else {
					if (mode == NORMAL_MODE) {
						for (int c = 0; c < numChans; c++)
							outputs[SIGNAL_OUTPUTS + i].setVoltage(0.0f, c);
					}
				}
			}
		}
		else {
			for (int i = 0; i < SEQ_NUM_STEPS; i++) {
				outputs[SIGNAL_OUTPUTS + i].setChannels(1);
				outputs[SIGNAL_OUTPUTS + i].setVoltage(0.0f);
			}
		}
#endif
#ifdef N_TO_ONE
		if (count > 0) {
			int j = count - 1;

			if (inputs[SIGNAL_INPUTS + j].isConnected()) {
				int numChans = inputs[SIGNAL_INPUTS + j].getChannels();
				outputs[SIGNAL_OUTPUT].setChannels(numChans);

				if (mode != SAMPLE_MODE || clockEdge) {
					for (int c = 0; c < numChans; c++)
						outputs[SIGNAL_OUTPUT].setVoltage(inputs[SIGNAL_INPUTS + j].getVoltage(c), c);
				}
			}
			else {
				if (mode == NORMAL_MODE) {
					outputs[SIGNAL_OUTPUT].setChannels(1);
					outputs[SIGNAL_OUTPUT].setVoltage(0.0f);
				}				
			}
		}
		else {
			outputs[SIGNAL_OUTPUT].setChannels(1);
			outputs[SIGNAL_OUTPUT].setVoltage(0.0f);
		}
#endif
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
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 15, STD_ROWS8[STD_ROW6]), module, STRUCT_NAME::ADDRESS_INPUT));

		// signal input/output
#ifdef ONE_TO_N		
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, STRUCT_NAME::SIGNAL_INPUT));
#endif
#ifdef N_TO_ONE
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, STRUCT_NAME::SIGNAL_OUTPUT));
#endif
				
		// length & direction params
		addParam(createParamCentered<RotarySwitch<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 15, STD_HALF_ROWS8(STD_ROW3)), module, STRUCT_NAME::LENGTH_PARAM));
		addParam(createParamCentered<RotarySwitch<OperatingAngle145<BlueKnob>>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 15, STD_ROWS8[STD_ROW5]), module, STRUCT_NAME::DIRECTION_PARAM));
		addParam(createParamCentered<Potentiometer<WhiteKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 15, STD_HALF_ROWS8(STD_ROW7)), module, STRUCT_NAME::ADDR_PARAM));

		// hold mode control
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW3)), module, STRUCT_NAME::HOLD_PARAM));
		
		// step lights and signal outputs/inputs
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[s > 7 ? STD_COL6 : STD_COL4], STD_ROWS8[STD_ROW1 + (s % 8)] - 19), module, STRUCT_NAME::LENGTH_LIGHTS + s));
			addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[s > 7 ? STD_COL7 : STD_COL5], STD_ROWS8[STD_ROW1 + (s % 8)] - 19), module, STRUCT_NAME::STEP_LIGHTS + s));
#ifdef ONE_TO_N		
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[s > 7 ? STD_COL6 : STD_COL4] + 15, STD_ROWS8[STD_ROW1 + (s % 8)]), module, STRUCT_NAME::SIGNAL_OUTPUTS + s));
#endif
#ifdef N_TO_ONE
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[s > 7 ? STD_COL6 : STD_COL4] + 15, STD_ROWS8[STD_ROW1 + (s % 8)]), module, STRUCT_NAME::SIGNAL_INPUTS + s));	
#endif
		}
		
		// direction and one-shot light
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_HALF_ROWS8(STD_ROW5) + 3), module, STRUCT_NAME::DIRECTION_LIGHTS));
		addChild(createLightCentered<SmallLight<YellowLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_HALF_ROWS8(STD_ROW5) + 17), module, STRUCT_NAME::DIRECTION_LIGHTS + 1));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_HALF_ROWS8(STD_ROW5) + 31), module, STRUCT_NAME::DIRECTION_LIGHTS + 2));
		addChild(createLightCentered<SmallLight<BlueLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_HALF_ROWS8(STD_ROW5) + 45), module, STRUCT_NAME::DIRECTION_LIGHTS + 3));
		addChild(createLightCentered<SmallLight<WhiteLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_HALF_ROWS8(STD_ROW5) + 59), module, STRUCT_NAME::DIRECTION_LIGHTS + 4));
	}
	

	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"
	
	void appendContextMenu(Menu *menu) override {
		STRUCT_NAME *module = dynamic_cast<STRUCT_NAME*>(this->module);
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

Model *MODEL_NAME = createModel<STRUCT_NAME, WIDGET_NAME>(MODULE_NAME);
