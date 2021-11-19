//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - 4 Input manual switch
//	Copyright (C) 2021  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Switch4
#define PANEL_FILE "Switch4.svg"

struct Switch4 : Module {
	enum ParamIds {
		SELECT_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(CV_INPUT, 4),
		NUM_INPUTS
	};
	enum OutputIds {
		CV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(SELECT_LIGHT, 4),
		SELECT_PARAM_LIGHT,
		NUM_LIGHTS
	};

	int selection = 0, prevSelection = 0;
	int processCount = 0;
	bool select = false, prevSelect = false;;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"	
	
	Switch4() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configButton(SELECT_PARAM, "Source select");
		configInput(CV_INPUT, "Source 1");
		configInput(CV_INPUT + 1, "Source 2");
		configInput(CV_INPUT + 2, "Source 3");
		configInput(CV_INPUT + 3, "Source 4");
		
		configOutput(CV_OUTPUT, "Signal");

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

				if (++ selection > 3)
					selection = 0;
			}
			
			processCount = 0;
			prevSelect = select;
		}

		// send the selected input to the output
		outputs[CV_OUTPUT].setVoltage(inputs[CV_INPUT + selection].getVoltage());
		lights[SELECT_LIGHT + selection].setBrightness(1.0f);
		
		// turn off previous selected input indicator if we need to
		if (prevSelection != selection) {
			lights[SELECT_LIGHT + prevSelection].setBrightness(0.0f);
			prevSelection = selection;
		}
	}
};

struct Switch4Widget : ModuleWidget {
	std::string panelName;
	
	Switch4Widget(Switch4 *module) {
		setModule(module);

		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"

		int r = 0;
		for (int i = 0; i < 4; i++) {
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[r]), module, Switch4::CV_INPUT + i));
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 20, STD_ROWS6[r]), module, Switch4::SELECT_LIGHT + i));
			r++;
		}
		
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, Switch4::CV_OUTPUT));
		
		addParam(createParamCentered<CountModulaLEDPushButtonMomentary<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, Switch4::SELECT_PARAM, Switch4::SELECT_PARAM_LIGHT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		Switch4 *module = dynamic_cast<Switch4*>(this->module);
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


Model *modelSwitch4 = createModel<Switch4, Switch4Widget>("Switch4");
