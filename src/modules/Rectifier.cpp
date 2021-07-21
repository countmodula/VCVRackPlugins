//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Rectifier Module
//	A full and half wave rectifier
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Inverter.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Rectifier
#define PANEL_FILE "Rectifier.svg"

struct Rectifier : Module {
	enum ParamIds {
		CV_PARAM,
		MANUAL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		SIGNAL_INPUT,
		CV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		FWR_OUTPUT,
		PHR_OUTPUT,
		NHR_OUTPUT,
		FWRI_OUTPUT,
		PHRI_OUTPUT,
		NHRI_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};


	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"	
	
	Rectifier() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(MANUAL_PARAM, -10.0f, 10.0f, 0.0f, "Rectification axis", " V");
		configParam(CV_PARAM, 0.0f, 1.0f, 0.0f, "CV amount", " %", 0.0f, 100.0f, 0.0f);

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

		// determine the rectification axis
		float axis = clamp(params[MANUAL_PARAM].getValue() + (params[CV_PARAM].getValue() * inputs[CV_INPUT].getNormalVoltage(0.0f)), -10.0f, 10.0f);
		
		// determine the channels we're working with
		int n = inputs[SIGNAL_INPUT].getChannels();
		outputs[FWR_OUTPUT].setChannels(n);
		outputs[PHR_OUTPUT].setChannels(n);
		outputs[NHR_OUTPUT].setChannels(n);
		outputs[FWRI_OUTPUT].setChannels(n);
		outputs[PHRI_OUTPUT].setChannels(n);
		outputs[NHRI_OUTPUT].setChannels(n);
		
		// now process each channel
		float v, posHalf, negHalf;
		for (int c = 0; c < n; c++) {
			v = inputs[SIGNAL_INPUT].getVoltage(c);
			posHalf = clamp(fmaxf(v, axis), -12.0f, 12.0f);
			negHalf = clamp(fminf(v, axis), -12.0f, 12.0f);
		
			outputs[PHR_OUTPUT].setVoltage(posHalf, c);
			outputs[NHR_OUTPUT].setVoltage(negHalf, c);
			outputs[FWR_OUTPUT].setVoltage(posHalf - negHalf, c);
			
			outputs[PHRI_OUTPUT].setVoltage(-posHalf, c);
			outputs[NHRI_OUTPUT].setVoltage(-negHalf, c);
			outputs[FWRI_OUTPUT].setVoltage(-(posHalf - negHalf), c);
		}
	}	
};

struct RectifierWidget : ModuleWidget {

	std::string panelName;
	
	RectifierWidget(Rectifier *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	
		
		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]),module, Rectifier::SIGNAL_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]),module, Rectifier::CV_INPUT));
		
		// knobs
		addParam(createParamCentered<Potentiometer<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1]), module, Rectifier::MANUAL_PARAM));
		addParam(createParamCentered<Potentiometer<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, Rectifier::CV_PARAM));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, Rectifier::FWR_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, Rectifier::PHR_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, Rectifier::NHR_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, Rectifier::FWRI_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, Rectifier::PHRI_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW5]), module, Rectifier::NHRI_OUTPUT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		Rectifier *module = dynamic_cast<Rectifier*>(this->module);
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

Model *modelRectifier = createModel<Rectifier, RectifierWidget>("Rectifier");
