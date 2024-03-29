//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - StartupDelay Module
//	Startup Delay Module
//	Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/PulseModifier.hpp"
#include "../inc/Utility.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME StartupDelay
#define PANEL_FILE "StartupDelay.svg"

struct StartupDelay : Module {
	enum ParamIds {
		DELAY_PARAM,	
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		DELAY_OUTPUT,
		GATE_OUTPUT,
		TRIG_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		DELAY_LIGHT,
		GATE_LIGHT,
		TRIG_LIGHT,
		NUM_LIGHTS
	};

	bool starting = true;
	bool ended = false;

	PulseModifier pgStart;
	dsp::PulseGenerator pgEnd;

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	StartupDelay() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(DELAY_PARAM, 1.0f, 30.0f, 1.0f, "Delay", " Seconds");		

		configOutput(DELAY_OUTPUT, "Delay gate");
		configOutput(GATE_OUTPUT, "End of delay gate");
		configOutput(TRIG_OUTPUT, "End of delay trigger");

		outputInfos[DELAY_OUTPUT]->description = "A gate signal that stays high during the delay phase";
		outputInfos[GATE_OUTPUT]->description = "A gate signal that goes high at the end of the delay phase";
		outputInfos[TRIG_OUTPUT]->description = "A trigger signal that fires at the end of the delay phase";

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
		starting = false;
		ended = true;
		pgStart.reset();
		pgEnd.reset();
	}
	
	void process(const ProcessArgs &args) override {

		pgStart.set(params[DELAY_PARAM].getValue());

		// on startup, set the pulse modifier going
		if (starting) {
			starting = false;
			pgStart.restart();
		}
		
		// if we've not yet ended and we're at the  end of the startup pulse counter, flag the end and fire the end tgrigger.
		if (!ended && !pgStart.process(args.sampleTime)) {
			ended = true;
			
			// fire off the trigger pulse
			pgEnd.trigger(1e-3f);
		}
	
		// process the delay light and output
		outputs[DELAY_OUTPUT].setVoltage(boolToGate(!ended));
		lights[DELAY_LIGHT].setSmoothBrightness(boolToLight(!ended), args.sampleTime);
			
		// process the gate light and output
		outputs[GATE_OUTPUT].setVoltage(boolToGate(ended));
		lights[GATE_LIGHT].setSmoothBrightness(boolToLight(ended), args.sampleTime);

		// process the end trigger
		outputs[TRIG_OUTPUT].setVoltage(boolToGate(pgEnd.remaining > 0.0f));
		lights[TRIG_LIGHT].setSmoothBrightness(boolToLight(pgEnd.remaining > 0.0f), args.sampleTime);	
		pgEnd.process(args.sampleTime);
	}
};

struct StartupDelayWidget : ModuleWidget {

	std::string panelName;
	
	StartupDelayWidget(StartupDelay *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"	
		
		// screws
		#include "../components/stdScrews.hpp"	

		// params
		addParam(createParamCentered<Potentiometer<VioletKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2]), module, StartupDelay::DELAY_PARAM));

		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW4]), module, StartupDelay::DELAY_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW6] - 6), module, StartupDelay::GATE_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8] - 12), module, StartupDelay::TRIG_OUTPUT));
		
		// lights
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW3] + 12), module, StartupDelay::DELAY_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW5] + 6), module, StartupDelay::GATE_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW7]), module, StartupDelay::TRIG_LIGHT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		StartupDelay *module = dynamic_cast<StartupDelay*>(this->module);
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

Model *modelStartupDelay = createModel<StartupDelay, StartupDelayWidget>("StartupDelay");
