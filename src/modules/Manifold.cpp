//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Voltage Controlled Polarizer Module
//	AA 2 channel voltage controlled signal polarizer
//	Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Polarizer.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Manifold
#define PANEL_FILE "Manifold.svg"

struct Manifold : Module {
	enum ParamIds {
		ENUMS(CHANNELS_PARAM, 2),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(SIGNAL_INPUT, 4),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(SIGNAL_OUTPUT, 4),
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	Manifold() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configParam(CHANNELS_PARAM, 1.0f, 16.0f, 1.0f, "Number of channels");
		configParam(CHANNELS_PARAM + 1, 1.0f, 16.0f, 1.0f, "Number of channels");
		
		std::string inputLabels[4] = {"A", "B", "C", "D"};
		for (int i = 0; i < 4; i++) {
			configInput(SIGNAL_INPUT + i, inputLabels[i]);
			configOutput(SIGNAL_OUTPUT + i, inputLabels[i]);
			configBypass(SIGNAL_INPUT + i, SIGNAL_OUTPUT + i);
		}
		
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
	
	void onReset() override {
	}	
	
	void process(const ProcessArgs &args) override {

		for (int i = 0; i < 2; i++) {
			int outChans = (int)(params[CHANNELS_PARAM + i].getValue());
			
			for (int j = 0; j < 2; j++) {
				// which input/output are we working with?
				int x = (i * 2) + j;
				
				if (inputs[SIGNAL_INPUT + x].isConnected()) {
					int inChans = inputs[SIGNAL_INPUT + x].getChannels();

					outputs[SIGNAL_OUTPUT + x].setChannels(outChans);
					
					int k = 0; 
					for (int c = 0; c < outChans; c++) {
						outputs[SIGNAL_OUTPUT + x].setVoltage(inputs[SIGNAL_INPUT + x].getPolyVoltage(k++), c);
						
						// loop the input channels around so they replicate in order
						if (k >= inChans)
							k = 0;
					}
				}
				else
					outputs[SIGNAL_INPUT + x].channels = 0;
			}
		}
	}
};

struct ManifoldWidget : ModuleWidget {

	std::string panelName;
	
	ManifoldWidget(Manifold *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"

		// screws
		#include "../components/stdScrews.hpp"	

		// knobs
		addParam(createParamCentered<RotarySwitch<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 15, STD_HALF_ROWS8(STD_ROW3)), module, Manifold::CHANNELS_PARAM));
		addParam(createParamCentered<RotarySwitch<WhiteKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 15, STD_HALF_ROWS8(STD_ROW7)), module, Manifold::CHANNELS_PARAM + 1));

		int rows[4] = {STD_ROWS8[STD_ROW1], STD_ROWS8[STD_ROW2], STD_ROWS8[STD_ROW5], STD_ROWS8[STD_ROW6]};
		for (int i=0; i < 4; i ++) {
			// inputs
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 10, rows[i]), module, Manifold::SIGNAL_INPUT + i));

			// outputs
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 10, rows[i]), module, Manifold::SIGNAL_OUTPUT + i));
		}
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		Manifold *module = dynamic_cast<Manifold*>(this->module);
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

Model *modelManifold = createModel<Manifold, ManifoldWidget>("Manifold");
