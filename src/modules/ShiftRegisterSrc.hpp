//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Shift Register engine
//	Copyright (C) 2023  Adam Verspaget
//----------------------------------------------------------------------------

struct STRUCT_NAME : Module {
	
	enum LoopModes {
		LOOP_LOCKED,
		LOOP_TAKES_MINIMUM,
		LOOP_TAKES_MAXIMUM,
		LOOP_TAKES_AVERAGE,
		LOOP_INVERTS,
		NUM_LOOP_MODES
	};
		
	
	enum ParamIds {
		LOOP_ENABLE_PARAM,
		LOOP_MODE_PARAM,
		REVERSE_PARAM,
		CHANCE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		SHIFT_INPUT,
		LOOP_ENABLE_INPUT,
		REVERSE_INPUT,
		LOOP_MODE_INPUT,
		CHANCE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(CV_OUTPUTS, MAX_LEN),
		RANDOM_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(CV_LIGHTS, MAX_LEN*2),
		LOOP_ENABLE_LIGHT,
		REVERSE_LIGHT,
		ANALOGUE_LIGHT,
		DIGITAL_LIGHT,
		ENUMS(LOOP_MODE_LIGHTS, NUM_LOOP_MODES),
		NUM_LIGHTS
	};
	
	enum randomRanges {
		RANDOM_PLUS10,
		RANDOM_PLUSMINUS10,
		RANDOM_PLUS5,
		RANDOM_PLUSMINUS5,
		NUM_RANDOM_RANGES
	};
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	GateProcessor triggerGP;
	GateProcessor reverseGP;
	GateProcessor loopEnableGP;
	GateProcessor cvInputGP;

	float out[MAX_LEN] = {};	
	float randomOut = 0.0f;
	
	bool reverse;	
	float reverseCV = 0.0f;
	
	bool loopEnabled;
	float loopEnableCV = 0.0f;
	int loopMode = LOOP_LOCKED;
	
	bool digitalMode;
	bool prevDigitalMode;
	int randomRange = RANDOM_PLUS10;
	
	STRUCT_NAME() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configSwitch(LOOP_ENABLE_PARAM, 0.0f, 1.0f, 0.0f, "Loop enable", {"Disabled", "Enabled"});
		configSwitch(LOOP_MODE_PARAM, 0.0f, 4.0f, 0.0f, "Mode", {"Lock", "Minimum/AND", "Maximum/OR", "Averaage/XOR", "Invert"});
		configSwitch(REVERSE_PARAM, 0.0f, 1.0f, 0.0f, "Direction", {"Forward", "Reverse"});
		
		configParam(CHANCE_PARAM, 0.0f, 1.0f, 0.0f, "Chance loop changes", " %", 0.0f, 100.0f, 0.0f);

		configInput(CV_INPUT, "CV");
		configInput(SHIFT_INPUT, "Shift");
		configInput(LOOP_ENABLE_INPUT, "Loop enable");
		configInput(REVERSE_INPUT, "Direction");
		configInput(LOOP_MODE_INPUT, "Loop mode");
		configInput(CHANCE_INPUT, "Chance");
		
		for (int i=0; i < MAX_LEN; i++) {
			configOutput(CV_OUTPUTS + i, std::to_string(i + 1));
		}																						

		configOutput(RANDOM_OUTPUT, "Sampled random");
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
		
