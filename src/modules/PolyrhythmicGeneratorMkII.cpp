//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Polyrhythmic Generator Module Mk II
//	Generates polyrhythmns 
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/FrequencyDivider.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME PolyrhythmicGeneratorMkII
#define PANEL_FILE "PolyrhythmicGeneratorMkII.svg"

struct PolyrhythmicGeneratorMkII : Module {
	enum ParamIds {
		ENUMS(CV_PARAM, 8),
		ENUMS(DIV_PARAM, 8),
		ENUMS(MUTE_PARAM, 8),
		MUTEALL_PARAM,
		BEATMODE_PARAM,
		OUTPUTMODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(CLOCK_INPUT, 8),
		ENUMS(RESET_INPUT, 8),
		ENUMS(CV_INPUT, 8),
		MUTEALL_INPUT,
		BEATMODE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(TRIG_OUTPUT, 8),
		POLY_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(TRIG_LIGHT, 8),
		ENUMS(MUTE_PARAM_LIGHT, 8),
		MUTEALL_PARAM_LIGHT,
		BEATMODE_PARAM_LIGHT,
		NUM_LIGHTS
	};

	FrequencyDivider dividers[8];
	FrequencyDividerOld legacyDividers[8];
	
	float legacyCVMap[16] = {	0.333f,
								1.0f,
								1.667f,
								2.333f,
								3.0f,
								3.667f,
								4.333f,
								5.0f,
								5.667f,
								6.333f,
								6.999f,
								7.665f,
								8.331f,
								8.997f,
								9.663f,
								10.329f };
	
	dsp::PulseGenerator pgTriggers[8];
	GateProcessor gpResets[8];
	GateProcessor gpClocks[8];
	bool clockOut[8] = {};
	
	int prevCountMode = COUNT_DN;
	bool isStarting = false;
	bool legacyMode = false;
	
	GateProcessor gpMuteAll;
	GateProcessor gpBeatMode;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	PolyrhythmicGeneratorMkII() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// division channels
		for (int i = 0; i < 8; i++) {
			// knobs
			configParam(CV_PARAM + i, -1.0f, 1.0f, 0.0f, "Division  CV amount", " %", 0.0f, 100.0f, 0.0f);
			configParam(DIV_PARAM + i, 1.0f, 16.0f, 1.0f, "Division");
			
			// buttons
			configParam(MUTE_PARAM + i, 0.0f, 1.0f, 0.0f, "Mute channel");
		}
		
		// global stuff
		configParam(OUTPUTMODE_PARAM, 0.0f, 3.0f, 0.0f, "Output mode");
		configParam(BEATMODE_PARAM, 0.0f, 1.0f, 1.0f, "Beat mode");
		configParam(MUTEALL_PARAM, 0.0f, 1.0f, 0.0f, "Global mute");
		
