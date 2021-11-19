//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Voltage Inverter Module
//	A basic quad voltage inverter
//	Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME VoltageInverter
#define PANEL_FILE "VoltageInverter.svg"

struct VoltageInverter : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		A_INPUT,
		B_INPUT,
		C_INPUT,
		D_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		A_OUTPUT,
		B_OUTPUT,
		C_OUTPUT,
		D_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};
	
	float out = 0.0f;

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	VoltageInverter() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configInput(A_INPUT, "A");
		configInput(B_INPUT, "B");
		configInput(C_INPUT, "C");
		configInput(D_INPUT, "D");

		configOutput(A_OUTPUT, "A inverted");
		configOutput(B_OUTPUT, "B inverted");
		configOutput(C_OUTPUT, "C inverted");
		configOutput(D_OUTPUT, "D inverted");

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
		int inputToUse = 0;
		for (int i = 0; i < 4; i++) {
			// determine which input we want to actually use
			if (inputs[A_INPUT + i].isConnected())
				inputToUse = i;
				
			// grab output value normallised to the previous input value
			int n = inputs[A_INPUT + inputToUse].getChannels();
			outputs[A_OUTPUT + i].setChannels(n);
			for (int c = 0; c < n; c++)
				outputs[A_OUTPUT + i].setVoltage(-inputs[A_INPUT + inputToUse].getVoltage(c), c);
		}
	}
};

struct VoltageInverterWidget : ModuleWidget {

	std::string panelName;
	
	VoltageInverterWidget(VoltageInverter *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	

		for (int i = 0; i < 4; i++) {
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1 + (i * 2)]), module, VoltageInverter::A_INPUT + i));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2 + (i * 2)]), module, VoltageInverter::A_OUTPUT + (i)));
		}
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		VoltageInverter *module = dynamic_cast<VoltageInverter*>(this->module);
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

Model *modelVoltageInverter = createModel<VoltageInverter, VoltageInverterWidget>("VoltageInverter");
