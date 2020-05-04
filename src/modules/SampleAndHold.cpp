//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Sample & Hold Module
//	Sample/Track/Pass and Hold
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME SampleAndHold
#define PANEL_FILE "SampleAndHold.svg"

struct SampleAndHold : Module {
	enum ParamIds {
		MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		SAMPLE_INPUT,
		TRIG_INPUT,
		MODE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		SAMPLE_OUTPUT,
		INV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		TRACK_LIGHT,
		SAMPLE_LIGHT,
		PASS_LIGHT,
		NUM_LIGHTS
	};

	
	enum Modes {
		SAMPLE,
		TRACK,
		PASS
	};
	
	GateProcessor gateTrig;

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	SampleAndHold() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	
		// tracking mode switch
		configParam(MODE_PARAM, 0.0f, 2.0f, 0.0f, "Sample, Track or Pass Mode");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	void onReset() override {
		gateTrig.reset();
	}	
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(3));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"				
		
		return root;	
	}

	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}	
	
	void process(const ProcessArgs &args) override {

		// process trigger input
		gateTrig.set(inputs[TRIG_INPUT].getVoltage());
		
		// determine the mode - input takes precedence over switch
		int trackMode = SAMPLE;
		if (inputs[MODE_INPUT].isConnected()) {
			trackMode =  (int)(clamp(inputs[MODE_INPUT].getVoltage(), 0.0f, 5.0f)) / 2;
		}
		else
			trackMode = (int)(params[MODE_PARAM].getValue());
				
		// determine number of channels
		int n = inputs[SAMPLE_INPUT].getChannels();
		outputs[SAMPLE_OUTPUT].setChannels(n);
		outputs[INV_OUTPUT].setChannels(n);

		// now sample away
		float s;
		for (int c = 0; c < n; c++) {
			if ((trackMode == TRACK && gateTrig.high()) || (trackMode == SAMPLE && gateTrig.leadingEdge()) || (trackMode == PASS && !gateTrig.high())) {
				// track, pass  or sample the input
				s = inputs[SAMPLE_INPUT].getVoltage(c);
			}
			else {
				// hold the last sample value
				s = outputs[SAMPLE_OUTPUT].getVoltage(c);
			}
			
			// set the output voltages
			outputs[SAMPLE_INPUT].setVoltage(s, c);
			outputs[INV_OUTPUT].setVoltage(-s, c);
		}
		
		lights[PASS_LIGHT].setBrightness(boolToLight(trackMode == PASS));	
		lights[TRACK_LIGHT].setBrightness(boolToLight(trackMode == TRACK));	
		lights[SAMPLE_LIGHT].setBrightness(boolToLight(trackMode == SAMPLE));
	}
	
};

struct SampleAndHoldWidget : ModuleWidget {
	SampleAndHoldWidget(SampleAndHold *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SampleAndHold.svg")));

		// screws
		#include "../components/stdScrews.hpp"	

		// inputs`
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]),module, SampleAndHold::SAMPLE_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]),module, SampleAndHold::TRIG_INPUT));

		// mode switch
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, SampleAndHold::MODE_PARAM));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, SampleAndHold::SAMPLE_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, SampleAndHold::INV_OUTPUT));

		// mode input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]),module, SampleAndHold::MODE_INPUT));
		
		// lights
		addChild(createLightCentered<SmallLight<YellowLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 19, STD_ROWS6[STD_ROW3] - 18), module, SampleAndHold::PASS_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 19, STD_ROWS6[STD_ROW3]), module, SampleAndHold::TRACK_LIGHT));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 19, STD_ROWS6[STD_ROW3] + 18), module, SampleAndHold::SAMPLE_LIGHT));
	}
	
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		SampleAndHold *module = dynamic_cast<SampleAndHold*>(this->module);
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

Model *modelSampleAndHold = createModel<SampleAndHold, SampleAndHoldWidget>("SampleAndHold");
