//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Multi-tapped Gate Delay Module
//	A shift register style gate delay offering 8 tapped gate outputs 
//	Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include <queue>
#include <deque>

#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/GateDelayLine.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME GateDelayMT
#define PANEL_FILE "GateDelayMT.svg"

struct GateDelayMT : Module {
	enum ParamIds {
		TIME_PARAM,
		CVLEVEL_PARAM,
		RANGE_PARAM,
		MIXDIR_PARAM,
		ENUMS(MIXDEL_PARAMS, 8),
		NUM_PARAMS
	};
	enum InputIds {
		TIME_INPUT,
		GATE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		DIRECT_OUTPUT,
		ENUMS(DELAYED_OUTPUTS, 8),
		MIX_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		DIRECT_LIGHT,
		ENUMS(DELAYED_LIGHTS, 8),
		MIX_LIGHT,
		MIXDIR_PARAM_LIGHT,
		ENUMS(MIXDEL_PARAM_LIGHTS, 8),
		NUM_LIGHTS
	}; 

	GateDelayLine delayLine;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	GateDelayMT() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		// parameters 
		configParam(CVLEVEL_PARAM, -5.0f, 5.0f, 0.0f, "Delay time CV amount", " %", 0.0f, 100.0f, 0.0f);
		configParam(TIME_PARAM, 0.0f, 10.0f, 5.0f, "Delay time");
		configSwitch(RANGE_PARAM, 0.0f, 2.0f, 2.0f,  "Time range", {"40", "20", "10"});
		
		// direct output
		configSwitch(MIXDIR_PARAM, 0.0f, 1.0f, 1.0f, "Direct Mix", {"Excluded","Included"});

		configInput(TIME_INPUT, "Time CV");
		configInput(GATE_INPUT, "Gate");

		configOutput(DIRECT_OUTPUT, "Direct");
		configOutput(MIX_OUTPUT, "Mixed");

		// tapped outputs
		int k = 0;
		std::string tapName;
		for (int i = 0; i < 5; i += 4) {
			for (int j = 0; j < 4; j++) {
				tapName = "Tap " + std::to_string(k + 1);
				configSwitch(MIXDEL_PARAMS + k, 0.0f, 1.0f, 1.0f, "Tap " + std::to_string(k + 1) + " mix", {"Excluded","Included"});
				configOutput(DELAYED_OUTPUTS + k, tapName),

				k++;
			}
		}

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	int range = 0;
	
	int taps[3][8] = {
		{ 4,  8, 12, 16, 20, 24, 28, 32}, 
		{ 2,  4,  6,  8, 10, 12, 14, 16},
		{ 1,  2,  3,  4,  5,  6,  7,  8}
	};

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(1));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}

	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}	
	
	void onReset() override {
		delayLine.reset();
	}
	
	void process(const ProcessArgs &args) override {

		// determine the range option 
		int t = (int)(params[RANGE_PARAM].value);
		if (range != t) {
#ifdef NO_TIME_TRAVEL		
			// range has changed - clear out the delay line otherwise we can get some undesirable time travel artefacts
			delayLine.reset();
#endif
			range = t;
		}

		// compute delay time in seconds
		float delay = params[TIME_PARAM].getValue();
		if (inputs[TIME_INPUT].getVoltage())
			 delay += (inputs[TIME_INPUT].getVoltage() * params[CVLEVEL_PARAM].getValue());

		 // process the delay and grab the input gate level
		delayLine.process(inputs[GATE_INPUT].getVoltage(), delay);

		float mix = 0.0f;
		
		// direct output
		outputs[DIRECT_OUTPUT].setVoltage(boolToGate(delayLine.gateValue()));
		lights[DIRECT_LIGHT].setBrightness(outputs[DIRECT_OUTPUT].getVoltage() / 10.0f);
		
		// mix direct in if we want it
		mix += (params[MIXDIR_PARAM].getValue() * outputs[DIRECT_OUTPUT].getVoltage());
		
		for (int i = 0; i < 8; i++) {
			outputs[DELAYED_OUTPUTS + i].setVoltage(boolToGate(delayLine.tapValue(taps[range][i])));
			lights[DELAYED_LIGHTS + i].setBrightness(outputs[DELAYED_OUTPUTS + i].getVoltage() / 10.0f);
			
			// add this tap to the mix
			mix += (params[MIXDEL_PARAMS + i].getValue() * outputs[DELAYED_OUTPUTS + i].getVoltage());
		}
		
		// finally output the mix 
		outputs[MIX_OUTPUT].setVoltage(boolToGate(mix > 0.1f));
		lights[MIX_LIGHT].setBrightness(boolToLight(mix > 0.1f));
	}
};


struct GateDelayMTWidget : ModuleWidget { 

	std::string panelName;
	
	GateDelayMTWidget(GateDelayMT *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"

		// screws
		#include "../components/stdScrews.hpp"	
		
		// parameters 
		addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW1]), module, GateDelayMT::CVLEVEL_PARAM));
		addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW1]), module, GateDelayMT::TIME_PARAM));
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW2]), module, GateDelayMT::RANGE_PARAM));
		
		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1]), module, GateDelayMT::TIME_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, GateDelayMT::GATE_INPUT));

		// direct output
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, GateDelayMT::DIRECT_OUTPUT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS6[STD_ROW2]), module, GateDelayMT::DIRECT_LIGHT));
		addParam(createParamCentered<CountModulaLEDPushButton<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, GateDelayMT::MIXDIR_PARAM, GateDelayMT::MIXDIR_PARAM_LIGHT));
			
		// mix output
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW2]), module, GateDelayMT::MIX_OUTPUT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_ROWS6[STD_ROW2]), module, GateDelayMT::MIX_LIGHT));
		
		// tapped outputs
		int k = 0;
		for (int i = 0; i < 5; i += 4) {
			for (int j = 0; j < 4; j++) {
				addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3 + i], STD_ROWS6[STD_ROW3 + j]), module, GateDelayMT::DELAYED_OUTPUTS + k));
				addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2 + i], STD_ROWS6[STD_ROW3 + j]), module, GateDelayMT::DELAYED_LIGHTS + k));
				addParam(createParamCentered<CountModulaLEDPushButton<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL1 + i], STD_ROWS6[STD_ROW3 + j]), module, GateDelayMT::MIXDEL_PARAMS + k, GateDelayMT::MIXDEL_PARAM_LIGHTS + k));
			
				k++;
			}
		}
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		GateDelayMT *module = dynamic_cast<GateDelayMT*>(this->module);
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

Model *modelGateDelayMT = createModel<GateDelayMT, GateDelayMTWidget>("GateDelayMT");