		onReset();
	}

	void onReset() override {
		loopEnabled = reverse = false;
		loopEnableCV = reverseCV = 0.0f;
		loopMode = LOOP_LOCKED;
		
		triggerGP.reset();
		reverseGP.reset();
		loopEnableGP.reset();
		cvInputGP.reset();
		
		prevDigitalMode = digitalMode = false;
		resetShiftRegister();
	}
	
	void resetShiftRegister() {
		for (int i = 0; i < MAX_LEN; i++)
			out[i] = 0.0f;		
	}

	json_t *dataToJson() override {
		json_t *root = json_object();
		
		json_t *ov = json_array();
		for (int i = 0; i < MAX_LEN; i++) {
			json_array_insert_new(ov, i, json_real(out[i]));
		}		
		
		json_object_set_new(root, "moduleVersion", json_integer(1));
		json_object_set_new(root, "digitalMode", json_boolean(digitalMode));
		json_object_set_new(root, "randomRange", json_integer(randomRange));
		json_object_set_new(root, "shiftState", json_boolean(triggerGP.high()));
		json_object_set_new(root, "outputValues", ov);
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		

		return root;
	}
	
	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
		
		digitalMode = false;
		json_t *om = json_object_get(root, "digitalMode");
		if (om) {
			prevDigitalMode = digitalMode = json_boolean_value(om);		
		}
		
		randomRange = RANDOM_PLUS10;
		json_t *rr = json_object_get(root, "randomRange");
		if (rr) {
			randomRange = json_integer_value(rr);
		}
		
		json_t *ov = json_object_get(root, "outputValues");
		for (int i = 0; i < MAX_LEN; i++) {
			if (ov) {
				json_t *v = json_array_get(ov, i);
				if (v) {
					out[i] = json_real_value(v);
				}
				else {
					out[i] = 0.0f;
				}
			}
			else {
				out[i] = 0.0f;
			}
		}

		triggerGP.reset();
		json_t *sh = json_object_get(root, "shiftState");		
		if (sh)
			triggerGP.preset(json_boolean_value(sh));	
	}
	
	float processLoop(float existingCV, float newCV, float chance) {
		float result = 0.0f;
		
		// apply chance of change logic
		bool doChange = (random::uniform() < chance);
		
		switch (loopMode) {
			case LOOP_INVERTS:
				// if the result of the chance toss above is true, we want to insert the new value instead of inverting the existing one 
				if (doChange) {
					if (digitalMode) {
						result = boolToGate(newCV > 2.0f);
					}
					else {
						result = newCV;
					}					
				}
				else {
					if (digitalMode) {
						result = boolToGate(existingCV <= 2.0f);
					}
					else {
						result = -existingCV;
					}
				}
				break;
			case LOOP_TAKES_MINIMUM: // analogue AND
				// if the result of the above chance toss is true, we want to do the AND operation, otherwise we keep the existing cv
				if (doChange) {
					result = fminf(existingCV, newCV);
				}
				else {
					result = existingCV;
				}
				break;
			case LOOP_TAKES_MAXIMUM: // analogue OR
				// if the result of the above chance toss is true, we want to do the OR operation, otherwise we keep the existing cv
				if (doChange) {
					result = fmaxf(existingCV, newCV);
				}
				else {
					result = existingCV;
				}
				break;
			case LOOP_TAKES_AVERAGE:
				// if the result of the above chance toss is true, we want to do the average/XOR operation, otherwise we keep the existing cv
				if (digitalMode) {
					if (doChange) {
						// digital mode is XOR logic function.
						bool a = existingCV > 2.0f;
						bool b = newCV > 2.0f;
						result = boolToGate(a != b);
					}
					else {
						result = boolToGate(existingCV);
					}
				}
				else {
					if (doChange) {
						result = (existingCV + newCV) / 2.0f;
					}
					else {
						result = existingCV;
					}
				}
				break;
			default: // Locked
				// if the result of the chance toss above is true, we want to insert the new value instead of keeping the existing one
				float cvToUse = doChange ? newCV : existingCV;
				if (digitalMode) {
					result = boolToGate(cvToUse > 2.0f);
				}
				else {
					result = cvToUse;
				}
				break;
		}

		
		return result;
	}	
	
	float getRandomValue() {
		
		if (digitalMode) {
			// in digital mode, we use a 50% chance model for the gates
			return random::uniform() > 0.5f ? 10.0f : 0.0f;
		}
		else {
			switch (randomRange) {
				case RANDOM_PLUS5:
					return random::uniform() * 5.0f;
				case RANDOM_PLUSMINUS5:
					return random::uniform() * 10.0f - 5.0f;
				case RANDOM_PLUSMINUS10:
					return random::uniform() * 20.0f - 10.0f;
				case RANDOM_PLUS10:
				default:
					return random::uniform() * 10.0f;
			}
		}
	}
	
	void process(const ProcessArgs &args) override {
		
		// todo: only every 8 cycles...
		reverseCV = params[REVERSE_PARAM].getValue()*10.0f;
		loopEnableCV = params[LOOP_ENABLE_PARAM].getValue()*10.0f;
		
		if (inputs[LOOP_MODE_INPUT].isConnected()) {
			float lmCV = clamp(inputs[LOOP_MODE_INPUT].getVoltage(), 0.0f, 4.99f);
			loopMode = (int)floor(lmCV);
		}
		else {
			loopMode = (int)(params[LOOP_MODE_PARAM].getValue());
		}
		
		reverseCV = inputs[REVERSE_INPUT].getNormalVoltage(reverseCV);			
		reverse = reverseGP.set(reverseCV);
		
		loopEnableCV = inputs[LOOP_ENABLE_INPUT].getNormalVoltage(loopEnableCV);			
		loopEnabled = loopEnableGP.set(loopEnableCV);
		
		triggerGP.set(inputs[SHIFT_INPUT].getVoltage());
		
		// only on clock transition from 0 to 1
		if (triggerGP.leadingEdge()) {
			
			// only need this whenever we trigger
			float chanceCV = params[CHANCE_PARAM].getValue();
			if (inputs[CHANCE_INPUT].isConnected()) {
				chanceCV = chanceCV * clamp(inputs[CHANCE_INPUT].getVoltage()/10.0f, 0.0f, 1.0f);
			}
		
			if (prevDigitalMode != digitalMode) {
				resetShiftRegister();
				
				prevDigitalMode = digitalMode;
			}
			
			randomOut = getRandomValue();
			float cv = inputs[CV_INPUT].getNormalVoltage(randomOut);
			cvInputGP.set(cv);
			
			if (digitalMode) {
				cv = cvInputGP.value();
			}
			
			if (reverse) {
				
				// handle loop function here
				if (loopEnabled) {
					// in reverse, cell 0 loops around to the last cell
					cv = processLoop(out[0], cv, chanceCV);
				}				
				
				// shift current values back 1
				for (int i = 0; i < LAST_CELL; i++) {
					out[i] = out[i+1];
				}
				
				// insert new value at end
				out[LAST_CELL] = cv;
			}
			else {
				// handle loop function here
				if (loopEnabled) {
					// normal operation, last cell loops around to cell 0
					cv = processLoop(out[LAST_CELL], cv, chanceCV);
				}
				
				// shift current values along 1
				for (int i = LAST_CELL; i > 0; i--) {
					out[i] = out[i-1];
				}
				
				// insert new value at start
				out[0] = cv;
			}
		}
			
		// set outputs and lights
		for (int i = 0; i < MAX_LEN; i++) {
			outputs[CV_OUTPUTS + i].setVoltage(out[i]);
			
			if (out[i] < 0.0f) {
				lights[CV_LIGHTS + (i * 2)].setBrightness(-out[i]/10.0f);
				lights[CV_LIGHTS + (i * 2) + 1].setBrightness(0.0f);
			}
			else {
				lights[CV_LIGHTS + (i * 2)].setBrightness(0.0f);			}
				lights[CV_LIGHTS + (i * 2) + 1].setBrightness(out[i]/10.0f);	
		}
		
		outputs[RANDOM_OUTPUT].setVoltage(randomOut);
		
		int l = LOOP_MODE_LIGHTS;
		for (int i = 0; i < NUM_LOOP_MODES; i++) {
			lights[l++].setBrightness(boolToLight(loopMode == i));
		}
		
		lights[REVERSE_LIGHT].setBrightness(boolToLight(reverse));
		lights[LOOP_ENABLE_LIGHT].setBrightness(boolToLight(loopEnabled));
		lights[ANALOGUE_LIGHT].setBrightness(boolToLight(!digitalMode));
		lights[DIGITAL_LIGHT].setBrightness(boolToLight(digitalMode));
	}
};