	// set the theme from the current default value
	#include "../themes/setDefaultTheme.hpp"
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(2));
		json_object_set_new(root, "legacyMode", json_boolean(legacyMode));
		
		json_t *countMode = json_array();
		json_t *n = json_array();
		
		for (int i = 0; i < 8; i++) {
			json_array_insert_new(countMode, i, json_integer(dividers[i].countMode));
			json_array_insert_new(n, i, json_integer(dividers[i].N));
		}
		
		json_object_set_new(root, "divCountMode", countMode);
		json_object_set_new(root, "divN", n);
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}

	void dataFromJson(json_t *root) override {

		json_t *legacy = json_object_get(root, "legacyMode");
		if (legacy)
			legacyMode = json_boolean_value(legacy);
		
		json_t *countMode = json_object_get(root, "divCountMode");
		json_t *n = json_object_get(root, "divN");
		
		for (int i = 0; i < 8; i++) {
			// need to start from the beginning as the positioning of the clock edges cannot be guaranteed so the results can be a little random
			dividers[i].count = -1;
			dividers[i].phase = false;
	
			if (countMode) {
				json_t *v = json_array_get(countMode, i);
				if (v)
					dividers[i].countMode = json_integer_value(v);
			}
	
			if (n) {
				json_t *v = json_array_get(n, i);
				if (v)
					dividers[i].N = json_integer_value(v);
			}
		}
		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"		
	}
	
	void onReset() override {
		for (int i = 0; i < 8; i++) {
			dividers[i].reset();
			legacyDividers[i].reset();
			pgTriggers[i].reset();
			gpResets[i].reset();
			gpClocks[i].reset();
			clockOut[i] = false;
		}
	}
		
	void process(const ProcessArgs &args) override {
		if (legacyMode)
			legacyProcess(args);
		else
			newProcess(args);
	}
	
	void newProcess(const ProcessArgs &args) {

		// determine global control values
		gpMuteAll.set(inputs[MUTEALL_INPUT].getVoltage());
		gpBeatMode.set(inputs[BEATMODE_INPUT].getVoltage());
		int countMode =  (gpBeatMode.high() || params[BEATMODE_PARAM].getValue() > 0.5f)? COUNT_DN : COUNT_UP;
		bool muteAll = gpMuteAll.high() || params[MUTEALL_PARAM].getValue() > 0.5f;
		
		float prevClock = 0.0f;
		bool prevPhase;
		float prevReset = 0.0f;
		float prevCV = 0.0f;
		
		outputs[POLY_OUTPUT].setChannels(8);
		
		for (int i = 0; i < 8; i++) {
			// handle the reset input - also reset on change of beat mode
			float res = inputs[RESET_INPUT + i].getNormalVoltage(prevReset);
			gpResets[i].set(res);
			if(gpResets[i].leadingEdge() || prevCountMode != countMode || isStarting) {
				dividers[i].reset();
				pgTriggers[i].reset();
				isStarting = false;
			}

			// counter mode - always down for legacy mode as it made no real difference
			dividers[i].setCountMode(countMode);
			
			// save for rising edge determination
			prevPhase = dividers[i].phase;
			
			// calculate the current division value and set the division number
			float divCV = inputs[CV_INPUT + i].getNormalVoltage(prevCV);
			int div = (int)(params[DIV_PARAM + i].getValue() + (divCV * params[CV_PARAM + i].getValue() * 1.6)); // scale 10V CV up to 16
			
			// set the division amount - take into account the legacy mode (the old version was halving the clock so would only div by 2/4/6/8/10 etc)
			dividers[i].setN(div);
			
			// clock the divider 
			float clock = inputs[CLOCK_INPUT + i].getNormalVoltage(prevClock);
			dividers[i].process(clock);
			gpClocks[i].set(clock);
			
			// fire off the trigger on a 0 to 1 rising edge
			if (!prevPhase &&  dividers[i].phase) {
				clockOut[i] = true;
				pgTriggers[i].trigger(1e-3f);
			}
			
			// determine the output value
			bool trigOut = false;
			if (!muteAll && params[MUTE_PARAM + i].getValue() < 0.5f ) {
				switch ((int)(params[OUTPUTMODE_PARAM].getValue())) {
					case 0: // trigger
						trigOut = pgTriggers[i].remaining > 0.0f;
						pgTriggers[i].process(args.sampleTime);
						break;
					case 1: // gate
						trigOut = dividers[i].phase;
						break;
					case 2: // gated clock
						trigOut = gpClocks[i].high() &&  dividers[i].phase;
						break;
					case 3:	// clock
						if (dividers[i].N == 1)
							trigOut = dividers[i].phase;
						else {
							trigOut = clockOut[i];
							if (trigOut && prevPhase &&  dividers[i].phase && gpClocks[i].anyEdge())
								clockOut[i] = trigOut = false;
						}
						break;
				}
			}
			
			// output the trigger value to both the poly out and the individual channel then set the light
			outputs[POLY_OUTPUT].setVoltage(boolToGate(trigOut), i);
			outputs[TRIG_OUTPUT + i].setVoltage(boolToGate(trigOut));
			lights[TRIG_LIGHT + i].setSmoothBrightness(boolToLight(trigOut), args.sampleTime);
		
			// save these values for normalling in the next iteration 
			prevClock = clock;
			prevReset = res;
			prevCV = divCV;
		}
		
		prevCountMode = countMode;
	}
	
	
	void legacyProcess(const ProcessArgs &args) {

		// determine global control values
		gpMuteAll.set(inputs[MUTEALL_INPUT].getVoltage());
		gpBeatMode.set(inputs[BEATMODE_INPUT].getVoltage());
		int countMode =  (gpBeatMode.high() || params[BEATMODE_PARAM].getValue() > 0.5f)? COUNT_DN : COUNT_UP;
		bool muteAll = gpMuteAll.high() || params[MUTEALL_PARAM].getValue() > 0.5f;
		
		float prevClock = 0.0f;
		bool prevPhase;
		float prevReset = 0.0f;
		float prevCV = 0.0f;
		
		outputs[POLY_OUTPUT].setChannels(8);

		for (int i = 0; i < 8; i++) {
			// set the required maximum division in the divider
			legacyDividers[i].setMaxN(15);
			legacyDividers[i].setCountMode(countMode);
			
			// handle the reset input
			float res = inputs[RESET_INPUT + i].getNormalVoltage(prevReset);
			gpResets[i].set(res);
			if(gpResets[i].leadingEdge()) {
				legacyDividers[i].reset();
				pgTriggers[i].reset();
			}
			
			// save for rising edge determination
			prevPhase = legacyDividers[i].phase;
			
			// calculate the current division value and set the division number
			float cv = inputs[CV_INPUT + i].getNormalVoltage(prevCV);
			float div = legacyCVMap[clamp((int)(params[DIV_PARAM + i].getValue()) - 1, 0, 15)];

			float x = div + (params[CV_PARAM + i].getValue() * cv);
			legacyDividers[i].setN(x);

			// clock the divider 
			float clock = inputs[CLOCK_INPUT + i].getNormalVoltage(prevClock);
			legacyDividers[i].process(clock);
			gpClocks[i].set(clock);
			
			// fire off the trigger on a 0 to 1 rising edge
			if (!prevPhase &&  legacyDividers[i].phase) {
				pgTriggers[i].trigger(1e-3f);
			}
			
			// determine the output value
			bool trigOut = false;
			if (!muteAll && params[MUTE_PARAM + i].getValue() < 0.5f ) {
				switch ((int)(params[OUTPUTMODE_PARAM].getValue())) {
					case 0: // trigger
						trigOut = pgTriggers[i].remaining > 0.0f;
						pgTriggers[i].process(args.sampleTime);
						break;
					case 1: // gate
						trigOut = legacyDividers[i].phase;
						break;
					case 2: // gated clock
						trigOut = gpClocks[i].high() &&  legacyDividers[i].phase;
						break;
					case 3:	// clock
						trigOut = pgTriggers[i].process(args.sampleTime);
						if (gpClocks[i].low()) {
							trigOut = false;
							pgTriggers[i].reset();
						}
						else if (trigOut){
							// extend the pulse as long as the clock is high
							pgTriggers[i].trigger(1e-3f);
						}
						break;
				}
			}
			
			// output the trigger value to both the poly out and the individual channel then set the light
			outputs[POLY_OUTPUT].setVoltage(boolToGate(trigOut), i);
			outputs[TRIG_OUTPUT + i].setVoltage(boolToGate(trigOut));
			lights[TRIG_LIGHT + i].setSmoothBrightness(boolToLight(trigOut), args.sampleTime);
		
			// save these values for normalling in the next iteration 
			prevClock = clock;
			prevReset = res;
			prevCV = cv;
		}
	}	
};


