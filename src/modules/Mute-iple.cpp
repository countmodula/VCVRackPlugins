//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Muteble Module
//	A mutable multiple.
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/SlewLimiter.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME MuteIple
#define PANEL_FILE "Mute-iple.svg"

struct MuteIple : Module {
	enum ParamIds {
		ENUMS(MUTE_PARAMS, 8),
		MODEA_PARAM,
		MODEB_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(SIGNAL_INPUTS, 2),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(SIGNAL_OUTPUTS, 8),
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	LagProcessor slew[8];
	bool softMute[8];
	int numChannels[8];

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"	
	
	MuteIple() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	
		for (int i = 0; i < 8 ; i++) {
			configParam(MUTE_PARAMS + i, 0.0f, 1.0f, 0.0f, "Mute");
		}

		configParam(MODEA_PARAM, 0.0f, 1.0f, 0.0f, "Hard/Soft Mute");
		configParam(MODEB_PARAM, 0.0f, 1.0f, 0.0f, "Hard/Soft Mute");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	void onReset() override {
		for (int i = 0; i < 8 ; i++) {
			slew[i].reset();
			softMute[i] = false;
		}
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

		// grab the inputs 
		Input in1 = inputs[SIGNAL_INPUTS];
		Input in2 = inputs[SIGNAL_INPUTS + 1];
		
		// determine the mute mode
		bool softMuteA = (params[MODEA_PARAM].getValue() > 0.5);
		bool softMuteB = (params[MODEB_PARAM].getValue() > 0.5);

		// apply the normalling behaviour
		if (!in2.isConnected()) {
			// channel b is normalled to channel A
			in2 = in1;

			// mute mode on channel B follows A if Input B is not connected
			softMuteB = softMuteA;
		}
		
		// set the mute mode for each output
		softMute[0] = softMute[1] = softMute [2] = softMute[3] = softMuteA;
		softMute[4] = softMute[5] = softMute [6] = softMute[7] = softMuteB;
		
		// set the number of channels for each output
		numChannels[0] = numChannels[1] = numChannels [2] = numChannels[3] = in1.getChannels();
		numChannels[4] = numChannels[5] = numChannels [6] = numChannels[7] = in2.getChannels();

		
		// send the inputs to the outputs
		float mute;
		for (int i = 0; i < 8; i++) {
			// grab the mute status for this output
			mute = (params[MUTE_PARAMS + i].getValue() > 0.5 ? 0.0f : 1.0f);
			
			// apply the soft mode response if we need to
			if (softMute[i]) {
				// soft mode - apply some slew to soften the switch
				mute = slew[i].process(mute, 1.0f, 0.1f, 0.1f, args.sampleTime);
			}
			else {
				// hard mode - keep slew in sync but don't use it
				slew[i].process(mute, 1.0f, 0.01f, 0.01f, args.sampleTime);
			}
			
			// determine the number of channels and send the inputs to the outputs
			outputs[SIGNAL_OUTPUTS + i].setChannels(numChannels[i]);
			for (int c = 0; c < numChannels[i]; c++) {
				if (i < 4)
					outputs[SIGNAL_OUTPUTS + i].setVoltage(in1.getVoltage(c) * mute, c);
				else
					outputs[SIGNAL_OUTPUTS + i].setVoltage(in2.getVoltage(c) * mute, c);
			}
		}
	}
};


struct MuteIpleWidget : ModuleWidget {
	MuteIpleWidget(MuteIple *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Mute-iple.svg")));

		// screws
		#include "../components/stdScrews.hpp"	
		
		// input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1]), module, MuteIple::SIGNAL_INPUTS));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW5]), module, MuteIple::SIGNAL_INPUTS + 1));
		
		// mode switches
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW3]), module, MuteIple::MODEA_PARAM));
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW7]), module, MuteIple::MODEB_PARAM));

		
		// outputs/lights
		for (int i = 0; i < 8 ; i++) {
			addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1 + i]), module, MuteIple::MUTE_PARAMS + i));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS8[STD_ROW1 + i]), module, MuteIple::SIGNAL_OUTPUTS + i));	
		}
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		MuteIple *module = dynamic_cast<MuteIple*>(this->module);
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

Model *modelMuteIple = createModel<MuteIple, MuteIpleWidget>("Mute-iple");