struct WIDGET_NAME : ModuleWidget {

	std::string panelName;

	const int outputRows[MAX_LEN] = ROW_POSITIONS
	const int outputCols[MAX_LEN] = COL_POSITIONS
	
	const float customColumns[7] = {30, 52.5, 75, 120, 165, 210, 255};
	
	WIDGET_NAME(STRUCT_NAME *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"
		
		// screws
		#include "../components/stdScrews.hpp"	

		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(customColumns[STD_COL1], STD_ROWS8[STD_ROW1]), module, STRUCT_NAME::SHIFT_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(customColumns[STD_COL3], STD_ROWS8[STD_ROW1]), module, STRUCT_NAME::CV_INPUT));

		// reverse controls
		addInput(createInputCentered<CountModulaJack>(Vec(customColumns[STD_COL1], STD_HALF_ROWS8(STD_ROW3)), module, STRUCT_NAME::REVERSE_INPUT));
		addParam(createParamCentered<CountModulaToggle2P>(Vec(customColumns[STD_COL3], STD_HALF_ROWS8(STD_ROW3)), module, STRUCT_NAME::REVERSE_PARAM));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(customColumns[STD_COL2], STD_HALF_ROWS8(STD_ROW3)-8), module, STRUCT_NAME::REVERSE_LIGHT));
		
		// loop controls
		addInput(createInputCentered<CountModulaJack>(Vec(customColumns[STD_COL1], STD_HALF_ROWS8(STD_ROW4)), module, STRUCT_NAME::LOOP_ENABLE_INPUT));
		addParam(createParamCentered<CountModulaToggle2P>(Vec(customColumns[STD_COL3], STD_HALF_ROWS8(STD_ROW4)), module, STRUCT_NAME::LOOP_ENABLE_PARAM));
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(customColumns[STD_COL2], STD_HALF_ROWS8(STD_ROW4)-8), module, STRUCT_NAME::LOOP_ENABLE_LIGHT));
		addInput(createInputCentered<CountModulaJack>(Vec(customColumns[STD_COL1], STD_HALF_ROWS8(STD_ROW5)), module, STRUCT_NAME::CHANCE_INPUT));
		addParam(createParamCentered<Potentiometer<WhiteKnob>>(Vec(customColumns[STD_COL3], STD_HALF_ROWS8(STD_ROW5)), module, STRUCT_NAME::CHANCE_PARAM));
		addParam(createParamCentered<RotarySwitch<OperatingAngle120<Left90<BlueKnob>>>>(Vec(customColumns[STD_COL3], STD_ROWS8[STD_ROW7]), module, STRUCT_NAME::LOOP_MODE_PARAM));
		addInput(createInputCentered<CountModulaJack>(Vec(customColumns[STD_COL2], STD_ROWS8[STD_ROW8]), module, STRUCT_NAME::LOOP_MODE_INPUT));
		
		float led = STD_ROWS8[STD_ROW7] + 20.0f;
		for (int i = 0; i < STRUCT_NAME::NUM_LOOP_MODES; i++) {
			addChild(createLightCentered<SmallLight<BlueLight>>(Vec(customColumns[STD_COL1]-20, led), module, STRUCT_NAME::LOOP_MODE_LIGHTS + i));
			led -= 10.0f;
		}
		
		// outputs
		for (int i=0; i < MAX_LEN; i++) {
			addOutput(createOutputCentered<CountModulaJack>(Vec(customColumns[outputCols[i]], STD_ROWS8[outputRows[i]]), module, STRUCT_NAME::CV_OUTPUTS + i));
			addChild(createLightCentered<MediumLight<CountModulaLightRG>>(Vec(customColumns[outputCols[i]] + 10, STD_ROWS8[outputRows[i]] - 19), module, STRUCT_NAME::CV_LIGHTS + (i * 2)));
		}
		
		addOutput(createOutputCentered<CountModulaJack>(Vec(customColumns[STD_COL3], STD_HALF_ROWS8(STD_ROW2)), module, STRUCT_NAME::RANDOM_OUTPUT));
			
		// mode indicators
		addChild(createLightCentered<SmallLight<YellowLight>>(Vec(customColumns[STD_COL1]-20, STD_HALF_ROWS8(STD_ROW2)-8), module, STRUCT_NAME::ANALOGUE_LIGHT));
		addChild(createLightCentered<SmallLight<WhiteLight>>(Vec(customColumns[STD_COL1]-20, STD_HALF_ROWS8(STD_ROW2)+4), module, STRUCT_NAME::DIGITAL_LIGHT));
		
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	// operational mode menu
	struct OperationModeMenuItem : MenuItem {
		STRUCT_NAME *module;
	
		void onAction(const event::Action &e) override {
			module->digitalMode ^= true;
		}
	};

	// random range menu item
	struct RandomRangeMenuItem : MenuItem {
		STRUCT_NAME *module;
		int selectedRange;
		
		void onAction(const event::Action &e) override {
			module->randomRange = selectedRange;
		}
	};
	
	// random range menu 
	struct RandRangeMenu : MenuItem {
		STRUCT_NAME *module;
		std::string randLabels[STRUCT_NAME::NUM_RANDOM_RANGES] = {"0V to 10V", "-10V to 10V", "0V to 5V", "-5V to 5V"};

		Menu *createChildMenu() override {
			Menu *menu = new Menu;
			
			for (int i = 0;i < STRUCT_NAME::NUM_RANDOM_RANGES; i++) {
				RandomRangeMenuItem *randMenuItem = createMenuItem<RandomRangeMenuItem>(randLabels[i], CHECKMARK(module->randomRange == i));
				randMenuItem->module = module;
				randMenuItem->selectedRange = i;
				menu->addChild(randMenuItem);
			}
		
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
		menu->addChild(createMenuLabel("Settings"));
		
		// add the output mode menu
		OperationModeMenuItem *modeMenu = createMenuItem<OperationModeMenuItem>("Digital mode", CHECKMARK(module->digitalMode));
		modeMenu->module = module;
		menu->addChild(modeMenu);

		// internal random range selection - valid only for analogue operation
		if (!module->digitalMode) {
			RandRangeMenu *randMenu = createMenuItem<RandRangeMenu>("Internal random range");
			randMenu->module = module;
			menu->addChild(randMenu);
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