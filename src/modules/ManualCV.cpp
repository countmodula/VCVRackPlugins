//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Manual CV Module
//	A dual manual CV generator
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME ManualCV
#define PANEL_FILE "ManualCV.svg"

struct ManualCV : Module {
	enum ParamIds {
		CV1COARSE_PARAM,
		CV1FINE_PARAM,
		CV2COARSE_PARAM,
		CV2FINE_PARAM,
		MANUAL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		CV1_OUTPUT,
		CV2_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	float outVal;

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	ManualCV() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(CV1COARSE_PARAM, -10.0f, 10.0f, 0.0f, "Coarse value", " V");
		configParam(CV1FINE_PARAM, -0.5f, 0.5f, 0.0f, "Fine value", " V");
		configParam(CV2COARSE_PARAM, -10.0f, 10.0f, 0.0f, "Coarse value", " V");
		configParam(CV2FINE_PARAM, -0.5f, 0.5f, 0.0f, "Fine value", " V");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
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
		outputs[CV1_OUTPUT].setVoltage(clamp(params[CV1COARSE_PARAM].getValue() + params[CV1FINE_PARAM].getValue(), -10.0f, 10.0f));
		outputs[CV2_OUTPUT].setVoltage(clamp(params[CV2COARSE_PARAM].getValue() + params[CV2FINE_PARAM].getValue(), -10.0f, 10.0f));
	}
};

struct ManualCVWidget : ModuleWidget {

	std::string panelName;
	
	ManualCVWidget(ManualCV *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	
		
		// knobs
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, ManualCV::CV1COARSE_PARAM));
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, ManualCV::CV1FINE_PARAM));

		addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, ManualCV::CV2COARSE_PARAM));
		addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, ManualCV::CV2FINE_PARAM));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, ManualCV::CV1_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, ManualCV::CV2_OUTPUT));	
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		ManualCV *module = dynamic_cast<ManualCV*>(this->module);
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

Model *modelManualCV = createModel<ManualCV, ManualCVWidget>("ManualCV");
