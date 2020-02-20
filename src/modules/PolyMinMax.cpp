//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - PolyMinMax Module
//	Polyphonic Min/Max module
//  Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME PolyMinMax
#define PANEL_FILE "PolyMinMax.svg"

struct PolyMinMax : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		SIGNAL_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		MIN_OUTPUT,
		MEAN_OUTPUT,
		MAX_OUTPUT,
		ASC_OUTPUT,
		DESC_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	float voltages[PORT_MAX_CHANNELS] = {};
	float vSum;

	int numChannels;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	PolyMinMax() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("1.1"));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}

	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}	
	
	void process(const ProcessArgs &args) override {
		
		if (inputs[SIGNAL_INPUT].isConnected()) {		

			// how many channels are we dealing with?
			numChannels = inputs[SIGNAL_INPUT].getChannels();
			outputs[ASC_OUTPUT].setChannels(numChannels);
			outputs[DESC_OUTPUT].setChannels(numChannels);

			// grab the input voltages and calculate the mean
			vSum = 0;		
			for (int c = 0; c < PORT_MAX_CHANNELS; c++) {
				if (c < numChannels) {
					voltages[c] = inputs[SIGNAL_INPUT].getPolyVoltage(c);
					vSum += voltages[c];
				}
				else
					voltages[c] = 999999999.0f;
			}
			
			// sort into ascending order
			std::sort(voltages, voltages + PORT_MAX_CHANNELS);
			
			// sorted outputs
			if(outputs[ASC_OUTPUT].isConnected() || outputs[DESC_OUTPUT].isConnected()) {
				for (int c = 0; c < numChannels; c++) {
					outputs[ASC_OUTPUT].setVoltage(voltages[c], c);
					outputs[DESC_OUTPUT].setVoltage(voltages[c], numChannels - c - 1);
				}
			}
			
			// min, mean and max outputs
			outputs[MIN_OUTPUT].setVoltage(voltages[0]);
			outputs[MEAN_OUTPUT].setVoltage(vSum/(float)numChannels);
			outputs[MAX_OUTPUT].setVoltage(voltages[numChannels - 1]);
		}
		else {
			outputs[MIN_OUTPUT].setVoltage(0.0f);
			outputs[MEAN_OUTPUT].setVoltage(0.0f);
			outputs[MAX_OUTPUT].setVoltage(0.0f);
			outputs[ASC_OUTPUT].channels = 0;
			outputs[DESC_OUTPUT].channels = 0;
		}
	}
};

struct PolyMinMaxWidget : ModuleWidget {
	PolyMinMaxWidget(PolyMinMax *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PolyMinMax.svg")));

		// screws
		#include "../components/stdScrews.hpp"	

		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, PolyMinMax::SIGNAL_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, PolyMinMax::MIN_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, PolyMinMax::MEAN_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, PolyMinMax::MAX_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, PolyMinMax::ASC_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, PolyMinMax::DESC_OUTPUT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		PolyMinMax *module = dynamic_cast<PolyMinMax*>(this->module);
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

Model *modelPolyMinMax = createModel<PolyMinMax, PolyMinMaxWidget>("PolyMinMax");
