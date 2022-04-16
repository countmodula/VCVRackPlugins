//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - 8 to 1 random access switch
//  Copyright (C) 2021  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME RandomAccessSwitch81
#define PANEL_FILE "RandomAccessSwitch81.svg"

struct RandomAccessSwitch81 : Module {
	enum ParamIds {
		ENUMS(SELECT_PARAM, 8),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(CV_INPUT, 8),
		ENUMS(SELECT_INPUT, 8),
		NUM_INPUTS
	};
	enum OutputIds {
		CV_OUTPUT,
		TRIG_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(SELECT_LIGHT, 8),
		TRIG_LIGHT,
		NUM_LIGHTS
	};

	GateProcessor gpSelect[8];
	dsp::PulseGenerator pgChange;
	int selection = 0, prevSelection = 0;
	int processCount = 0;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"	
	
	RandomAccessSwitch81() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configOutput(CV_OUTPUT, "Destination");
		configOutput(TRIG_OUTPUT, "Change trigger");

		for (int i = 0, c = 1; i < 8; i++, c++) {
			configButton(SELECT_PARAM + i, rack::string::f("Select %d", c));
			configInput(SELECT_INPUT + i, rack::string::f("Select %d", c));
			configInput(CV_INPUT + i, rack::string::f("Source %d", c));
		}
		
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
		pgChange.reset();
		prevSelection = selection;
		selection = 0;
	}	

	void process(const ProcessArgs &args) override {
	
		if (++ processCount > 8) {
			for (int i = 0; i < 8; i++) {
				gpSelect[i].set(params[SELECT_PARAM + i].getValue() > 0.5f ? 10.0f : inputs[SELECT_INPUT + i].getVoltage());
				if (gpSelect[i].leadingEdge()) {
					selection = i;
				}
			}

			processCount = 0;
		}

		outputs[CV_OUTPUT].setVoltage(inputs[CV_INPUT + selection].getVoltage());
		lights[SELECT_LIGHT + selection].setBrightness(1.0f);

		if (prevSelection != selection) {
			pgChange.trigger(1e-3f);
			lights[SELECT_LIGHT + prevSelection].setBrightness(0.0f);
			
			prevSelection = selection;
		}
	
		if (pgChange.remaining > 0.0f) {
			outputs[TRIG_OUTPUT].setVoltage(10.0f);
			lights[TRIG_LIGHT].setBrightness(1.0);
		}
		else {
			outputs[TRIG_OUTPUT].setVoltage(0.0f);
			lights[TRIG_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
		}
		
		pgChange.process(args.sampleTime);
		
	}
};

struct RandomAccessSwitch81Widget : ModuleWidget {
	std::string panelName;
	
	// custom columns for this module to squeeze it up a little
	const int COLUMN_POSITIONS[8] = {
		30,
		55,
		80,
		105,
		130,
		155,
		180,
		205
	};

	RandomAccessSwitch81Widget(RandomAccessSwitch81 *module) {
		setModule(module);

		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"

		for (int i = 0; i < 8; i++) {
			addInput(createInputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL1], STD_ROWS8[i]), module, RandomAccessSwitch81::CV_INPUT + i));
			addInput(createInputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL3], STD_ROWS8[i]), module, RandomAccessSwitch81::SELECT_INPUT + i));
			addParam(createParamCentered<CountModulaUnlitPushButtonMomentary>(Vec(COLUMN_POSITIONS[STD_COL5], STD_ROWS8[i]), module, RandomAccessSwitch81::SELECT_PARAM + i));
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(COLUMN_POSITIONS[STD_COL2], STD_ROWS8[i]), module, RandomAccessSwitch81::SELECT_LIGHT + i));
		}
		
		addOutput(createOutputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL7], STD_HALF_ROWS8(STD_ROW2)), module, RandomAccessSwitch81::CV_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL7], STD_HALF_ROWS8(STD_ROW6)), module, RandomAccessSwitch81::TRIG_OUTPUT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(COLUMN_POSITIONS[STD_COL7] + 15, STD_HALF_ROWS8(STD_ROW6) - 19), module, RandomAccessSwitch81::TRIG_LIGHT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		RandomAccessSwitch81 *module = dynamic_cast<RandomAccessSwitch81*>(this->module);
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


Model *modelRandomAccessSwitch81 = createModel<RandomAccessSwitch81, RandomAccessSwitch81Widget>("RandomAccessSwitch81");
