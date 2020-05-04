//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Step Sequencer Module
//  A classic 8 step CV/Gate sequencer
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME BusRoute2
#define PANEL_FILE "BusRoute2.svg"

struct BusRoute2 : Module {

	enum ParamIds {
		ENUMS(BUS_A_PARAMS, 7),
		ENUMS(BUS_B_PARAMS, 7),
		NUM_PARAMS
	};
	
	enum InputIds {
		ENUMS(GATE_INPUTS, 7),
		NUM_INPUTS
	};
	
	enum OutputIds {
		A_OUTPUT,
		B_OUTPUT,
		NUM_OUTPUTS
	};
	
	enum LightIds {
		A_LIGHT,
		B_LIGHT,
		NUM_LIGHTS
	};

	
	GateProcessor gates[7];
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	BusRoute2() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	
		// step params
		for (int s = 0; s < 7; s++) {
			configParam(BUS_A_PARAMS + s, 0.0f, 1.0f, 0.0f, "Bus A Select");
			configParam(BUS_B_PARAMS + s, 0.0f, 1.0f, 0.0f, "Bus B Select");
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
		for (int i = 0; i < 7; i++) {
			gates[i].reset();
		}
	}	
	
	void process(const ProcessArgs &args) override {

		// now process the lights and outputs
		bool aOut = false;
		bool bOut = false;	
			
		for (int i = 0; i < 7; i++) {
		
			gates[i].set(inputs[i].getVoltage());
			
			// determine the output values
			if (gates[i].high()) {
				aOut |= (params[BUS_A_PARAMS + i].getValue() > 0.5f);
				bOut |= (params[BUS_B_PARAMS + i].getValue() > 0.5f);
			}
		}
			
		// set the outputs accordingly
		outputs[A_OUTPUT].setVoltage(boolToGate(aOut));	
		outputs[B_OUTPUT].setVoltage(boolToGate(bOut));	
	
		lights[A_LIGHT].setBrightness(boolToLight(aOut));	
		lights[B_LIGHT].setBrightness(boolToLight(bOut));

	}
};

struct BusRoute2Widget : ModuleWidget {
	BusRoute2Widget(BusRoute2 *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BusRoute2.svg")));

		// screws
		#include "../components/stdScrews.hpp"	

		// inputs and switches
		for (int s = 0; s < 7; s++) {
			addParam(createParamCentered<CountModulaPBSwitchMini>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1 + s]), module, BusRoute2::BUS_A_PARAMS + s));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS8[STD_ROW1 + s]), module, BusRoute2::GATE_INPUTS + s));
			addParam(createParamCentered<CountModulaPBSwitchMini>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1 + s]), module, BusRoute2::BUS_B_PARAMS + s));
		}

		// output lights
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW7)), module, BusRoute2::A_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_HALF_ROWS8(STD_ROW7)), module, BusRoute2::B_LIGHT));

		// a/b outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, BusRoute2::A_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW8]), module, BusRoute2::B_OUTPUT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		BusRoute2 *module = dynamic_cast<BusRoute2*>(this->module);
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

Model *modelBusRoute2 = createModel<BusRoute2, BusRoute2Widget>("BusRoute2");
