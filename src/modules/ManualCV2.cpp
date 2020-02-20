//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Manual CV Module
//	A dual manual CV generator
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME ManualCV2
#define PANEL_FILE "ManualCV2.svg"
#define UNIPOLAR 0
#define BIPOLAR 1

struct ManualCV2 : Module {
	enum ParamIds {
		CV_PARAM,
		RANGE_PARAM,
		POLARITY_PARAM,
		CHANNELS_PARAM,
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
	int polarity = UNIPOLAR;
	int prevPolarity = UNIPOLAR;
	float prevRange = 10.0f;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	ManualCV2() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(CV_PARAM, 0.0f, 10.0f, 0.0f, "Output value", " V");
		configParam(RANGE_PARAM, 1.0f, 10.0f, 10.0f, "Output range", " V");
		configParam(POLARITY_PARAM, 0.0f, 1.0f, 0.0f, "Output polarity");
		configParam(CHANNELS_PARAM, 1.0f, 16.0f, 1.0f, "Number of channels");
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	void onReset() override {
		polarity = prevPolarity = UNIPOLAR;
		paramQuantities[ManualCV2::CV_PARAM]->minValue = 0.0f;
		paramQuantities[ManualCV2::CV_PARAM]->displayMultiplier = 1.0f;
	}		
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("1.0"));

		json_object_set_new(root, "polarity", json_integer(polarity));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}
	
	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"

		json_t *pol = json_object_get(root, "polarity");

		if (pol) {
			prevPolarity = json_integer_value(pol);

			// adjust the min value based on the chosen polarity mode
			paramQuantities[ManualCV2::CV_PARAM]->minValue = (prevPolarity == UNIPOLAR ? 0.0f : -10.0f);
		}
	}			
	
	void process(const ProcessArgs &args) override {
	
		float cv = params[CV_PARAM].getValue();

		float range = params[RANGE_PARAM].getValue();
		
		// make the values on the knob reflect the selected range
		if (range != prevRange) {
			paramQuantities[ManualCV2::CV_PARAM]->displayMultiplier = range/10.0f;
			prevRange = range;
		}
		
		polarity = params[POLARITY_PARAM].getValue() > 0.5f ? BIPOLAR : UNIPOLAR;
		
		if (prevPolarity != polarity) {
			// adjust the min value based on the chosen polarity mode
			paramQuantities[ManualCV2::CV_PARAM]->minValue = (polarity == UNIPOLAR ? 0.0f : -10.0f);

			// on change of polarity, adjust the control value so it stays in the same position
			switch (polarity) {
				case BIPOLAR:
					params[CV_PARAM].setValue((cv/10.0f * 20.0f) - 10.0f);
					break;
				case UNIPOLAR:
					params[CV_PARAM].setValue((cv + 10.0f) / 2.0f);
					break;
			}
		}

		prevPolarity = polarity;
		
		outVal = cv * range / 10.0f;
		
		int numChans = clamp((int)(params[CHANNELS_PARAM].getValue()), 1, 16);
		
		outputs[CV1_OUTPUT].setChannels(numChans);
		outputs[CV2_OUTPUT].setChannels(numChans);
		
		for (int c =- 0; c < numChans; c++) {
			outputs[CV1_OUTPUT].setVoltage(outVal, c);
			outputs[CV2_OUTPUT].setVoltage(-outVal, c);
		}
	}
};

struct ManualCV2Widget : ModuleWidget {
	ManualCV2Widget(ManualCV2 *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ManualCV2.svg")));

		// screws
		#include "../components/stdScrews.hpp"	
		
		// knobs
		addParam(createParamCentered<CountModulaKnobMegaGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_HALF_ROWS7(STD_ROW6)), module, ManualCV2::CV_PARAM));

		addParam(createParamCentered<CountModulaRotarySwitchBlue>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW2]), module, ManualCV2::RANGE_PARAM));		
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS5[STD_ROW2]), module, ManualCV2::POLARITY_PARAM));
		
		addParam(createParamCentered<CountModulaRotarySwitchOrange>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS8[STD_ROW5]), module, ManualCV2::CHANNELS_PARAM));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, ManualCV2::CV1_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1]), module, ManualCV2::CV2_OUTPUT));	
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		ManualCV2 *module = dynamic_cast<ManualCV2*>(this->module);
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

Model *modelManualCV2 = createModel<ManualCV2, ManualCV2Widget>("ManualCV2");
