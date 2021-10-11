//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Mixer Module
//	A 4 input mixer with switchable uni/bi polar mixing capabilities
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/MixerEngine.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Mixer
#define PANEL_FILE "Mixer.svg"

struct Mixer : Module {
	enum ParamIds {
		R1_LEVEL_PARAM,
		R2_LEVEL_PARAM,
		R3_LEVEL_PARAM,
		R4_LEVEL_PARAM,
		MODE_PARAM,
		LEVEL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		R1_INPUT,
		R2_INPUT,
		R3_INPUT,
		R4_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		MIX_OUTPUT,
		XIM_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		OVERLOAD_LIGHT,
		NUM_LIGHTS
	};

	MixerEngine mixer;

	bool bipolar;
	bool prevBipolar;
	float mixLevels[4] = {};
	float outputLevel = 0.0f;
	
	int processCount = 8;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"	
	
	Mixer() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// mix knobs
		char c ='A';
		std::string s;
		for (int i = 0; i < 4; i++) {
			s = c++;
			configParam(R1_LEVEL_PARAM + i, 0.0f, 1.0f, 0.5f, "Level " + s, " %", 0.0f, 100.0f, 0.0f);
			configInput(R1_INPUT + i, s);
		}

		// output level 
		configParam(LEVEL_PARAM, 0.0f, 1.0f, 0.0f, "Output level", " %", 0.0f, 100.0f, 0.0f);
		
		// switches
		configSwitch(MODE_PARAM, 0.0f, 1.0f, 0.0f, "Mode", {"Unipolar", "Bipolar"});
		
		configOutput(MIX_OUTPUT, "Mix");
		configOutput(XIM_OUTPUT, "Inverted mix");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	void onReset() override {
		processCount = 8;
	}	
	
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
	
	void process(const ProcessArgs &args) override {
		
		if (++processCount > 8) {
			processCount = 0;
			
			mixLevels[0] = params[R1_LEVEL_PARAM].getValue();
			mixLevels[1] = params[R2_LEVEL_PARAM].getValue();
			mixLevels[2] = params[R3_LEVEL_PARAM].getValue();
			mixLevels[3] = params[R4_LEVEL_PARAM].getValue();
			outputLevel = params[LEVEL_PARAM].getValue();
					
			bipolar = params[MODE_PARAM].getValue() > 0.5f;
		}
		
		float out = mixer.process(inputs[R1_INPUT].getNormalVoltage(10.0f), inputs[R2_INPUT].getVoltage(), inputs[R3_INPUT].getVoltage(), inputs[R4_INPUT].getVoltage(), 
			mixLevels[0],mixLevels[1], mixLevels[2], mixLevels[3], 
			outputLevel, bipolar);
			
		outputs[MIX_OUTPUT].setVoltage(out);
		outputs[XIM_OUTPUT].setVoltage(-out);
		
		if (processCount == 0)
			lights[OVERLOAD_LIGHT].setSmoothBrightness(mixer.overloadLevel, args.sampleTime * 4);
	}
};

struct MixerWidget : ModuleWidget {

	std::string panelName;
	
	MixerWidget(Mixer *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	
		
		for (int j = 0; j < 4; j++) {
			addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1 + j]), module, Mixer::R1_LEVEL_PARAM + j));
			
			// inputs
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1 + j]), module, Mixer::R1_INPUT + j));
		}
			
		// level knobs
		addParam(createParamCentered<Potentiometer<WhiteKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW5]), module, Mixer::LEVEL_PARAM));
		

		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, Mixer::MIX_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, Mixer::XIM_OUTPUT));

		// switches
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW6]), module, Mixer::MODE_PARAM));

		// lights
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS6[STD_ROW5]), module, Mixer::OVERLOAD_LIGHT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		Mixer *module = dynamic_cast<Mixer*>(this->module);
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

Model *modelMixer = createModel<Mixer, MixerWidget>("Mixer");
