//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Multiple
//	Copyright (C) 2021  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Mult
#define PANEL_FILE "Mult.svg"

#define NUM_MULTS 8

using simd::float_4;

struct Mult : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(MULT_INPUT, NUM_MULTS),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(MULT_OUTPUT, NUM_MULTS),
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	Mult() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		char c ='A';
		std::string s, prevS = "";
		for (int i = 0; i < NUM_MULTS; i++) {
			s = c++;
			configInput(MULT_INPUT + i, s);
			configOutput(MULT_OUTPUT + i, s);
			if (i > 0)
				inputInfos[MULT_INPUT + i]->description = "Normalled to " + prevS + " input";

			prevS = s;
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
	

	void process(const ProcessArgs &args) override {

		int iToUse = 0;
		int numChans = 1;
		
		for (int i = 0; i < NUM_MULTS; i++) {
			if (i == 0) {
				numChans = inputs[i].getChannels();
			}
			else if (inputs[i].isConnected()) {
				iToUse = i;
				numChans = inputs[i].getChannels();
			}

			if (outputs[i].isConnected()) {
				outputs[i].setChannels(numChans);
				for (int c = 0; c < numChans; c += 4) {
					float_4 v = inputs[iToUse].getPolyVoltageSimd<float_4>(c);
					outputs[i].setVoltageSimd(v, c);
				}
			}
		}
	}
};

struct MultWidget : ModuleWidget {
	std::string panelName;

	MultWidget(Mult *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"

		// screws
		#include "../components/stdScrews.hpp"	

		for (int g = 0; g < NUM_MULTS; g++) {
			// inputs
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 6, STD_ROWS8[STD_ROW1 + g]),module, Mult::MULT_INPUT + g));
			
			// outputs
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 6, STD_ROWS8[STD_ROW1 + g]), module, Mult::MULT_OUTPUT + g));
		}
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		Mult *module = dynamic_cast<Mult*>(this->module);
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

Model *modelMult = createModel<Mult, MultWidget>("Mult");
