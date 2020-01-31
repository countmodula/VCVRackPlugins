//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Polyphonic Mute
//	A polyphonic mute.
//  Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/SlewLimiter.hpp"
#include "../inc/GateProcessor.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME PolyMute
#define PANEL_FILE "PolyMute.svg"

#define NUM_CHANS 16
#define NUM_ROWS 8

struct PolyMute : Module {
	enum ParamIds {
		ENUMS(MUTE_PARAMS, NUM_CHANS),
		MASTER_PARAM,
		MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		POLY_INPUT,
		MUTE_INPUT,
		MASTER_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		POLY_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	
	GateProcessor gateMaster;
	GateProcessor gateMutes[NUM_CHANS];
	LagProcessor slewMutes[NUM_CHANS];
	bool softMute;
	int numChannels;

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"	
	
	PolyMute() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	
	
		char buffer[100];
		for (int i = 0; i < NUM_CHANS ; i++) {
			sprintf(buffer, "Mute channel %d", i + 1);	
			configParam(MUTE_PARAMS + i, 0.0f, 1.0f, 0.0f, buffer);
		}

		configParam(MODE_PARAM, 0.0f, 1.0f, 0.0f, "Hard/Soft Mute");
		configParam(MASTER_PARAM, 0.0f, 1.0f, 0.0f, "Master Mute");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	void onReset() override {
		for (int i = 0; i < NUM_CHANS ; i++) {
			slewMutes[i].reset();
			gateMutes[i].reset();
		}
		
		softMute = false;
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
	
	void process(const ProcessArgs &args) override {

		// determine the mute mode
		softMute = (params[MODE_PARAM].getValue() > 0.5);
			
		// master muted?
		gateMaster.set(inputs[MASTER_INPUT].getNormalVoltage(params[MASTER_PARAM].getValue() * 10.0f));
		params[MASTER_PARAM].setValue(gateMaster.high() ? 1.0f : 0.0f);

		if (inputs[POLY_INPUT].isConnected()) {
			numChannels = inputs[POLY_INPUT].getChannels();
		
			outputs[POLY_OUTPUT].setChannels(numChannels);

			// send the inputs to the outputs
			float mute;
			for (int i = 0; i < NUM_CHANS; i++) {
				gateMutes[i].set(inputs[MUTE_INPUT].getNormalPolyVoltage(params[MUTE_PARAMS + i].getValue() * 10.0f, i));
				params[MUTE_PARAMS + i].setValue(gateMutes[i].high() ? 1.0f : 0.0f);
				
				// grab the mute status for this output
				 mute = (gateMaster.high() || gateMutes[i].high()) ? 0.0f : 1.0f;
							
				// apply the soft mode response if we need to
				if (softMute) {
					// soft mode - apply some slew to soften the switch
					mute = slewMutes[i].process(mute, 1.0f, 0.1f, 0.1f);
				}
				else {
					// hard mode - keep slew in sync but don't use it
					slewMutes[i].process(mute, 1.0f, 0.01f, 0.01f);
				}
				
				outputs[POLY_OUTPUT].setVoltage(inputs[POLY_INPUT].getVoltage(i) * mute, i);
			}
		}
		else {
			outputs[POLY_OUTPUT].channels = 0;
		}
	}
};

struct PolyMuteWidget : ModuleWidget {
	PolyMuteWidget(PolyMute *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PolyMute.svg")));

		// screws
		#include "../components/stdScrews.hpp"	
		
		// inputs/outputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1]), module, PolyMute::MUTE_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW7]), module, PolyMute::POLY_INPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, PolyMute::POLY_OUTPUT));	
		
		// mode switch
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW5)), module, PolyMute::MODE_PARAM));

		// mute buttons
		int j = 0;
		for (int i = 0; i < NUM_CHANS ; i++) {
			addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[i < NUM_ROWS ? STD_COL3 : STD_COL5], STD_ROWS8[STD_ROW1 + j]), module, PolyMute::MUTE_PARAMS + i));
			if (++j == NUM_ROWS)
				j = 0;
		}
		
		// master
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW3]), module, PolyMute::MASTER_INPUT));
		addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW4]), module, PolyMute::MASTER_PARAM));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		PolyMute *module = dynamic_cast<PolyMute*>(this->module);
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

Model *modelPolyMute = createModel<PolyMute, PolyMuteWidget>("PolyMute");
