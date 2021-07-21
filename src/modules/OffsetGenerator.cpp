//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Offset Generator
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/GateProcessor.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME OffsetGenerator
#define PANEL_FILE "OffsetGenerator.svg"

struct OffsetGenerator : Module {
	enum ParamIds {
		COARSE_PARAM,
		FINE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		COARSE_INPUT,
		TRIG_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		MIX_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	GateProcessor gateTrig;
	float cv[PORT_MAX_CHANNELS] = {};
	
	OffsetGenerator() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	
		configParam(COARSE_PARAM, -8.0f, 8.0f, 0.0f, "Coarse", " V");
		configParam(FINE_PARAM, -1.0f, 1.0f, 0.0f, "Fine", " V");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	void onReset() override {
		gateTrig.reset();
		for (int i = 0; i < PORT_MAX_CHANNELS;i++)
			cv[i] = 0.0f;
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

		// process trigger input
		bool trig = true;
		if (inputs[TRIG_INPUT].isConnected()) {
			gateTrig.set(inputs[TRIG_INPUT].getVoltage());
			trig = gateTrig.leadingEdge();
		}
		
		// determine number of channels
		int n = inputs[CV_INPUT].getChannels();
		outputs[MIX_OUTPUT].setChannels(n);

		if (trig) {
			// grab the coarse offset
			float offset = 0.0f;
			if (inputs[COARSE_INPUT].isConnected())
				offset = floor(inputs[COARSE_INPUT].getVoltage());
			else
				offset = params[COARSE_PARAM].getValue();
			
			offset = offset + params[FINE_PARAM].getValue();
			
			for (int i = 0; i < n; i ++) {
				cv[i] = clamp(inputs[CV_INPUT].getVoltage(i) + offset, -12.0f, 12.0f);
			}
		}
		
		
		if (n > 0) {
			for (int i  = 0; i < n; i ++)
				outputs[MIX_OUTPUT].setVoltage(cv[i], i);
		}
		else
			outputs[MIX_OUTPUT].setVoltage(0.0f);
	}
};

struct OffsetGeneratorWidget : ModuleWidget {

	std::string panelName;
	
	OffsetGeneratorWidget(OffsetGenerator *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	
		
		// knobs
		addParam(createParamCentered<RotarySwitch<BlueKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, OffsetGenerator::COARSE_PARAM));
		addParam(createParamCentered<Potentiometer<BlueKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, OffsetGenerator::FINE_PARAM));
		
		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, OffsetGenerator::COARSE_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, OffsetGenerator::TRIG_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, OffsetGenerator::CV_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, OffsetGenerator::MIX_OUTPUT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		OffsetGenerator *module = dynamic_cast<OffsetGenerator*>(this->module);
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

Model *modelOffsetGenerator = createModel<OffsetGenerator, OffsetGeneratorWidget>("OffsetGenerator");
