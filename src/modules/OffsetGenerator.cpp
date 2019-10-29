//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Offset Generator
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME OffsetGenerator
#define PANEL_FILE "OffsetGenerator.svg"

struct OffsetGenerator : Module {
	enum ParamIds {
		COARSE_PARAM,
		FINE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		COARSE_INPUT,
		RESET_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		MIX_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	OffsetGenerator() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	
		configParam(COARSE_PARAM, -8.0f, 8.0f, 0.0f, "Coarse", " V");
		configParam(FINE_PARAM, -1.0f, 1.0f, 0.0f, "Fine", " V");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	void onReset() override {
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("1.0"));
			
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}

	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}
	
	void process(const ProcessArgs &args) override {

		// grab the coarse offset
		float offset = 0.0f;
		if (inputs[COARSE_INPUT].isConnected())
			offset = floor(inputs[COARSE_INPUT].getVoltage());
		else
			offset = params[COARSE_PARAM].getValue();
		
		offset = offset + params[FINE_PARAM].getValue();
		
		float cv = clamp(inputs[CV_INPUT].getVoltage() + offset, -12.0f, 12.0f);
		outputs[MIX_OUTPUT].setVoltage(cv);
	}
};

struct OffsetGeneratorWidget : ModuleWidget {
	OffsetGeneratorWidget(OffsetGenerator *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/OffsetGenerator.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		// knobs
		addParam(createParamCentered<CountModulaRotarySwitchBlue>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW2]), module, OffsetGenerator::COARSE_PARAM));
		addParam(createParamCentered<CountModulaKnobBlue>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW3]), module, OffsetGenerator::FINE_PARAM));
		
		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW1]), module, OffsetGenerator::COARSE_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW4]), module, OffsetGenerator::CV_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW5]), module, OffsetGenerator::MIX_OUTPUT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		OffsetGenerator *module = dynamic_cast<OffsetGenerator*>(this->module);
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

Model *modelOffsetGenerator = createModel<OffsetGenerator, OffsetGeneratorWidget>("OffsetGenerator");
