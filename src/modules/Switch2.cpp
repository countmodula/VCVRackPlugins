//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack -DPDT manual switch
//  Copyright (C) 2021  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Switch2
#define PANEL_FILE "Switch2.svg"

struct Switch2 : Module {
	enum ParamIds {
		SELECT_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(CVA_INPUT, 2),
		ENUMS(CVB_INPUT, 2),
		NUM_INPUTS
	};
	enum OutputIds {
		CVA_OUTPUT,
		CVB_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(SELECTA_LIGHT, 2),
		ENUMS(SELECTB_LIGHT, 2),
		SELECT_PARAM_LIGHT,
		NUM_LIGHTS
	};

	int selection = 0, prevSelection = 0;
	int processCount = 0;
	bool select = false, prevSelect = false;;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"	
	
	Switch2() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configButton(SELECT_PARAM, "Input select");

		configInput(CVA_INPUT, "A1");
		configInput(CVA_INPUT+ 1, "A2");
		configOutput(CVA_OUTPUT, "A");
		configInput(CVB_INPUT, "B1");
		configInput(CVB_INPUT + 1, "B2");
		configOutput(CVB_OUTPUT, "B");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"	
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(1));
		json_object_set_new(root, "selection", json_integer(selection));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"				
		
		return root;	
	}

	void dataFromJson(json_t* root) override {
		
		json_t *sel = json_object_get(root, "selection");

		if (sel)
			selection = json_integer_value(sel);
			
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}
	
	void onReset() override {
		prevSelection = selection;
		selection = 0;
	}	
	
	void process(const ProcessArgs &args) override {
	
		
		if (++ processCount > 4) {
			
			select = (params[SELECT_PARAM].getValue() > 0);
				
			if (select && !prevSelect) {
				prevSelection = selection;
				
				if (++ selection > 1)
					selection = 0;
			}
			
			processCount = 0;
			prevSelect = select;
		}

		// send the selected inputs to the outputs
		outputs[CVA_OUTPUT].setVoltage(inputs[CVA_INPUT + selection].getVoltage());
		outputs[CVB_OUTPUT].setVoltage(inputs[CVB_INPUT + selection].getVoltage());
		lights[SELECTA_LIGHT + selection].setBrightness(1.0f);
		lights[SELECTB_LIGHT + selection].setBrightness(1.0f);
		
		// turn off previous selected input indicators if we need to
		if (prevSelection != selection) {
			lights[SELECTA_LIGHT + prevSelection].setBrightness(0.0f);
			lights[SELECTB_LIGHT + prevSelection].setBrightness(0.0f);
			prevSelection = selection;
		}
	}
};

struct Switch2Widget : ModuleWidget {
	std::string panelName;
	
	Switch2Widget(Switch2 *module) {
		setModule(module);

		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"

		int r = 0;
		for (int i = 0; i < 2; i++) {
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[r]), module, Switch2::CVA_INPUT + i));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[3 + r]), module, Switch2::CVB_INPUT + i));
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 20, STD_ROWS7[r]), module, Switch2::SELECTA_LIGHT + i));
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 20, STD_ROWS7[3 + r]), module, Switch2::SELECTB_LIGHT + i));
			r++;
		}
		
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW3]), module, Switch2::CVA_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW6]), module, Switch2::CVB_OUTPUT));
		
		// deliberately still on 6 row grid to put buttons for both switches at same position. 
		addParam(createParamCentered<CountModulaLEDPushButtonMomentary<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, Switch2::SELECT_PARAM, Switch2::SELECT_PARAM_LIGHT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		Switch2 *module = dynamic_cast<Switch2*>(this->module);
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


Model *modelSwitch2 = createModel<Switch2, Switch2Widget>("Switch2");
