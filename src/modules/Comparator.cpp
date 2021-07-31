//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Comparator Module
//	An analogue comparator
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Comparator
#define PANEL_FILE "Comparator.svg"

struct Comparator : Module {
	enum ParamIds {
		THRESHOLD_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		THRESHOLD_INPUT,
		COMPARE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		COMPARE_OUTPUT,
		INV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		OVER_LIGHT,
		UNDER_LIGHT,
		NUM_LIGHTS
	};

	bool state = 0;

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	Comparator() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(THRESHOLD_PARAM, -10.0, 10.0, 0.0, "Threshold", " V");
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"	
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(2));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}
	
	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}		
	
	void process(const ProcessArgs &args) override {

		// Compute the threshold from the threshold parameter and input
		float threshold = params[THRESHOLD_PARAM].getValue();
		threshold += inputs[THRESHOLD_INPUT].getVoltage();

		float compare = inputs[COMPARE_INPUT].getVoltage();
		
		// compare
		state = (compare > threshold);
		
		if (state) {
			// set the output high
			outputs[COMPARE_OUTPUT].setVoltage(10.0f);
			outputs[INV_OUTPUT].setVoltage(0.0f);

			// Set light off
			lights[OVER_LIGHT].setBrightness(1.0f);
			lights[UNDER_LIGHT].setBrightness(0.0f);
		}
		else {
			// set the output low
			outputs[COMPARE_OUTPUT].setVoltage(0.0f);
			outputs[INV_OUTPUT].setVoltage(10.0f);

			// Set light off
			lights[UNDER_LIGHT].setBrightness(1.0f);
			lights[OVER_LIGHT].setBrightness(0.0f);
		}
	}
};

struct ComparatorWidget : ModuleWidget {

	std::string panelName;
	
	ComparatorWidget(Comparator *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	

		addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(45, 75), module, Comparator::THRESHOLD_PARAM));

		addInput(createInputCentered<CountModulaJack>(Vec(45, 130), module, Comparator::THRESHOLD_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(45, 180), module, Comparator::COMPARE_INPUT));

		addOutput(createOutputCentered<CountModulaJack>(Vec(25, 260), module, Comparator::COMPARE_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(25, 307), module, Comparator::INV_OUTPUT));

		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(65, 260), module, Comparator::OVER_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(65, 307), module, Comparator::UNDER_LIGHT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"
	
	void appendContextMenu(Menu *menu) override {
		Comparator *module = dynamic_cast<Comparator*>(this->module);
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

Model *modelComparator = createModel<Comparator, ComparatorWidget>("Comparator");
