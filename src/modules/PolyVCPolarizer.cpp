//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Voltage Controlled Polarizer Module
//	AA 2 channel voltage controlled signal polarizer
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Polarizer.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME PolyVCPolarizer
#define PANEL_FILE "PolyVCPolarizer.svg"

struct PolyVCPolarizer : Module {
	enum ParamIds {
		CVAMOUNT_PARAM,
		MANUAL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		SIGNAL_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		SIGNAL_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(STATUS_LIGHT, 32),
		NUM_LIGHTS
	};

	Polarizer polarizer;
	bool isConnected;
	int count = 0;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	PolyVCPolarizer() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(CVAMOUNT_PARAM, 0.0f, 1.0f, 0.0f, "CV Amount", " %", 0.0f, 100.0f, 0.0f);
		configParam(MANUAL_PARAM, -2.0f, 2.0f, 0.0f, "Manual Amount");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
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
	
	void onReset() override {
		polarizer.reset();
		
		for (int c = 0; c < 16; c++) {
			lights[STATUS_LIGHT + (c * 2)].setBrightness(0.0f);
			lights[STATUS_LIGHT + (c * 2) + 1].setBrightness(0.0f);
		}
	}	
	
	void process(const ProcessArgs &args) override {

		if (inputs[SIGNAL_INPUT].isConnected()) {
			float manual = params[MANUAL_PARAM].getValue();
			float cvAmount = params[CVAMOUNT_PARAM].getValue();
			
			int n = inputs[SIGNAL_INPUT].getChannels();

			outputs[SIGNAL_OUTPUT].setChannels(n);
			
			for (int c = 0; c < 16; c++) {
				float cv = inputs[CV_INPUT].getPolyVoltage(c);
				polarizer.process(inputs[SIGNAL_INPUT].getPolyVoltage(c), manual, cv, cvAmount);
		
				if (count == 0) {
					lights[STATUS_LIGHT + (c * 2)].setBrightness(polarizer.negativeLevel);
					lights[STATUS_LIGHT + (c * 2) + 1].setBrightness(polarizer.positiveLevel);
				}
		
				if (c < n)
					outputs[SIGNAL_OUTPUT].setVoltage(polarizer.out, c);
			}
		
		}
		else {
			outputs[SIGNAL_OUTPUT].channels = 0;

			if (count == 0) {
				for (int c = 0; c < 16; c++) {
					lights[STATUS_LIGHT + (c * 2)].setBrightness(0.0f);
					lights[STATUS_LIGHT + (c * 2) + 1].setBrightness(0.0f);
				}
			}
		}
		
		if (++count > 3)
			count = 0;
	}
};

struct PolyVCPolarizerWidget : ModuleWidget {
	PolyVCPolarizerWidget(PolyVCPolarizer *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PolyVCPolarizer.svg")));

		// screws
		#include "../components/stdScrews.hpp"	
	
		// knobs
		addParam(createParamCentered<CountModulaKnobYellow>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 15, STD_ROWS7[STD_ROW3]), module, PolyVCPolarizer::CVAMOUNT_PARAM));
		addParam(createParamCentered<CountModulaKnobYellow>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 15, STD_ROWS6[STD_ROW4] - 10), module, PolyVCPolarizer::MANUAL_PARAM));
		
		// clock and reset input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 15, STD_ROWS6[STD_ROW1]), module, PolyVCPolarizer::SIGNAL_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 15, STD_ROWS7[STD_ROW2]), module, PolyVCPolarizer::CV_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 15, STD_ROWS6[STD_ROW6]), module, PolyVCPolarizer::SIGNAL_OUTPUT));

		// led matrix
		int x = 0, y = 0;
		int startPos = STD_ROWS6[STD_ROW5] - 15;
		for (int s = 0; s < 16; s++) {
			if (x > 3) {
				x = 0;
				y++;
			}
			
			addChild(createLightCentered<SmallLight<CountModulaLightRG>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + (10 * x++), startPos + (10 * y)), module, PolyVCPolarizer::STATUS_LIGHT + (s * 2)));
		}		
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		PolyVCPolarizer *module = dynamic_cast<PolyVCPolarizer*>(this->module);
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

Model *modelPolyVCPolarizer = createModel<PolyVCPolarizer, PolyVCPolarizerWidget>("PolyVCPolarizer");