struct PolyrhythmicGeneratorMkIIWidget : ModuleWidget {
	PolyrhythmicGeneratorMkIIWidget(PolyrhythmicGeneratorMkII *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PolyrhythmicGeneratorMkII.svg")));

		// screws
		#include "../components/stdScrews.hpp"	
		
		// division channels
		for (int i = 0; i < 8; i++) {
			// clock and reset input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGeneratorMkII::CLOCK_INPUT + i));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGeneratorMkII::RESET_INPUT + i));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGeneratorMkII::CV_INPUT + i));

			// knobs
			addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL9], STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGeneratorMkII::CV_PARAM + i));
			addParam(createParamCentered<CountModulaRotarySwitchWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL11], STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGeneratorMkII::DIV_PARAM + i));
			
			// buttons
			addParam(createParamCentered<CountModulaLEDPushButton<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL13]-10, STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGeneratorMkII::MUTE_PARAM + i, PolyrhythmicGeneratorMkII::MUTE_PARAM_LIGHT + i));
			
			// lights
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL14], STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGeneratorMkII::TRIG_LIGHT + i));

			// outputs
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL15], STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGeneratorMkII::TRIG_OUTPUT + i));
		}
		
		// global stuff
		addParam(createParamCentered<CountModulaRotarySwitch5PosRed>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2]-15), module, PolyrhythmicGeneratorMkII::OUTPUTMODE_PARAM));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW4]-15), module, PolyrhythmicGeneratorMkII::BEATMODE_INPUT));
		addParam(createParamCentered<CountModulaLEDPushButton<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW5]-15), module, PolyrhythmicGeneratorMkII::BEATMODE_PARAM, PolyrhythmicGeneratorMkII::BEATMODE_PARAM_LIGHT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW7]), module, PolyrhythmicGeneratorMkII::MUTEALL_INPUT));
		addParam(createParamCentered<CountModulaLEDPushButton<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, PolyrhythmicGeneratorMkII::MUTEALL_PARAM, PolyrhythmicGeneratorMkII::MUTEALL_PARAM_LIGHT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW6]), module, PolyrhythmicGeneratorMkII::POLY_OUTPUT));
	}
	
	struct LegacyModeMenuItem : MenuItem {
		PolyrhythmicGeneratorMkII *module;
		void onAction(const event::Action &e) override {
			module->legacyMode = !module->legacyMode;
			module->onReset();
		}
	};	
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"	
	
	void appendContextMenu(Menu *menu) override {
		PolyrhythmicGeneratorMkII *module = dynamic_cast<PolyrhythmicGeneratorMkII*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"	
		
		// add legacy mode menu item
		LegacyModeMenuItem *legacyMenuItem = createMenuItem<LegacyModeMenuItem>("Legacy Mode", CHECKMARK(module->legacyMode));
		legacyMenuItem->module = module;
		menu->addChild(legacyMenuItem);	
	}	
	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}	
};

Model *modelPolyrhythmicGeneratorMkII = createModel<PolyrhythmicGeneratorMkII, PolyrhythmicGeneratorMkIIWidget>("PolyrhythmicGeneratorMkII");
