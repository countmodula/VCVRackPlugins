//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Manual Gate Module
//	A simple manual gate generator with a nice big button offering gate, latch
//	extended gate and trigger outputs 
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/PulseModifier.hpp"
#include "../inc/Utility.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME MasterReset
#define PANEL_FILE "MasterReset.svg"

struct MasterReset : Module {
	enum ParamIds {
		RESET_PARAM,
		LENGTH_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		RESET_INPUT,
		ENUMS(CLOCK_INPUTS, 4),
		NUM_INPUTS
	};
	
	enum OutputIds {
		RESET_OUTPUT,
		ENUMS(CLOCK_OUTPUTS, 4),
		NUM_OUTPUTS
	};
	enum LightIds {
		RESET_PARAM_LIGHT,
		NUM_LIGHTS
	};
	
	GateProcessor gateReset;
	dsp::PulseGenerator  pgReset;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	MasterReset() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(RESET_PARAM, 0.0f, 1.0f, 0.0f, "Reset");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	void onReset() override {
		gateReset.reset();
		pgReset.reset();
	}	
	
	json_t* dataToJson() override {
		json_t* root = json_object();

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
		
		gateReset.set(fmax(inputs[RESET_INPUT].getVoltage(), params[RESET_PARAM].getValue() * 10.0f));
		
		bool isResetting = false;
		if (gateReset.leadingEdge()) {
			pgReset.trigger(1e-4f);
			isResetting = true;
		}
		else {
			isResetting = pgReset.process(args.sampleTime);
		}	
		
		// process the poly enabled clock input first
		float clk = 0.0f;
		if (inputs[CLOCK_INPUTS].isConnected()) {
			int numChannels = inputs[CLOCK_INPUTS].getChannels();
			outputs[CLOCK_OUTPUTS].setChannels(numChannels);
			
			for (int c = 0; c < numChannels; c++) {
				clk = isResetting ? 0.0f : inputs[CLOCK_INPUTS].getVoltage(c);
				outputs[CLOCK_OUTPUTS].setVoltage(clk, c);
			}			
		}
		else
			outputs[CLOCK_OUTPUTS].channels = 0;	
		
		// now do the remaining 3 clocks
		for (int i = 1; i < 4; i++) {
			clk = isResetting ? 0.0f : inputs[CLOCK_INPUTS + i].getVoltage();
			outputs[CLOCK_OUTPUTS + i].setVoltage(clk);
		}
		
		outputs[RESET_OUTPUT].setVoltage(boolToGate(gateReset.high()));
	}
};

struct MasterResetWidget : ModuleWidget {

	std::string panelName;
	
	MasterResetWidget(MasterReset *module) {	
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	
		
		// clock inputs/outputs
		for (int i = 0; i < 4; i++) {
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW1 + i]), module, MasterReset::CLOCK_INPUTS + i));	
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS7[STD_ROW1 + i]), module, MasterReset::CLOCK_OUTPUTS + i));	
		}
		
		// reset inputs/outputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW5]), module, MasterReset::RESET_INPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS7[STD_ROW5]), module, MasterReset::RESET_OUTPUT));	
		
		// Mega manual button - non-standard position
		addParam(createParamCentered<CountModulaLEDPushButtonMegaMomentary<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS8[STD_ROW7]), module, MasterReset::RESET_PARAM, MasterReset::RESET_PARAM_LIGHT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		MasterReset *module = dynamic_cast<MasterReset*>(this->module);
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

Model *modelMasterReset = createModel<MasterReset, MasterResetWidget>("MasterReset");
